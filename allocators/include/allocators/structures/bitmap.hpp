#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <bit>


namespace dd99_allocators_namespace::structure
{
    
    // A bitmap type that doesn't own the underlying storage
    // On construction, it is given a pointer to the base of the storage, and a number of bits.
    // Bits are grouped in blocks of a specified type (template argument)
    template <class Block_T = std::uint8_t>
    class Bitmap
    {
    public:
        using block_type = Block_T;

        // TODO: Verify block_bits is used in calculations
        constexpr static auto block_size = sizeof(block_type);
        constexpr static auto block_bits = block_size * std::numeric_limits<unsigned char>::digits;
        constexpr static auto block_alignment = alignof(block_type);

    public: // statics
        // All blocks are fully used?
        constexpr static bool fully_mapped(std::size_t bit_count)
        {
            return bit_count % block_bits == 0;
        }

        // The number of Bitmap Blocks used
        constexpr static std::size_t calculate_block_count(std::size_t bit_count)
        {
            return  (bit_count + block_bits - 1) / block_bits;
        }
        
    public:
        // Create the bitmap in-place
        Bitmap(std::size_t bit_count, std::byte * base)
            : m_base(reinterpret_cast<block_type *>(base))
            , m_bit_size(bit_count)
            , m_size(Bitmap::calculate_block_count(bit_count))
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("memory must be appropriately aligned", is_aligned(m_base, block_alignment));
            
            reset();
        }

    public:
        bool fully_mapped() const
        {
            return fully_mapped(m_bit_size);
        }

        // size in blocks
        std::size_t size() const
        {
            return m_size;
        }

        std::size_t bit_size() const
        {
            return m_bit_size;
        }



        void set(std::size_t bit_index)
        {
            m_base[bit_index / block_bits] |= (block_type(1) << (bit_index % block_bits));
        }

        void unset(std::size_t bit_index)
        {
            m_base[bit_index / block_bits] &= ~(block_type(1) << (bit_index % block_bits));
        }

        bool toggle(std::size_t bit_index)
        {
            const auto blk_index = bit_index / block_bits;
            const auto bit_index_in_blk = bit_index % block_bits;
            const auto bit_mask = static_cast<block_type>(block_type{1} << bit_index_in_blk);

            m_base[blk_index] ^= bit_mask;

            // return the new value
            return bool(m_base[blk_index] & bit_mask);
        }

        // returns the index of the set bit, or -1 if not found
        std::size_t set_first_unset()
        {
            // Scan through blocks looking for one that's not fully set (all 1s)
            // Avoid strict aliasing violations by staying within block_type pointer type
            // Modern compilers should optimize this effectively through vectorization and loop unrolling
            auto block_ptr = m_base;
            const auto block_ptr_end = m_base + size();
            
            while (block_ptr < block_ptr_end)
            {
                if (*block_ptr != block_type(-1))
                {
                    // Found a block with at least one unset bit
                    unsigned bit = std::countr_zero(static_cast<block_type>(~*block_ptr)); // first 0 bit
                    *block_ptr |= (block_type{1} << bit);
                    return static_cast<std::size_t>(block_ptr - m_base) * block_bits + bit;

                    // for (unsigned bit = 0; bit < block_bits; ++bit)
                    // {
                    //     const auto mask = block_type(1) << bit;

                    //     if ((*block_ptr & mask) == 0)
                    //     {
                    //         *block_ptr |= mask;
                    //         return std::size_t(block_ptr - m_base) * block_bits + bit;
                    //     }
                    // }
                }

                ++block_ptr;
            }

            return std::size_t(-1);
        }

        // clear all mapped bits, set all unmapped bits
        void reset()
        {
            // Use memset to clear memory - avoids strict aliasing violations
            // while maintaining performance through compiler optimizations
            std::memset(m_base, 0, size() * block_size);

            // mark unmapped bits as used
            // last block only
            if (!fully_mapped())
            {
                const auto n_used_bits = bit_size() % block_bits;
                const auto n_unused_bits = block_bits - n_used_bits;

                m_base[size() - 1] |= static_cast<block_type>(((std::make_unsigned_t<block_type>(1) << n_unused_bits) - 1) << n_used_bits);
            }
        }

        // get the value of a specific bit
        bool operator[](std::size_t index) const
        {
            const auto block_index = index / block_bits;
            const auto bit_index = index % block_bits;
            return static_cast<bool>(m_base[block_index] & (block_type(1) << bit_index));
        }

    private:
        block_type * m_base;
        std::size_t  m_bit_size;
        std::size_t  m_size;
    };
}
