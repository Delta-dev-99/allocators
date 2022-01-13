/* #include <iostream>
#include <string_view>
#include <memory>
#include <concepts>
#include <forward_list>
#include <tuple>

#include <cstdint>


// Notes on safety: NEVER deallocate more than once.
// Deallocating with these allocators is a noop if they don't own the memory.
// But if they do own the memory and it is already free, it will cause big problems.


namespace dd99
{
    namespace memory
    {
        struct Block
        {
            void *base = nullptr;
            std::size_t size = 0;

            auto get_end() const { return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) + size); }
            
            bool contains(const Block &other) const
            {
                const auto base_offset = reinterpret_cast<std::intptr_t>(other.base) - reinterpret_cast<std::intptr_t>(base);
                return (base_offset >= 0) && (other.size + base_offset <= size);
            }

            bool contains(void * ptr) const
            {
                return (base <= ptr) && (get_end() >= ptr);
            }
        };


        template <std::size_t Size>
        struct Acquire_Stack_Block : Block
        {
            Acquire_Stack_Block()
                : Block{.base = m_data, .size = Size}
            { }

            char m_data[Size];
        };

        template <std::size_t Size>
        struct Acquire_Heap_Block : Block
        {
            Acquire_Heap_Block()
                : m_data(std::make_unique<char>(Size))
                , Block{.base = m_data.get(), .size = Size}
            { }

            std::unique_ptr<char> m_data;
        };
    
        namespace freelist
        {
            // singly linked list of free memory blocks that stores
            // nodes on empty blocks
            template <std::size_t Block_Size>
            class Fixed_Size
            {
            protected:
                // header for free blocks
                // the nodes of the list
                struct Free_Block_Header
                {
                    Free_Block_Header *next = nullptr;
                };
                static_assert(sizeof(Free_Block_Header) <= Block_Size);

            private:
                Free_Block_Header *first = nullptr;

            public:
                ~Fixed_Size()
                {
                    // call destructor on created nodes
                    clear();
                }

                memory::Block pop()
                {
                    if (!first) return {};
                    auto current = first;
                    first = first->next;
                    current->~Free_Block_Header();
                    return {.base = current, .size = Block_Size};
                }

                void push(const memory::Block& memory)
                {
                    auto new_header = new (memory.base) Free_Block_Header{.next = first};
                    first = new_header;
                }

                void clear()
                {
                    auto current = first;
                    first = nullptr;
                    while (current)
                    {
                        const auto tmp = current->next;
                        current->~Free_Block_Header();
                        current = tmp;
                    }
                }
            };
        
            // singly linked list of free memory blocks that stores
            // nodes on empty blocks of different sizes
            class Sized_Blocks
            {
            protected:
                // header for free blocks
                // the nodes of the list
                struct Free_Sized_Block_Header
                {
                    Free_Sized_Block_Header *next = nullptr;
                    std::size_t size;

                    // get corresponding memory block
                    auto get_memory_block() { return memory::Block{.base = this, .size = size}; }
                };
                
            private:
                Free_Sized_Block_Header *first = nullptr;

            public:
                ~Sized_Blocks()
                {
                    // call destructor on created nodes
                    clear();
                }

                memory::Block extract(std::size_t min_size)
                {
                    // smaller allocations not allowed
                    if (min_size < sizeof(Free_Sized_Block_Header))
                        min_size = sizeof(Free_Sized_Block_Header);

                    auto current = first;
                    decltype(current) prev = nullptr;
                    while (current)
                    {
                        if (current->size >= min_size)
                        {
                            // found a suitable block.
                            if (current->size - min_size >= sizeof(Free_Sized_Block_Header))
                            {
                                // Remaining space is large enough to fit a free list header.
                                // Divide the block.
                                current->size -= min_size;
                                return {.base = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(current) + current->size), .size = min_size};
                            }
                            else
                            {
                                // Can't leave a space smaller than sizeof(Free_Sized_Block_Header).
                                // Extract the full block
                                if (prev) prev->next = current->next;
                                else first = current->next;
                                
                                auto r = current->get_memory_block();
                                current->~Free_Sized_Block_Header();
                                return r;
                            }
                        }

                        prev = current;
                        current = current->next;
                    }

                    // no suitable memory found
                    return {};
                }

                // sorted by base address
                void insert(const memory::Block& memory)
                {
                    // insert into list and get pointer to node previous to inserted one
                    // the pointer to the inserted node is do_insert(memory)->next
                    auto prev_ptr = do_insert(memory);

                    // Merge adjacent free blocks
                    if (prev_ptr)
                    {
                        if (!try_merge(prev_ptr))
                            prev_ptr = prev_ptr->next;
                    }
                    else
                        prev_ptr = first;

                    try_merge(prev_ptr);
                }

                void clear()
                {
                    auto current = first;
                    first = nullptr;
                    while (current)
                    {
                        const auto tmp = current->next;
                        current->~Free_Sized_Block_Header();
                        current = tmp;
                    }
                }
            
            private:
                // returns a pointer to the node before the inserted node
                Free_Sized_Block_Header *do_insert(const memory::Block& memory)
                {
                    //
                    // find position and link new node
                    //

                    if (!first)
                    {
                        auto new_header = new (memory.base) Free_Sized_Block_Header{.next = nullptr, .size = memory.size};
                        first = new_header;
                        return nullptr;
                    }

                    if (first > memory.base)
                    {
                        auto new_header = new (memory.base) Free_Sized_Block_Header{.next = first, .size = memory.size};
                        first = new_header;
                        return nullptr; // node before the new node does not exist
                    }

                    auto place = first;
                    while(place->next)
                    {
                        if (place->next > memory.base)
                        {
                            auto new_header = new (memory.base) Free_Sized_Block_Header{.next = place->next, .size = memory.size};
                            place->next = new_header;
                            return place;
                        }
                        place = place->next;
                    }

                    auto new_header = new (memory.base) Free_Sized_Block_Header{.size = memory.size};
                    place->next = new_header;
                    return place;
                }

                // merge a block with the next one if adjacent
                bool try_merge(Free_Sized_Block_Header *prev_ptr)
                {
                    if (prev_ptr->get_memory_block().get_end() == prev_ptr->next)
                    {
                        prev_ptr->size += prev_ptr->next->size;
                        prev_ptr->next = prev_ptr->next->next;
                        prev_ptr->next->~Free_Sized_Block_Header();
                        return true;
                    }

                    return false;
                }
            };
        }
    }


    namespace block_allocator
    {
        // abstract base class
        struct Allocator
        {
            virtual ~Allocator() = default;

            Allocator() = default;
            Allocator(const Allocator &) = delete; // no copy
            Allocator(Allocator &&) = default; // move allowed

            virtual memory::Block allocate(std::size_t requested_size) = 0;
            virtual void deallocate(const memory::Block &memory) = 0;
            virtual void deallocate_all() = 0;

            virtual bool owns(const memory::Block &memory) = 0;
        };

        
        // Only handles contiguous memory
        class Stack : public Allocator
        {
        public:
            Stack(const memory::Block &memory)
                : m_memory(memory)
                , m_current(reinterpret_cast<std::uintptr_t>(memory.base))
            { }
            
        public:
            memory::Block allocate(std::size_t requested_size)
            {
                const auto used_size = reinterpret_cast<std::uintptr_t>(m_current) - reinterpret_cast<std::uintptr_t>(m_memory.base);
                const auto remaining_size = m_memory.size - used_size;
                if (remaining_size >= requested_size)
                {
                    memory::Block current{.base = reinterpret_cast<void *>(m_current), .size = requested_size};
                    m_current += requested_size;
                    return current;
                }

                return {};
            }

            // Can only free the last allocated block
            void deallocate(const memory::Block &memory)
            {
                if (reinterpret_cast<std::uintptr_t>(memory.get_end()) == m_current)
                {
                    m_current -= memory.size;
                }
            }

            void deallocate_all()
            {
                m_current = reinterpret_cast<std::uintptr_t>(m_memory.base);
            }

            bool owns(const memory::Block &memory)
            {
                return m_memory.contains(memory);
            }

        private:
            memory::Block m_memory;
            std::uintptr_t m_current;
        };

        // Allocate fixed-size blocks
        // Uses a free list (forward list with nodes in unused blocks)
        // Requires Block_Size to be large enough to fit a list node in a block
        template <std::size_t Block_Size>
        class Pool : public Allocator
        {
        public:
            Pool(const memory::Block &memory)
                : m_memory(memory)
            {
                build_free_list();
            }

        public:
            memory::Block allocate(std::size_t requested_size)
            {
                if (requested_size <= Block_Size)
                    return m_free_list.pop();

                // larger allocations not supported
                return {};
            }

            void deallocate(const memory::Block &memory)
            {
                if (m_memory.contains(memory))
                    m_free_list.push(memory);
            }

            void deallocate_all()
            {
                m_free_list.clear();
                build_free_list();
            }

            bool owns(const memory::Block& memory)
            {
                return m_memory.contains(memory);
            }

        private:
            void build_free_list()
            {
                memory::Block current{.base = m_memory.base, .size = Block_Size};
                while(current.get_end() <= m_memory.get_end())
                {
                    m_free_list.push(current);
                    // advance
                    current.base = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(current.base) + Block_Size);
                }
            }

        private:
            memory::Block m_memory;
            // singly linked list
            // std::forward_list<int> m_free_list;
            memory::freelist::Fixed_Size<Block_Size> m_free_list;
        };

        // Uses a free list (linked list with nodes in unused blocks)
        // Requires allocation sizes to be large enough to fit a list node (adjusted if not)
        class Free_List : public Allocator
        {
        public:
            Free_List(const memory::Block& memory)
                : m_memory(memory)
            {
                m_free_list.insert(m_memory);
            }

        public:
            memory::Block allocate(std::size_t requested_size)
            {
                return m_free_list.extract(requested_size);
            }

            void deallocate(const memory::Block &memory)
            {
                if (m_memory.contains(memory))
                    m_free_list.insert(memory);
            }

            void deallocate_all()
            {
                m_free_list.clear();
                m_free_list.insert(m_memory);
            }

            bool owns(const memory::Block& memory)
            {
                return m_memory.contains(memory);
            }

        private:
            memory::Block m_memory;
            memory::freelist::Sized_Blocks m_free_list;
        };



        namespace composite
        {

            template <class Primary_T, class Fallback_T>
            class Fallback_Allocator : public Allocator, private Primary_T, private Fallback_T
            {
            public:
                Fallback_Allocator(Primary_T &&primary, Fallback_T &&fallback)
                    : Primary_T(primary), Fallback_T(fallback)
                { }

                memory::Block allocate(std::size_t requested_size)
                {
                    auto r = Primary_T::allocate(requested_size);
                    if (!r) r = Fallback_T::allocate(requested_size);
                    return r;
                }

                void deallocate(const memory::Block &memory)
                {
                    // TODO: Allocators should check if they own the memory
                    // before deallocating it.
                    // It should be OK to skip the check in higher-level allocators.

                    // if (Primary_T::owns(memory))
                        Primary_T::deallocate(memory);
                    // else if (Fallback_T::owns(memory))
                        Fallback_T::deallocate(memory);
                }

                void deallocate_all()
                {
                    Primary_T::deallocate_all();
                    Fallback_T::deallocate_all();
                }

                bool owns(const memory::Block &memory)
                {
                    return Primary_T::owns(memory) || Fallback_T::owns(memory);
                }
            };

            template <std::size_t Threshold, class Allocator_LE, class Allocator_G>
            class Segregator_Allocator : public Allocator, private Allocator_LE, private Allocator_G
            {
                Segregator_Allocator(Allocator_LE &&allocator_le, Allocator_G &&allocator_G)
                    : Allocator_LE(allocator_le), Allocator_G(allocator_G)
                { }

            public:
                memory::Block allocate(std::size_t requested_size)
                {
                    if (requested_size <= Threshold)
                        return Allocator_LE::allocate(requested_size);
                    else
                        return Allocator_G::allocate(requested_size);
                }

                void deallocate(const memory::Block &memory)
                {
                    // TODO: Allocators should check if they own the memory
                    // before deallocating it.
                    // It should be OK to skip the check in higher-level allocators.

                    // if (memory.size <= Threshold)
                        Allocator_LE::deallocate(memory);
                    // else
                        Allocator_G::deallocate(memory);
                }

                void deallocate_all()
                {
                    Allocator_LE::deallocate_all();
                    Allocator_G::deallocate_all();
                }

                bool owns(const memory::Block &memory)
                {
                    return (memory.size <= Threshold) ? Allocator_LE::owns(memory) : Allocator_G::owns(memory);
                }
            };

            // TODO:
            // Affix_Allocator
            // Allocator_Stats
            // Bitmapped block
            // Cascading allocator
            // Bucketizer (make allocation sizes discrete with a specified step size)
            // Logarithmic Bucketizer
        }
    }
}



void pool_test_1()
{
    dd99::memory::Acquire_Stack_Block<1024> memory_block_1;
    dd99::block_allocator::Pool<512> pool(memory_block_1);

    auto x1 = pool.allocate(1);
    auto x2 = pool.allocate(1000);
    auto x3 = pool.allocate(1);
    auto x4 = pool.allocate(1);

    pool.deallocate(x2);
    auto x5 = pool.allocate(1);
    pool.deallocate(x3);
    auto x6 = pool.allocate(1);

    pool.deallocate_all();
    auto x7 = pool.allocate(1);
    auto x8 = pool.allocate(1);
    auto x9 = pool.allocate(1);
    auto x10 = pool.allocate(1);
}

void chop_test_1()
{
    dd99::memory::Acquire_Stack_Block<1024> memory_block_1;
    dd99::block_allocator::Free_List freelist(memory_block_1);

    auto x1 = freelist.allocate(1);
    auto x2 = freelist.allocate(100);
    auto x3 = freelist.allocate(500);
    auto x4 = freelist.allocate(400);
    auto x5 = freelist.allocate(100);
    auto x6 = freelist.allocate(100);
    freelist.deallocate(x2);
    auto x7 = freelist.allocate(50);
    auto x8 = freelist.allocate(50);
    auto x9 = freelist.allocate(50);
    auto x10 = freelist.allocate(1);
    auto x11 = freelist.allocate(1);
    auto x12 = freelist.allocate(1);
    auto x13 = freelist.allocate(1);
    auto x14 = freelist.allocate(1);
}

void chop_test_2()
{
    dd99::memory::Acquire_Stack_Block<1024> memory_block_1;
    dd99::block_allocator::Free_List freelist(memory_block_1);

    auto x1 = freelist.allocate(1);
    auto x2 = freelist.allocate(30);
    auto x3 = freelist.allocate(50);
    auto x4 = freelist.allocate(40);
    auto x5 = freelist.allocate(80);
    auto x6 = freelist.allocate(30);
    freelist.deallocate(x2);
    auto x7 = freelist.allocate(50);
    auto x8 = freelist.allocate(50);
    auto x9 = freelist.allocate(20);
    auto x10 = freelist.allocate(80);
    auto x11 = freelist.allocate(80);
    auto x12 = freelist.allocate(40);
    auto x13 = freelist.allocate(70);
    auto x14 = freelist.allocate(80);

    freelist.deallocate(x1);
    // freelist.deallocate(x2); // Already deallocated
    freelist.deallocate(x3);
    freelist.deallocate(x5);
    freelist.deallocate(x6);
    freelist.deallocate(x4);
    freelist.deallocate(x10);
    freelist.deallocate(x8);

    auto x15 = freelist.allocate(50);
    auto x16 = freelist.allocate(20);
    x1 = freelist.allocate(300);
    x2 = freelist.allocate(100);
    x3 = freelist.allocate(120);
    x4 = freelist.allocate(120);
    x5 = freelist.allocate(120);
    x6 = freelist.allocate(30);
}
 */