#pragma once


namespace dd99::memory::storage
{
    
    // instructs allocators to place their bookkeeping data structures alongside their managed memory
    struct storage_policy_inline {};

}