# Why `new`/`delete` expressions are not supported

## Background

C++ separates the concerns of memory allocation and object construction. A `new`-expression
does two things: calls `operator new` to obtain raw memory, then constructs the object at that
address. A `delete`-expression does the reverse: destructs the object, then calls `operator delete`
to release the memory.

These two operators can be overridden — globally, per-class, or as placement forms — to redirect
raw memory operations to a custom allocator. This library's pointer allocators (`Basic` and any
future variants) are designed to serve as a `malloc`/`free` replacement and could in principle
back such overrides.

## The problem: placement delete does not exist as a user expression

Placement `operator new` accepts extra arguments: `new (my_allocator) T(...)` calls
`operator new(sizeof(T), my_allocator)`, allowing an allocator instance to be specified at the
call site.

There is no equivalent placement delete expression. `operator delete` can only be called by the
compiler, and only with one of these fixed signatures:

```cpp
operator delete(void*)
operator delete(void*, std::size_t)           // sized delete, C++14
operator delete(void*, std::align_val_t)      // aligned delete, C++17
operator delete(void*, std::size_t, std::align_val_t)
```

A matching placement `operator delete(void*, Allocator&)` can be defined, but the language only
invokes it in one specific situation: to undo an allocation when the constructor throws during a
placement `new`-expression. It cannot be called explicitly by user code. There is no
`delete (my_allocator) p` syntax, and this is not an oversight — the asymmetry is intentional
and permanent. Nothing in C++26 or any accepted proposal changes this.

## Why this rules out instance-specific allocator support

This system has no single global allocator. Different regions, pools, and subsystems use distinct
allocator instances. For `operator delete` overrides to work, the correct allocator instance must
be reachable at the call site — but there is no mechanism to pass it through a `delete`-expression.
Workarounds (global or thread-local allocator pointers) would impose architectural constraints
incompatible with the rest of the system.

## The design decision

Support for `new`/`delete` expressions with instance-specific allocators is not provided by this
library. It would require either a global allocator assumption (which this library deliberately
avoids) or a language feature that does not exist.

The library's answer to combined allocation and construction is `allocator_new`/`allocator_delete`,
which work with block allocators and carry allocation metadata in `new_result<T>`. This covers all
use cases that `new`/`delete` expressions would address, without the limitations above.

## What to do instead

Code that would otherwise use `new`/`delete` expressions should use `allocator_new`/`allocator_delete`
with an appropriate block allocator:

```cpp
// instead of: T* p = new T(args...);  ...  delete p;
auto result = mem::allocator_new<T>(my_allocator, args...);
// ...
mem::allocator_delete(my_allocator, result);
```

If a global `operator new`/`operator delete` shim is needed — for example, to support third-party
code or C++ runtime infrastructure that cannot be migrated — that shim should be provided by the
application code itself, wired to whichever allocator instance is appropriate for that context.
It is out of scope for this library.
