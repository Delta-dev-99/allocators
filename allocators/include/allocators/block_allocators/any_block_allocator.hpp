#pragma once

#include <allocators/structures/blocks/memory_block.hpp>
#include <allocators/block_allocators/block_allocator.hpp>
#include <cstddef>


namespace dd99::memory::block_allocator
{
    namespace detail
    {
        struct block_allocator_vtable
        {
            block   (* allocate)        (void *, std::size_t, std::size_t);
            void    (* deallocate)      (void *, const block &);
            void    (* deallocate_all)  (void *);
            bool    (* owns_block)      (void *, const block &);
            bool    (* owns_pointer)    (void *, const std::byte *);
        };

        template <Block_Allocator Allocator>
        constexpr block_allocator_vtable vtable_for = block_allocator_vtable
        {
            .allocate       = [](void * alloc_ptr, std::size_t size, std::size_t alignment) -> block    { return reinterpret_cast<Allocator *>(alloc_ptr)->allocate(size, alignment);   },
            .deallocate     = [](void * alloc_ptr, const block & blk)                       -> void     { return reinterpret_cast<Allocator *>(alloc_ptr)->deallocate(blk);             },
            .deallocate_all = [](void * alloc_ptr)                                          -> void     { return reinterpret_cast<Allocator *>(alloc_ptr)->deallocate_all();            },
            .owns_block     = [](void * alloc_ptr, const block & blk)                       -> bool     { return reinterpret_cast<Allocator *>(alloc_ptr)->owns(blk);                   },
            .owns_pointer   = [](void * alloc_ptr, const std::byte * blk_ptr)               -> bool     { return reinterpret_cast<Allocator *>(alloc_ptr)->owns(blk_ptr);               },
        };
    }

    struct any_block_allocator_ref
    {
        const detail::block_allocator_vtable * m_vptr;
        void * m_allocator_ptr;

        template <Block_Allocator Allocator>
        constexpr any_block_allocator_ref(Allocator & allocator)
            : m_vptr{& detail::vtable_for<Allocator>}
            , m_allocator_ptr{& allocator}
        { }

        block   allocate        (std::size_t size, std::size_t alignment = 1)   { return m_vptr->allocate(m_allocator_ptr, size, alignment);    }
        void    deallocate      (const block & blk)                             { return m_vptr->deallocate(m_allocator_ptr, blk);              }
        void    deallocate_all  ()                                              { return m_vptr->deallocate_all(m_allocator_ptr);               }
        bool    owns            (const block & blk)                             { return m_vptr->owns_block(m_allocator_ptr, blk);              }
        bool    owns            (const std::byte * blk_ptr)                     { return m_vptr->owns_pointer(m_allocator_ptr, blk_ptr);        }
    };

    static_assert(Block_Allocator<any_block_allocator_ref>);

}
