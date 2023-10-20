#pragma once

#include <new>
#include <type_traits>
#include <cstddef>

namespace mcech::memory
{
    /**
     * @brief   Aligned Allocator
     *
     * Allocators are classes that define memory models to be used by some parts
     * of the Standard Library, and most specifically, by STL containers.
     *
     * AlignedAllocator  takes  a  second  template  parameter  to  specify  the
     * alignment of the memory chunks returned by AlignedAllocator::allocate.
     *
     * @tparam  T   Type of the elements allocated by AlignedAllocator
     * @tparam  N   Alignment of the elements allocated by AlignedAllocator
     */
    template <class T, size_t N>
    class AlignedAllocator
    {
    public:
        /**
         * @brief   Element type
         */
        using value_type = T;

        /**
         * @brief   Quantities of elements
         */
        using size_type = size_t;

        /**
         * @brief   Difference between two pointers
         */
        using difference_type = ptrdiff_t;

        /**
         * @brief   Indicates that the AlignedAllocator shall propagate when the
         *          container is move-assigned
         */
        using propagate_on_container_move_assignment = std::true_type;

        /**
         * @brief   Construct AlignedAllocator object
         */
        constexpr AlignedAllocator() noexcept = default;

        /**
         * @brief   Copy-construct AlignedAllocator object
         *
         * @param   x   Another AlignedAllocator to construct from
         */
        constexpr AlignedAllocator(const AlignedAllocator& x) noexcept = default;

        /**
         * @brief   Copy-construct AlignedAllocator object
         *
         * @tparam  U   Element type of x
         * @tparam  N   Alignment of the elements
         *
         * @param   x   Another AlignedAllocator to construct from
         */
        template <class U>
        constexpr AlignedAllocator(const AlignedAllocator<U, N>& x) noexcept;

        /**
         * @brief   Destructs the AlignedAllocator object
         */
        constexpr ~AlignedAllocator() = default;

        /**
         * @brief   Allocate block of storage
         *
         * Attempts to allocate a  block of storage with a size large enough  to
         * contain n elements of member type value_type,  and returns a  pointer
         * to the first element.
         *
         * The storage is aligned according to the template parameter N,  but no
         * objects are constructed.
         *
         * @param   n   Number of elements to be allocated.
         *
         * @return  A pointer to the first element in the block of storage.
         *
         * @throws  std::bad_alloc   if  the amount  of storage  requested could
         *                           not be allocated.
         */
        [[nodiscard]] constexpr value_type* allocate(size_type n);

        /**
         * @brief   Release block of storage
         *
         * Releases  a  block  of   storage  previously  allocated  with  member
         * allocate.
         *
         * The elements in  the array are not destroyed by a call to this member
         * function.
         *
         * @param   p   Pointer  to a block of storage previously allocated with
         *              AlignedAllocator::allocate(size_type).
         * @param   n   Number   of   elements   allocated   on   the  call   to
         *              AlignedAllocator::allocate(size_type) for  this block of
         *              storage.
         */
        constexpr void deallocate(value_type* p, size_type n);

        /**
         * @brief   Compares the AlignedAllocators lhs and rhs
         *
         * Performs a comparison operation between the AlignedAllocator  lhs and
         * rhs.
         *
         * Evaluates to true, if N1 is equal to N2.
         *
         * @tparam  T1   Element type of lhs
         * @tparam  N1   Alignment of lhs
         * @tparam  T2   Element type of rhs
         * @tparam  N2   Alignment of rhs
         *
         * @param   lhs,rhs   AlignedAllocatorss to be compared
         *
         * @return  true,  if (N1 == N2)
         *          false, otherwise
         */
        template <class T1, size_t N1, class T2, size_t N2>
        friend constexpr bool operator==(
                const AlignedAllocator<T1, N1>& lhs,
                const AlignedAllocator<T2, N2>& rhs) noexcept;
    };

    template <class T, size_t N>
    template <class U>
    inline constexpr AlignedAllocator<T, N>::AlignedAllocator(const AlignedAllocator<U, N>&) noexcept
    {
    }

    template <class T, size_t N>
    inline constexpr typename AlignedAllocator<T, N>::value_type* AlignedAllocator<T, N>::allocate(size_type n)
    {
        return static_cast<value_type*>(::operator new(n * sizeof(value_type), static_cast<std::align_val_t>(N)));
    }

    template <class T, size_t N>
    inline constexpr void AlignedAllocator<T, N>::deallocate(value_type* p, size_type n)
    {
        ::operator delete(static_cast<void*>(p), n * sizeof(value_type), static_cast<std::align_val_t>(N));
    }

    template <class T1, size_t N1, class T2, size_t N2>
    inline constexpr bool operator==(const AlignedAllocator<T1, N1>&, const AlignedAllocator<T2, N2>&) noexcept
    {
        return N1 == N2;
    }
}
