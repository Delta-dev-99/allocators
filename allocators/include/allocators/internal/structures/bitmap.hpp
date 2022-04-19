#pragma once

#include <cstdint>

namespace dd99::memory::structure
{
    template <class Block_T = std::uint8_t>
    class Bitmap
    {
    public:
        // TODO: Verify Block_Bits is used in calculations
        constexpr static auto Block_Size = sizeof(Block_T);
        constexpr static auto Block_Bits = Block_Size * CHAR_BIT;

    public: // statics
        // All blocks are fully used?
        constexpr static bool fully_mapped(std::size_t bit_count)
        {
            return bit_count % Block_Bits == 0;
        }

        // The number of Bitmap Blocks used
        constexpr static std::size_t calculate_block_count(std::size_t bit_count)
        {
            if (bit_count == 0) return 0;
            return  (bit_count - 1) / Block_Bits + 1;
        }
        
    public:
        // Create the bitmap in-place
        Bitmap(std::size_t bit_count, std::byte *base)
            : m_base(reinterpret_cast<Block_T *>(base))
            , m_bit_size(bit_count)
            , m_size(Bitmap::calculate_block_count(bit_count))
        {
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
            m_base[bit_index / Block_Bits] |= (Block_T(1) << (bit_index % Block_Bits));
        }

        void unset(std::size_t bit_index)
        {
            m_base[bit_index / Block_Bits] &= ~(Block_T(1) << (bit_index % Block_Bits));
        }

        bool toggle(std::size_t bit_index)
        {
            const auto blk_index = bit_index / Block_Bits;
            const auto bit_index_in_blk = bit_index % Block_Bits;
            const auto bit_mask = Block_T(1) << bit_index_in_blk;

            m_base[blk_index] ^= bit_mask;

            // return the new value
            return bool(m_base[blk_index] & bit_mask);
        }

        // returns the index of the set bit, or -1 if not found
        std::size_t set_first_unset()
        {
            // optimization using larger type for iteration
            using Fast_Block = std::uint_fast32_t;
            const auto Fast_Block_Bits = sizeof(Fast_Block) * CHAR_BIT;
            const auto n_fast_blocks = bit_size() / Fast_Block_Bits;
            /* volatile */ auto fast_block_ptr = reinterpret_cast<Fast_Block *>(m_base);
            const auto fast_block_ptr_end = reinterpret_cast<Fast_Block *>(m_base) + n_fast_blocks;
            while (fast_block_ptr < fast_block_ptr_end)
            {
                if (*fast_block_ptr != Fast_Block(-1))
                    break;

                ++fast_block_ptr;
            }

            auto block_ptr = reinterpret_cast<Block_T *>(fast_block_ptr);
            const auto block_ptr_end = m_base + size();
            while (block_ptr < block_ptr_end)
            {
                if (*block_ptr != Block_T(-1))
                {
                    for (unsigned bit = 0; bit < Block_Bits; ++bit)
                    {
                        const auto mask = Block_T(1) << bit;

                        if ((*block_ptr & mask) == 0)
                        {
                            *block_ptr |= mask;
                            return std::size_t(block_ptr - m_base) * Block_Bits + bit;
                        }
                    }
                }

                ++block_ptr;
            }

            return std::size_t(-1);
        }

        // clear all mapped bits, set all unmapped bits
        void reset()
        {
            // optimization using larger type for iteration
            using Fast_Block = std::uint_fast32_t;
            const auto Fast_Block_Bits = sizeof(Fast_Block) * CHAR_BIT;
            const auto n_fast_blocks = bit_size() / Fast_Block_Bits;
            /* volatile */ auto fast_block_ptr = reinterpret_cast<Fast_Block *>(m_base);
            const auto fast_block_ptr_end = reinterpret_cast<Fast_Block *>(m_base) + n_fast_blocks;
            while (fast_block_ptr < fast_block_ptr_end)
            {
                *fast_block_ptr++ = 0;
            }

            // clear the rest of blocks
            /* volatile */ auto block_ptr = reinterpret_cast<Block_T *>(fast_block_ptr);
            const auto block_ptr_end = m_base + size();
            while (block_ptr < block_ptr_end)
            {
                *block_ptr++ = Block_T{0};
            }

            // mark unmapped bits as used
            // last block only
            if (!fully_mapped())
            {
                const auto n_used_bits = bit_size() % Block_Bits;
                const auto n_unused_bits = Block_Bits - n_used_bits;

                m_base[size() - 1] |= Block_T((std::make_unsigned_t<Block_T>(1) << n_unused_bits) - 1) << n_used_bits;
            }
        }

        // get the value of a specific bit
        bool operator[](std::size_t index) const
        {
            return m_base[index / Block_Bits] & (Block_T(1) << index % Block_Bits);
        }

    private:
        Block_T *m_base;
        std::size_t m_bit_size;
        std::size_t m_size;
    };
}
