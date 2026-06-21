#include <cstddef>
#include <type_traits>

// Helper to check a single type
template <typename T>
constexpr bool size_is_multiple_of_alignment() {
    return sizeof(T) % alignof(T) == 0;
}

// --- Fundamental types ---
static_assert(size_is_multiple_of_alignment<int>());
static_assert(size_is_multiple_of_alignment<double>());
static_assert(size_is_multiple_of_alignment<char>());
static_assert(size_is_multiple_of_alignment<void*>());

// --- Struct with natural alignment ---
struct S1 { int a; char b; };            // likely 8 bytes, align 4
static_assert(size_is_multiple_of_alignment<S1>());

// --- Struct with padding to force size multiple of alignment ---
struct S2 { char a; double b; };          // size 16, align 8
static_assert(size_is_multiple_of_alignment<S2>());

// --- Over‑aligned types (C++11) ---
struct alignas(64) OverAligned { char c; };
static_assert(size_is_multiple_of_alignment<OverAligned>());
// sizeof(OverAligned) must be at least 64, and a multiple of 64.

struct alignas(128) BigAligned { int x; };
static_assert(size_is_multiple_of_alignment<BigAligned>());

// --- Over‑aligned with member forcing padding to alignment multiple ---
struct alignas(32) Mixed { char a; int b; };  // size likely 32
static_assert(size_is_multiple_of_alignment<Mixed>());

// --- Array types (complete object types) ---
static_assert(size_is_multiple_of_alignment<int[5]>());
static_assert(size_is_multiple_of_alignment<S2[10]>());
static_assert(size_is_multiple_of_alignment<OverAligned[3]>());

// --- Class with virtual functions ---
struct Polymorphic { virtual ~Polymorphic() = default; int a; };
static_assert(size_is_multiple_of_alignment<Polymorphic>());

// --- Empty base optimization (unlikely to break, but test anyway) ---
struct Empty {};
struct Derived : Empty { int x; };
static_assert(size_is_multiple_of_alignment<Derived>());

// --- Enum ---
enum class E : unsigned short { a, b, c };
static_assert(size_is_multiple_of_alignment<E>());

// --- std::max_align_t (the largest fundamental alignment) ---
static_assert(size_is_multiple_of_alignment<std::max_align_t>());

int main() {
    // All checks are compile-time, so main is empty.
    return 0;
}
