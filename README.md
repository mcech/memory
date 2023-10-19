memory
======


## Allocators

Allocators are classes that define memory models to be used by some parts of the
Standard Library, and most specifically, by STL containers.


### AlignedAllocator

`AlignedAllocator` takes a second template parameter to specify the alignment of
the memory chunks returned by `AlignedAllocator::allocate`.


### PoolAllocator

Unlike `std::allocator`,  `PoolAllocator::deallocate` does  not free the memory,
but stores  it  into  a memory pool  and  recycles  it  on  a subsequent call to
`PoolAllocator::allocate`.  `PoolAllocator` is thread-safe, lock-free, and (most
of the time) also wait-free.

It is designed  for use with containers that manage linked nodes of fixed  size,
such as `std::list`,  `std::set`, `std::map` etc. Regarding `std::vector`, there
is no difference compared to `std::allocator`.
