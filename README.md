
# `allocators` by DD99

This is a header-only library that provides composable allocators.
A full blown alternative to the usual allocation schemes.

The goal of this library is to provide the required building blocks and flexibility for the construction of a complete memory management system. It can be used alongside standard memory management systems or standalone.

## The design and interface

Allocator instances are non-fungible and non-copyable.
They have exclusive control over a memory region, from which they dish-out memory.
    
    This avoids problems where different instances (which store bookkeeping info inside themselves) allocate the same memory. By allowing only one instance to control a memory region, we eliminate the need for a global control mechanism (which would probably need allocation).

An allocation request consists of the ammount of bytes required.
The response is a memory block (pointer and size), which may be empty (size = 0) if the user requested 0 bytes.
If the allocation fails, the memory block is null (pointer = nullptr, size = 0).

On deallocation the user must give the allocator the same memory block he received on allocation (unless otherwise noted, e.g. partial deallocation).

## A comparisson with the usual allocation system

The major difference with usual allocation systems is the 2-value paradigm. Each allocation returns a memory block, that is, a structure consisting of a pointer and a size. For deallocation, that same memory block value must be used. This eliminates the need to store allocation size or other bookkeeping information in allocation headers at the edge of the memory being allocated.

This design puts yet another responsibility on user code: to keep the memory block information and pass it on deallocation.
But we consider that the complexity and error propension of manually handling memory is not considerably increased.
A buffer overrun could overwrite the memory header in the usual system, just as easy as it could affect a local variable.
The memory that is taken by this bookkeeping info in user code is almost the same than the memory taken by the allocation header in the usual system.
Lifetime issues are the same as with the pointer approach.
Performance may be better (not tested), considering that there is no need to look-up the allocation header.

The major gain of this design is that now the user has a way to know how much memory was actually allocated. This gives the user and the allocation system as a whole an extra degree of freedom.
Fixed size allocators tend to be faster than their conterpart but can only allocate certain memory sizes; to use them, the user must have a way to know how much memory was actually allocated.
Having control over allocation size could also allow an allocation to be partially deallocated if the allocator supports it (TODO).


# Library organization

Allocators are organized in folders, according to their interface.
        
## Basic allocators `<allocators/basic/>`

Basic allocators are the main allocator category.
They take a memory block on construction and allocate memory from there.

In this category we have:

- `Bitmap`: bitmapped, fixed-size.
- `Buddy`: binary buddy, discrete-size
- `Pool`: freelist, fixed-size
- `Slicing`: freelist, request-sized
- `Stack`: simple stack, request-sized

more information on the individual allocators later in this file.

## Borrowing allocators `<allocators/borrowing/>`

Some allocators can be implemented more efficiently by allowing it to allocate memory for internal state (without using the given memory block for that).
This allocators are said to borrow memory from other allocator (hence borrowing allocators).

Borrowing allocators are templated on the auxiliary allocator type and take an allocator on construction.
The allocator instance ownership is taken too, to avoid lifetime issues. If taking a reference is intended, the composite `ref` allocator can be used.

In this category we have:

- `Bitmap`: bitmapped, fixed-size, borrowing
- `Buddy`: binary buddy, discrete-size, borrowing

## Composite allocators `<allocators/composite/>`

The classes that provide composition and probably the main attraction of the library.
Composite allocators are allocators implemented based entirely on other allocators. They provide special behavior or characteristics.

Composite allocators are templated on one or more allocator types and take instances (and usually ownership) of those on construction. The exception is the `ref` allocator which, by design, does not take ownership of the underlying allocator.

In this category we have:

- `Fallback`: if an allocator cannot fullfil a request, try the next one
- `Filter`: only allocates if a predicate on the request size is true
- `Quantizer`: forces discrete-sized allocation with some step size
- `Ref`: a proxy to another allocator instance through a reference
- `Segregator`: determines which allocator to use based on an allocation size threshold
- `Throwing`: throws when allocation fails (and optionally on deallocation too)

## Degenerate allocators `<allocators/degenerate/>`

Degenerate allocators, while fully implementing the allocator interface, are lacking some internal mechanisms; this restricts their ussage to very specific situations. The advantage of degenerate allocators is their reduced memory footprint and improved performance, when compared to full-fledged allocators.

In this category we have:

- `Boolean`: all of it's memory is either allocated or free
- `Constant`: allocation always gives the same result: the whole memory
- `Null`: allocation always fails

## Metric allocators `<allocators/metrics/>`

Metric allocators record metrics on operations which can be later queried.

In this category we have:

- `stats`: record statistics (times failed, total allocated, etc)
- `timing`: record operation timings and mean times

## Pointer allocators `<allocators/pointer/>`

Pointer allocators are a bridge between the design paradigm used in this library and the usual system.

In a sense, pointer allocators are composite allocators, as they are implemented based on an allocator instance of different type, but they do not comply with the allocator interface, so they are not considered to be allocators inside this library, but merely bridge entities.
That's why they have a separate category.

Pointer allocators, as usual, return a pointer on allocation and take a pointer on deallocation.
Bookkeeping information is stored in an allocation header.

In this category we have:

- `basic`: basic functionality
- `checked`: provides an allocation header checksum. Throws on deallocation if corruption was detected.

## Utility allocators `<allocators/utility/>`

Utility allocators, as with pointer allocators, are not considered allocators in this library. They provide useful capabilities and extended flexibility, by bending their interface.
Because of this, they cannot have a common base class or be polymorphic.

Most (word used lightly) utility allocators relay allocation to another allocator instance, like composite allocators.

This category is still under development and may change without notice.

In this category we have:

- `filter`: A filter allocator with arbitrary request type.
- `owner`: Don't use it. Read the comments on the code and ponder your choice.
- `switch`: Arbitrary request type. A function on the request indicates which allocator to use.
- `unique_block`: Auto-freeing allocations with unique owner semantics. (TODO: Rename)
- `xref`: An extended proxy ref. Can be used with utility allocators.


# Design details

## Other headers, classes and structures

This library has a main header: `<allocators/allocators.hpp>`. It includes almost everything else.

The abstract base that exposes the allocator interface is defined in `<allocators/allocator.hpp>` (note the singular). More about this in the next section.

The exception types thrown in the library are defined in `<allcoators/exception.hpp>`

The headers in `<allocators/acquire_memory/>` provide an easy way to acquire memory from the stack (or heap, if there's support) to feed allocators. The user can also manually fill a `memory_block` for this purpose; this is useful in a freestanding environment.

The headers in `<allocators/structures/>` provide the definitions of the basic structures used throughout the library, such as the `memory_block` structure.

## Common allocator base for polymorphism

All allocators derive from a common abstract base, defined in `<allocators/allocator.hpp>`, which exposes the standard allocator interface. This makes possible to write polymorphic code that works with any allocator type.

## Moved-from allocators

Moved-from allocators cannot be used for allocation.
The only operations that are legal in this state are destruction and assignment.

