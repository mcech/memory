#pragma once

#include <atomic>
#include <type_traits>
#include <cstddef>

namespace mcech::memory
{
    /**
     * @brief   Pool Allocator
     *
     * Allocators are classes that define memory models to be used by some parts
     * of the Standard Library, and most specifically, by STL containers.
     *
     * Unlike  std::allocator,   PoolAllocator::deallocate  does  not  free  the
     * memory,  but stores it into a memory pool and recycles it on a subsequent
     * call to PoolAllocator::allocate.
     *
     * @tparam  T   Type of the elements allocated by PoolAllocator
     */
    template <class T>
    class PoolAllocator
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
         * @brief   Indicates that the PoolAllocator  shall  propagate  when the
         *          container is move-assigned
         */
        using propagate_on_container_move_assignment = std::true_type;

        /**
         * @brief   Construct PoolAllocator object
         */
        constexpr PoolAllocator() noexcept = default;

        /**
         * @brief   Copy-construct PoolAllocator object
         *
         * @param   x   Another PoolAllocator to construct from
         */
        constexpr PoolAllocator(const PoolAllocator& x) noexcept = default;

        /**
         * @brief   Copy-construct PoolAllocator object
         *
         * @tparam  U   Element type of x
         *
         * @param   x   Another PoolAllocator to construct from
         */
        template <class U>
        constexpr PoolAllocator(const PoolAllocator<U>& x) noexcept;

        /**
         * @brief   Destructs the PoolAllocator object
         */
        constexpr ~PoolAllocator() = default;

        /**
         * @brief   Allocate block of storage
         *
         * Attempts to allocate a block of storage with  a size large enough  to
         * contain n elements of  member type value_type,  and returns a pointer
         * to the first element.
         *
         * The storage is aligned appropriately for objects of type  value_type,
         * but they are not constructed.
         *
         * If  there have been prior calls to  PoolAllocator::deallocate,  these
         * blocks of storage may be reused.
         *
         * @param   n   Number of elements to be allocated.
         *
         * @return  A pointer to the first element in the block of storage.
         *
         * @throws  std::bad_alloc   if the amount  of storage  requested  could
         *                           not be allocated.
         */
        [[nodiscard]] constexpr value_type* allocate(size_type n);

        /**
         * @brief   Release block of storage
         *
         * Releases  a  block  of  storage  previously  allocated  with   member
         * allocate.
         *
         * The elements in the array are  not destroyed by a call to this member
         * function.
         *
         * Unlike  std::allocator::deallocate,  the  memory is stored and may be
         * reused by PoolAllocator::allocate.
         *
         * @param   p   Pointer to a block of storage previously allocated  with
         *              PoolAllocator::allocate(size_type).
         * @param   n   Number   of  elements   allocated   on   the   call   to
         *              PoolAllocator::allocate(size_type)  for  this  block  of
         *              storage.
         */
        constexpr void deallocate(value_type* p, size_type n);

        /**
         * @brief   Compares the PoolAllocators lhs and rhs
         *
         * Performs  a comparison operation between the  PoolAllocator  lhs  and
         * rhs.
         *
         * Always evaluates to true.
         *
         * @tparam  T1   Element type of lhs
         * @tparam  T2   Element type of rhs
         *
         * @param   lhs,rhs   PoolAllocators to be compared
         *
         * @return  true
         */
        template <class T1, class T2>
        friend constexpr bool operator==(
                const PoolAllocator<T1>& lhs,
                const PoolAllocator<T2>& rhs) noexcept;

    private:
        union Node
        {
            Node* next;
            T value;
        };

        class SharedPool
        {
        public:
            constexpr ~SharedPool();

            constexpr Node* load() const noexcept;
            constexpr bool try_compare_exchange(Node*& expected, Node* desired) noexcept;
            constexpr void compare_exchange(Node*& expected, Node* desired) noexcept;

        private:
            std::atomic<Node*> head = nullptr;
        };

        class LocalPool
        {
        public:
            constexpr LocalPool(SharedPool& shared);
            constexpr ~LocalPool();
            constexpr LocalPool& operator=(Node* p) noexcept;
            constexpr operator Node*&() noexcept;
            constexpr operator const Node*() const noexcept;

        private:
            SharedPool& shared;
            Node* head = nullptr;
        };

        constexpr T*   alloc(size_t n);
        constexpr void add_to_shared_pool(T* p);
        constexpr void move_from_shared_to_local_pool() noexcept;
        constexpr T*   reuse_from_local_pool();
        constexpr void move_from_local_to_shared_pool() noexcept;
        constexpr void dealloc(T* p, size_t n);

        static SharedPool shared_pool;
        static thread_local LocalPool local_pool;
    };

    template <class T>
    template <class U>
    inline constexpr PoolAllocator<T>::PoolAllocator(const PoolAllocator<U>&) noexcept
    {
    }

    template <class T>
    inline constexpr typename PoolAllocator<T>::value_type* PoolAllocator<T>::allocate(size_type n)
    {
        if (n == 1)
        {
            move_from_shared_to_local_pool();
            value_type* p = reuse_from_local_pool();
            move_from_local_to_shared_pool();
            if (p != nullptr)
            {
                return p;
            }
        }

        return alloc(n);
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::deallocate(value_type* p, size_type n)
    {
        if (n == 1)
        {
            add_to_shared_pool(p);
        }
        else
        {
            dealloc(p, n);
        }
    }

    template <class T1, class T2>
    inline constexpr bool operator==(const PoolAllocator<T1>&, const PoolAllocator<T2>&) noexcept
    {
        return true;
    }

    template <class T>
    inline constexpr PoolAllocator<T>::SharedPool::~SharedPool()
    {
        Node* p = head.load();
        while (p != nullptr)
        {
            head.compare_exchange_weak(p, p->next);
            ::operator delete(static_cast<void*>(p));
            p = head.load();
        }
    }

    template <class T>
    inline constexpr typename PoolAllocator<T>::Node* PoolAllocator<T>::SharedPool::load() const noexcept
    {
        return head.load();
    }

    template <class T>
    inline constexpr bool PoolAllocator<T>::SharedPool::try_compare_exchange(Node*& expected, Node* desired) noexcept
    {
        return head.compare_exchange_weak(expected, desired);
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::SharedPool::compare_exchange(Node*& expected, Node* desired) noexcept
    {
        while (!head.compare_exchange_weak(expected, desired))
        {
        }
    }

    template <class T>
    inline constexpr PoolAllocator<T>::LocalPool::LocalPool(SharedPool& shared) : shared(shared)
    {
    }

    template <class T>
    inline constexpr PoolAllocator<T>::LocalPool::~LocalPool()
    {
        Node* null_ptr = nullptr;
        if (!shared.try_compare_exchange(null_ptr, head))
        {
            while (head != nullptr)
            {
                Node* p = head;
                head = p->next;
                p->next = shared.load();
                shared.compare_exchange(p->next, p);
            }
        }
    }

    template <class T>
    inline constexpr PoolAllocator<T>::LocalPool& PoolAllocator<T>::LocalPool::operator=(Node* p) noexcept
    {
        head = p;
        return *this;
    }

    template <class T>
    inline constexpr PoolAllocator<T>::LocalPool::operator typename PoolAllocator<T>::Node*&() noexcept
    {
        return head;
    }

    template <class T>
    inline constexpr PoolAllocator<T>::LocalPool::operator const typename PoolAllocator<T>::Node*() const noexcept
    {
        return head;
    }

    template <class T>
    inline constexpr T* PoolAllocator<T>::alloc(size_t n)
    {
        return static_cast<T*>(::operator new(n * sizeof(Node)));
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::add_to_shared_pool(T* p)
    {
        Node* node = reinterpret_cast<Node*>(p);
        node->next = shared_pool.load();
        shared_pool.compare_exchange(node->next, node);
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::move_from_shared_to_local_pool() noexcept
    {
        if (local_pool == nullptr)
        {
            local_pool = shared_pool.load();
            shared_pool.compare_exchange(local_pool, nullptr);
        }
    }

    template <class T>
    inline constexpr T* PoolAllocator<T>::reuse_from_local_pool()
    {
        Node* p = local_pool;
        if (p != nullptr)
        {
            local_pool = p->next;
        }
        return reinterpret_cast<T*>(p);
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::move_from_local_to_shared_pool() noexcept
    {
        Node* null_ptr = nullptr;
        if (shared_pool.try_compare_exchange(null_ptr, local_pool))
        {
            local_pool = nullptr;
        }
    }

    template <class T>
    inline constexpr void PoolAllocator<T>::dealloc(T* p, size_t n)
    {
        ::operator delete(static_cast<void*>(p), n * sizeof(Node));
    }

    template <class T>
    typename PoolAllocator<T>::SharedPool PoolAllocator<T>::shared_pool;

    template <class T>
    thread_local PoolAllocator<T>::LocalPool PoolAllocator<T>::local_pool(shared_pool);
}
