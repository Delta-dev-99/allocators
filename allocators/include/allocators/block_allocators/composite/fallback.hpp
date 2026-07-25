#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99_allocators_namespace::block_allocator::composite
{
    // Tries to allocate from the primary allocator.
    // Tries to allocate from a fallback allocator if primary fails.
    // And so on...
    template <class Primary_T, class Fallback_T, class... More_Fallbacks_T>
    class Fallback
        : public Fallback<Fallback<Primary_T, Fallback_T>, More_Fallbacks_T...>
    {
        using Base = Fallback<Fallback<Primary_T, Fallback_T>, More_Fallbacks_T...>;

    public:
        Fallback(Primary_T &&primary, Fallback_T &&fallback, More_Fallbacks_T &&... more_fallbacks)
            : Base(
                Fallback<Primary_T, Fallback_T>(
                    std::move(primary),
                    std::move(fallback)),
                std::move(more_fallbacks)...)
        { }
    };


    // Specialization for end case of recursion
    template <class Primary_T, class Fallback_T>
    class Fallback<Primary_T, Fallback_T>
    {
    public:
        Fallback(Primary_T primary, Fallback_T fallback)
            : m_primary(std::forward<Primary_T>(primary))
            , m_fallback(std::forward<Fallback_T>(fallback))
        { }

        Fallback(const Fallback&) = delete;
        Fallback(Fallback&&) = default;
        Fallback & operator=(const Fallback &) = delete;
        Fallback & operator=(Fallback &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size,
                               std::size_t requested_alignment = 1)
        {
            auto r = m_primary.allocate(requested_size, requested_alignment);
            if (!r)
                r = m_fallback.allocate(requested_size, requested_alignment);
            return r;
        }

        void deallocate(const block &memory)
        {
            // TODO: Allocators should check if they own the memory
            // before deallocating it.
            // It should be OK to skip the check in higher-level allocators.

            // TODO: we are changing the ownership check use practices
            // ownership on deallocation should be an assertion
            DD99_ALLOCATORS_ASSERT_HARDENED("block must be owned by this allocator", owns(memory));


            if (m_primary.owns(memory))
                m_primary.deallocate(memory);
            // else if (m_fallback.owns(memory))
            else
                m_fallback.deallocate(memory);
        }

        void deallocate_all()
        {
            m_primary.deallocate_all();
            m_fallback.deallocate_all();
        }

        bool owns(const std::byte * memory) const
        {
            return m_primary.owns(memory) || m_fallback.owns(memory);
        }

        bool owns(const block &memory) const
        {
            return m_primary.owns(memory) || m_fallback.owns(memory);
        }
    
    public:
        Primary_T m_primary;
        Fallback_T m_fallback;
    };

    static_assert(Block_Allocator<Fallback<void*, void*>>, "This definition doesn't comply with the `Block_Allocator` concept");
    
}
