
## 1. The buddy model

For $N$ lowest‑level blocks (size $S$), the total buddy bits needed is  

\[
T(N) = \sum_{i=1}^{L-1} \left\lfloor \frac{N}{2^i} \right\rfloor .
\]

This sum is **always** $< N \le 2^{64}-1$ – plain `uint64_t` is safe.  

The bitmap bytes are $B = B_{\text{BS}} \cdot \lceil T(N) / (8B_{\text{BS}}) \rceil$.  
The constraint: $N \cdot S + B \le M$ (memory size).

---

## 2. Analytical continuous approximation

Define  

$$
a = \sum_{i=1}^{L-1} \frac{1}{2^i} = 1 - \frac{1}{2^{L-1}} = \frac{2^{L-1} - 1}{2^{L-1}} .
$$

Ignoring floors and ceilings, we solve:

$$
N \left(S + \frac{a}{8}\right) \le M .
$$

Hence the (continuous) maximum:

$$
N_0 = \left\lfloor \frac{M}{S + a/8} \right\rfloor
     = \left\lfloor \frac{8M \cdot 2^{L-1}}{8S \cdot 2^{L-1} + 2^{L-1} - 1} \right\rfloor .
$$

This is a **closed form** that can be computed with 64‑bit arithmetic **if** the intermediate products don’t overflow. The numerator is $8M \cdot 2^{L-1}$. For $M$ up to $2^{64}-1$ and $L$ up to 64, this could overflow. However, in a real kernel, physical RAM sizes are many orders of magnitude smaller than $2^{64}$ bytes. Even for a future 64‑bit address space, the actual memory size will be at most a few terabytes. If you can guarantee $M \cdot 8 \cdot 2^{\min(L-1, 40)} < 2^{64}$ (i.e., bounded memory), then no overflow occurs. For fully safe code, you could use `__int128` (if available) or just cap the exponent.

**But here’s the key:** The error between $N_0$ and the true $N_{\max}$ is bounded by **$\lceil B_{\text{BS}} / S \rceil$** blocks (as earlier, but now with the smaller bitmap).  
That bound is typically **single‑digit** (e.g., $B_{\text{BS}}=64$, $S=16$ → 4). So a simple linear scan from $N_0$ downward/upward will finish in at most a few iterations.

---

## 3. Proposed implementation (hybrid: guess + tiny loop)

```cpp
static constexpr std::size_t calculate_block_count(std::size_t memory_size) {
    if (memory_size < BMP::Block_Size + Block_Size)
        return 0;

    constexpr std::size_t K = 8ULL * BMP::Block_Size;

    // Analytical initial guess (safe for limited memory sizes)
    constexpr auto pow2_Lm1 = 1ULL << (Levels - 1);
    constexpr auto a_num = pow2_Lm1 - 1;          // 2^{L-1} - 1
    constexpr auto denom = 8ULL * Block_Size * pow2_Lm1 + a_num;
    // Numerator: 8 * memory_size * pow2_Lm1
    // We assume memory_size * 8 * pow2_Lm1 < 2^64 (holds for realistic RAM).
    std::size_t N = (8ULL * memory_size * pow2_Lm1) / denom;

    // Helper
    auto buddy_bits = [](std::size_t n) {
        std::size_t sum = 0;
        for (unsigned i = 1; i < Levels; ++i)
            sum += n >> i;
        return sum;
    };

    auto fits = [&](std::size_t n) {
        std::size_t bits = buddy_bits(n);
        std::size_t bitmap = BMP::Block_Size * ((bits + K - 1) / K);
        return n * Block_Size + bitmap <= memory_size;
    };

    // Adjust – at most ceil(BMP_Block_Size / Block_Size) steps
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
```

**Why this is correct and safe:**

- The initial guess uses only 64‑bit arithmetic **under the assumption** that the combined product doesn’t exceed $2^{64}$. You can either assert this at compile time or let the compiler’s `__int128` handle it if you prefer.
- The adjustment loop runs at most $\lceil B_{\text{BS}} / S \rceil$ times – a constant **bounded by a small number** for any sensible block/size combination (e.g., 64/1 → 64 steps, still trivial).
- No binary search needed; the loop is effectively O(1) with a very tight bound.

---

## 4. Formal proof sketch

1. **Continuous model:** $U_c(N) = N(S + a/8)$. The rounding differences are bounded by one bitmap block (±$B_{\text{BS}}$) plus less than one byte from the floor sum.  
2. **Error bound:** $|U(N) - U_c(N)| \le B_{\text{BS}}$.  
3. **Consequence:** If $U_c(N_0) \le M$ then $U(N_0) \le M + B_{\text{BS}}$. Similarly, if $U_c(N_0+1) > M$ then $U(N_0+1) \ge M - B_{\text{BS}}$. Hence the true $N_{\max}$ lies within $\pm \lceil B_{\text{BS}}/S \rceil$ of $N_0$.  
4. **Monotonicity:** $U(N)$ is strictly increasing, so a linear scan within that window finds the exact maximum.

---

## Conclusion

I still think the pure binary search is the path of least resistance, with nearly zero overhead. But your point about not wasting CPU work is technically valid. The hybrid method above gives you:

- **O(1) closed‑form start** (fast, elegant)
- **Bounded tiny loop** (guaranteed to finish in a handful of iterations)
- **Exact correct answer** with a simple correctness proof

If you’re comfortable maintaining the proof and the overflow assumption holds (or you guard it), go with the hybrid. If you prefer absolute robustness with zero corner cases, stick with the binary search. Either way, the allocator will be correct and fast.