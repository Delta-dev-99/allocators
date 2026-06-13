#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99::memory::block_allocator::composite
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
        Fallback(Primary_T &&primary, Fallback_T &&fallback)
            : m_primary(std::move(primary))
            , m_fallback(std::move(fallback))
        { }

        Fallback(const Fallback&) = delete;
        Fallback(Fallback&&) = default;

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size,
                               std::size_t requested_alignment = 1)
        {
            auto r = m_primary.allocate(requested_size, requested_alignment);
            if (!r)
                r = m_fallback.allocate(requested_size, requested_alignment);
            return r;
        }

        void deallocate(const memory::block &memory)
        {
            // TODO: Allocators should check if they own the memory
            // before deallocating it.
            // It should be OK to skip the check in higher-level allocators.

            // if (Primary_T::owns(memory))
                m_primary.deallocate(memory);
            // else if (Fallback_T::owns(memory))
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

        bool owns(const memory::block &memory) const
        {
            return m_primary.owns(memory) || m_fallback.owns(memory);
        }
    
    private:
        Primary_T m_primary;
        Fallback_T m_fallback;
    };

    static_assert(Block_Allocator<Fallback<void*, void*>>, "This definition doesn't comply with the `Block_Allocator` concept");
    
}
