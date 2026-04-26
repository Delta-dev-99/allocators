#include <cstdint>
#include <algorithm>
#include <bit>
#include <stdexcept>

#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <chrono>
#include <atomic>
#include <sstream>

// ----------------------------------------------------------------------
// Core buddy allocation functions
// ----------------------------------------------------------------------

/**
 * Total number of buddy blocks (all levels) for N base blocks.
 * Levels L: sum_{i=0}^{L-1} floor(N / 2^i).
 * If L >= 64, terms beyond 63 are zero.
 */
inline unsigned __int128 total_blocks(std::uint64_t N, unsigned Levels) noexcept {
    // TODO: This overflows for large N and L. For testing, we can cap L at 64 since it won't contribute beyond that.
    // std::uint64_t sum = 0;
    // unsigned lim = std::min(Levels, 64u);
    // for (unsigned i = 0; i < lim; ++i)
    //     sum += N >> i;
    // return sum;

    unsigned __int128 sum = 0;
    unsigned lim = std::min(Levels, 64u);
    for (unsigned i = 0; i < lim; ++i)
        sum += (unsigned __int128)N >> i;
    return sum;
}

/**
 * Computes the maximum number of lowest‑level blocks that fit together with
 * the buddy bitmap into memory_size bytes.
 */
std::uint64_t max_usable_blocks(std::uint64_t memory_size,
                                std::uint64_t Block_Size,
                                std::uint64_t BMP_Block_Size,
                                unsigned Levels) {
    if (Block_Size == 0 || memory_size == 0)
        return 0;

    if (Levels == 0)
        return memory_size / Block_Size;

    const std::uint64_t K = 8ULL * BMP_Block_Size;   // bits per bitmap block
    const std::uint64_t max_N = memory_size / Block_Size;

    // Analytical initial guess from continuous relaxation

    std::uint64_t N;

    {
        std::uint64_t two_pow_Lm1 = 1ULL << (Levels - 1);          // 2^{L-1}
        std::uint64_t a_num = (1ULL << Levels) - 1;                // 2^L - 1
        std::uint64_t denom = 8ULL * Block_Size * two_pow_Lm1 + a_num;
        N = (8ULL * memory_size * two_pow_Lm1) / denom;
    }

    // ------ Safe initial guess using 128‑bit arithmetic ------
    // Only levels 0..63 contribute to the sum; cap L for the approximation.
    unsigned L = std::min(Levels, 64u);
    if (L == 0) L = 1;   // should not happen because Levels > 0 here
    std::uint64_t two_pow_Lm1 = 1ULL << (L - 1);
    std::uint64_t a_num = (1ULL << L) - 1;   // 2^L - 1
    // numerator   = 8 * memory_size * 2^{L-1}
    // denominator = 8 * Block_Size * 2^{L-1} + (2^L - 1)
    unsigned __int128 num = (unsigned __int128)8 * memory_size * two_pow_Lm1;
    unsigned __int128 den = (unsigned __int128)8 * Block_Size * two_pow_Lm1 + a_num;
    N = static_cast<std::uint64_t>(num / den);
    // ---------------------------------------------------------

    if (N > max_N) N = max_N;

    // Feasibility checker
    auto fits = [&](std::uint64_t n) -> bool {
        auto bits = total_blocks(n, Levels);
        auto bitmap = BMP_Block_Size * ((bits + K - 1) / K);
        return n * Block_Size + bitmap <= memory_size;
    };

    // Walk to the exact maximum (at most a few steps)
    if (fits(N)) {
        while (fits(N + 1))
            ++N;
    } else {
        do {
            --N;
        } while (!fits(N));
    }
    return N;
}

// ----------------------------------------------------------------------
// Reference implementation: binary search (exhaustive, fast)
// ----------------------------------------------------------------------

// std::uint64_t reference_Nmax(std::uint64_t memory_size,
//                              std::uint64_t Block_Size,
//                              std::uint64_t BMP_Block_Size,
//                              unsigned Levels) {
//     if (Block_Size == 0 || memory_size == 0) return 0;
//     if (Levels == 0) return memory_size / Block_Size;

//     const std::uint64_t K = 8ULL * BMP_Block_Size;
//     std::uint64_t lo = 0;
//     std::uint64_t hi = memory_size / Block_Size;
//     while (lo < hi) {
//         // auto mid = lo + (hi - lo + 1) / 2;   // upper mid to avoid infinite loop
//         auto mid = lo + ((hi - lo) >> 1) + ((hi - lo) & 1); // overflow-safe ceiling midpoint.
//         auto bits = total_blocks(mid, Levels);
//         std::uint64_t bitmap = BMP_Block_Size * ((bits + K - 1) / K);
//         if (mid * Block_Size + bitmap <= memory_size)
//             lo = mid;
//         else
//             hi = mid - 1;
//     }
//     return lo;
// }

std::uint64_t reference_Nmax(std::uint64_t memory_size,
                             std::uint64_t Block_Size,
                             std::uint64_t BMP_Block_Size,
                             unsigned Levels) {
    if (Block_Size == 0 || memory_size == 0) return 0;
    if (Levels == 0) return memory_size / Block_Size;

    const std::uint64_t K = 8ULL * BMP_Block_Size;
    std::uint64_t lo = 0;
    std::uint64_t hi = memory_size / Block_Size;

    auto fits = [&](std::uint64_t N) -> bool {
        unsigned __int128 bits = total_blocks(N, Levels);
        unsigned __int128 bitmap = BMP_Block_Size * ((bits + K - 1) / K);
        return (unsigned __int128)N * Block_Size + bitmap <= memory_size;
    };

    if (!fits(lo)) return 0;

    while (lo < hi) {
        // overflow‑safe ceiling midpoint
        std::uint64_t diff = hi - lo;
        std::uint64_t mid = lo + (diff >> 1) + (diff & 1);
        if (fits(mid))
            lo = mid;
        else
            hi = mid - 1;
    }
    return lo;
}

// ----------------------------------------------------------------------
// Parallel test harness
// ----------------------------------------------------------------------

struct TestCase {
    std::uint64_t M, S, B;
    unsigned L;
};

// Thread worker: runs a slice of random tests, compares, logs failures.
void run_test_slice(unsigned thread_id,
                    unsigned total_threads,
                    unsigned tests_per_thread,
                    std::mutex& cout_mutex,
                    std::vector<TestCase>& all_failures)   // append failures here
{
    // Per-thread random engine
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count()
                        + thread_id * 0x9e3779b9);
    std::uniform_int_distribution<unsigned> level_dist(1, 16);
    std::uniform_int_distribution<std::uint64_t> mem_dist(1ULL, 1ULL << 24); // up to ~16M
    std::uniform_int_distribution<std::uint64_t> block_dist(1, 256);
    std::uniform_int_distribution<std::uint64_t> bmp_dist(1, 128);

    std::vector<TestCase> local_failures;
    unsigned report_step = std::max(1u, tests_per_thread / 10); // 10 progress prints

    for (unsigned i = 0; i < tests_per_thread; ++i) {
        // Generate random parameters
        unsigned L = level_dist(rng);
        std::uint64_t M = mem_dist(rng);
        std::uint64_t S = block_dist(rng);
        std::uint64_t B = bmp_dist(rng);

        // Ensure M/S not too huge to keep test time bounded
        // (binary search reference is fast anyway)
        if (M / S > 2000000) continue;   // skip astronomically large loops

        std::uint64_t opt = max_usable_blocks(M, S, B, L);
        std::uint64_t ref = reference_Nmax(M, S, B, L);

        if (opt != ref) {
            local_failures.push_back({M, S, B, L});
        }

        // Print progress (thread‑safe)
        if (i % report_step == 0 || i == tests_per_thread - 1) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[Thread " << std::setw(2) << thread_id << "] "
                      << (i * 100 / tests_per_thread) << "% done"
                      << std::endl;
        }
    }

    // Append local failures to global list
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        all_failures.insert(all_failures.end(), local_failures.begin(), local_failures.end());
    }
}

int main() {
    // Detect concurrency
    unsigned concurrency = std::thread::hardware_concurrency();
    if (concurrency == 0) concurrency = 4;   // fallback
    std::cout << "Using " << concurrency << " threads.\n\n";

    // ------------------------------------------------------------------
    // 1. Run fast edge‑case tests (single‑threaded)
    // ------------------------------------------------------------------
    std::cout << "Running edge‑case tests...\n";
    const struct {
        std::uint64_t M, S, B; unsigned L;
    } edges[] = {
        {0, 1, 1, 1},
        {100, 1, 1, 0},
        {1024, 512, 64, 1},
        {1024, 1, 512, 5},
        {1ULL << 20, 4, 8, 10},
        {10000, 3, 7, 12},
        {1000000, 1, 1, 1},
        {100, 10000, 1, 1},
        {0xFFFFFFFFFFFFFFFF, 1, 1, 2},
        {0xFFFFFFFFFFFFFFFF, 1024*1024, 4, 12},
    };
    bool edge_ok = true;
    for (auto e : edges) {
        auto N1 = max_usable_blocks(e.M, e.S, e.B, e.L);
        auto N2 = reference_Nmax(e.M, e.S, e.B, e.L);
        if (N1 != N2) {
            std::cout << "FAIL edge: M=" << e.M << " S=" << e.S
                      << " B=" << e.B << " L=" << e.L
                      << " -> opt=" << N1 << " ref=" << N2 << '\n';
            edge_ok = false;
        }
        else {
            std::cout << "SUCCESS edge: M=" << e.M << " S=" << e.S
                      << " B=" << e.B << " L=" << e.L
                      << " -> opt=" << N1 << " ref=" << N2 << '\n';
        }
    }
    if (edge_ok) std::cout << "Edge cases passed.\n\n";

    // ------------------------------------------------------------------
    // 2. Random fuzz tests (parallel)
    // ------------------------------------------------------------------
    unsigned total_random_tests = 10000000;   // can be reduced for faster runs
    std::cout << "Starting " << total_random_tests << " random tests in parallel...\n";

    unsigned tests_per_thread = total_random_tests / concurrency;
    unsigned remainder = total_random_tests % concurrency;

    std::mutex cout_mutex;
    std::vector<TestCase> all_failures;

    std::vector<std::thread> threads;
    for (unsigned t = 0; t < concurrency; ++t) {
        unsigned my_tests = tests_per_thread + (t < remainder ? 1 : 0);
        threads.emplace_back(run_test_slice,
                             t, concurrency, my_tests,
                             std::ref(cout_mutex),
                             std::ref(all_failures));
    }

    // Join all threads
    for (auto& th : threads)
        th.join();

    // ------------------------------------------------------------------
    // 3. Stress test (single large case, only verify constraint)
    // ------------------------------------------------------------------
    std::cout << "\nRunning stress test (1 TiB, single‑threaded)...\n";
    {
        std::uint64_t M = 1ULL << 40;   // 1 TiB
        std::uint64_t S = 1;
        std::uint64_t B = 64;
        unsigned L = 20;
        auto N1 = max_usable_blocks(M, S, B, L);
        auto bits = total_blocks(N1, L);
        auto bmp = B * ((bits + 8*B - 1) / (8*B));
        bool ok = (N1 * S + bmp <= M);
        auto bits2 = total_blocks(N1+1, L);
        auto bmp2 = B * ((bits2 + 8*B - 1) / (8*B));
        ok = ok && ((N1+1) * S + bmp2 > M);
        std::cout << (ok ? "PASS" : "FAIL") << ": M=1TiB S=1 B=64 L=20 => N=" << N1 << '\n';
    }

    // ------------------------------------------------------------------
    // 4. Final report
    // ------------------------------------------------------------------
    std::cout << "\n========================================\n";
    if (all_failures.empty()) {
        std::cout << "All tests passed.\n";
    } else {
        std::cout << all_failures.size() << " random test(s) FAILED:\n";
        for (auto& f : all_failures) {
            std::cout << "  M=" << f.M << " S=" << f.S
                      << " B=" << f.B << " L=" << f.L << '\n';
        }
    }
    return all_failures.empty() ? 0 : 1;
}
