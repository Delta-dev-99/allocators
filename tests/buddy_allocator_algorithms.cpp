#include <cstdint>
#include <algorithm>
#include <bit>          // std::popcount (optional, not used below)
#include <stdexcept>

/**
 * Counts the total number of buddy blocks (all levels) for N base blocks.
 * Levels L: we sum floor(N / 2^i) for i = 0 .. L-1.
 * If L >= 64, terms beyond 63 are zero because N < 2^64.
 */
inline std::uint64_t total_blocks(std::uint64_t N, unsigned Levels) noexcept {
    std::uint64_t sum = 0;
    unsigned lim = std::min(Levels, 64u);
    for (unsigned i = 0; i < lim; ++i)
        sum += N >> i;
    return sum;
}

/**
 * Computes the maximum number of lowest‑level blocks (each of Block_Size bytes)
 * that fit together with the buddy bitmap into memory_size bytes.
 *
 * Parameters:
 *   memory_size    - total available memory (bytes)
 *   Block_Size     - size of one lowest‑level block (bytes)
 *   BMP_Block_Size - bitmap size must be a multiple of this (bytes)
 *   Levels         - number of levels in the buddy tree (≥ 0)
 *
 * Returns the largest N ≥ 0 satisfying:
 *   N * Block_Size + bitmap_bytes(N) ≤ memory_size
 * where bitmap_bytes(N) = BMP_Block_Size * ceil( total_blocks(N) / (8*BMP_Block_Size) )
 *
 * Special case: Levels == 0 → no bitmap, returns memory_size / Block_Size.
 */
std::uint64_t max_usable_blocks(std::uint64_t memory_size,
                                std::uint64_t Block_Size,
                                std::uint64_t BMP_Block_Size,
                                unsigned Levels) {
    // Trivial cases
    if (Block_Size == 0 || memory_size == 0)
        return 0;

    if (Levels == 0)
        return memory_size / Block_Size;

    const std::uint64_t K = 8ULL * BMP_Block_Size;   // bits per bitmap block
    const std::uint64_t max_N = memory_size / Block_Size;

    // ---------- Analytical initial guess ----------
    // Continuous relaxation: bitmap ≈ (a*N) bits, a = sum 1/2^i = 2 - 1/2^{L-1}
    // Solve: N*Block_Size + (a*N)/8 = memory_size  →  N = 8*memory_size / (8*Block_Size + a)
    // a = (2^L - 1) / 2^{L-1}   (rational)
    std::uint64_t two_pow_Lm1 = 1ULL << (Levels - 1);          // 2^{L-1}
    std::uint64_t a_num = (1ULL << Levels) - 1;                // 2^L - 1
    std::uint64_t denom = 8ULL * Block_Size * two_pow_Lm1 + a_num;
    std::uint64_t N = (8ULL * memory_size * two_pow_Lm1) / denom;

    // Clamp to possible maximum
    if (N > max_N) N = max_N;

    // ---------- Exact adjustment (at most a few steps) ----------
    auto fits = [&](std::uint64_t n) -> bool {
        std::uint64_t bits = total_blocks(n, Levels);
        std::uint64_t bitmap = BMP_Block_Size * ((bits + K - 1) / K);
        return n * Block_Size + bitmap <= memory_size;
    };

    // Find the true maximum
    if (fits(N)) {
        // N already works, maybe we can go higher
        while (fits(N + 1))
            ++N;
    } else {
        // N too large, walk down
        do {
            --N;
        } while (!fits(N));
    }
    return N;
}




#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cassert>

// Include the function above (or link)

namespace {
    // Brute-force reference: finds max N by scanning all candidates.
    std::uint64_t brute_force_Nmax(std::uint64_t memory_size,
                                   std::uint64_t Block_Size,
                                   std::uint64_t BMP_Block_Size,
                                   unsigned Levels) {
        if (Block_Size == 0 || memory_size == 0) return 0;
        if (Levels == 0) return memory_size / Block_Size;
        const std::uint64_t K = 8ULL * BMP_Block_Size;
        std::uint64_t best = 0;
        for (std::uint64_t N = 0; N <= memory_size / Block_Size; ++N) {
            std::uint64_t bits = total_blocks(N, Levels);
            std::uint64_t bitmap = BMP_Block_Size * ((bits + K - 1) / K);
            if (N * Block_Size + bitmap <= memory_size)
                best = N;
            else
                break;  // once it fails, larger N will also fail (monotonic)
        }
        return best;
    }

    // Simple random generator (not cryptographically secure)
    std::uint64_t rand_range(std::uint64_t lo, std::uint64_t hi) {
        return lo + (std::rand() % (hi - lo + 1));
    }
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    unsigned num_tests = 100000;
    unsigned failed = 0;

    // Test edge cases explicitly
    const struct {
        std::uint64_t M, S, B; unsigned L;
    } edges[] = {
        {0, 1, 1, 1},                // zero memory
        {100, 1, 1, 0},              // zero levels
        {1024, 512, 64, 1},           // large block, single level
        {1024, 1, 512, 5},            // large bitmap granularity
        {1ULL << 20, 4, 8, 10},       // typical
        {10000, 3, 7, 12},            // odd numbers
        {1000000, 1, 1, 1},           // flat allocator
        {100, 10000, 1, 1},           // block size > memory
        {0xFFFFFFFFFFFFFFFF, 1, 1, 2},// near max 64-bit memory
    };

    std::cout << "Running edge cases...\n";
    for (auto e : edges) {
        auto N1 = max_usable_blocks(e.M, e.S, e.B, e.L);
        auto N2 = brute_force_Nmax(e.M, e.S, e.B, e.L);
        if (N1 != N2) {
            std::cout << "Edge FAIL: M=" << e.M << " S=" << e.S
                      << " B=" << e.B << " L=" << e.L
                      << " => opt=" << N1 << " brute=" << N2 << '\n';
            ++failed;
        }
    }

    // Random fuzzing
    std::cout << "Running random tests...\n";
    for (unsigned i = 0; i < num_tests; ++i) {
        unsigned Levels = rand_range(1, 16);          // reasonable buddy depth
        std::uint64_t M = rand_range(1, 1ULL << (rand_range(10, 30))); // up to 1 Gi bits?
        std::uint64_t S = rand_range(1, 256);
        std::uint64_t B = rand_range(1, 128);

        // Ensure S and B not zero
        if (S == 0) S = 1;
        if (B == 0) B = 1;

        std::uint64_t N1 = max_usable_blocks(M, S, B, Levels);
        std::uint64_t N2 = brute_force_Nmax(M, S, B, Levels);

        if (N1 != N2) {
            std::cout << "Random FAIL: M=" << M << " S=" << S
                      << " B=" << B << " L=" << Levels
                      << " => opt=" << N1 << " brute=" << N2 << '\n';
            if (++failed > 20) {
                std::cout << "Too many failures, aborting.\n";
                return 1;
            }
        }
    }

    // Specific stress: very large memory / small block, many levels
    std::cout << "Running stress tests...\n";
    {
        std::uint64_t M = 1ULL << 40;   // 1 TiB
        std::uint64_t S = 1;
        std::uint64_t B = 64;
        unsigned L = 20;
        auto N1 = max_usable_blocks(M, S, B, L);
        // brute force impossible; just check that constraint holds & N+1 fails
        auto bits = total_blocks(N1, L);
        auto bmp = B * ((bits + 8*B - 1) / (8*B));
        assert(N1 * S + bmp <= M);
        auto bits2 = total_blocks(N1+1, L);
        auto bmp2 = B * ((bits2 + 8*B - 1) / (8*B));
        assert((N1+1) * S + bmp2 > M);
        std::cout << "Stress: 1TiB, S=1, B=64, L=20 => N=" << N1 << " (validated)\n";
    }

    if (failed == 0)
        std::cout << "All tests passed.\n";
    else
        std::cout << failed << " test(s) failed.\n";
    return failed ? 1 : 0;
}