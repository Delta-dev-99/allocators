#pragma once

#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>


namespace dd99::memory::block_allocator::buddy_namespace
{

    // This implementation uses user-provided blocks to store a bitmap and one freelist per level
    // The user is responsible for providing suitably sized and aligned blocks
    // Static functions for calculating required sizes and alignments are provided.
    template <Layout_Concept Layout>
    struct buddy_external_state
    {

    };

}
