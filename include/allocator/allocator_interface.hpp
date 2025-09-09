#include <memory>

namespace allocator {
class AllocatorInterface {
  public:
    virtual ~AllocatorInterface() = default;

    virtual void *allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void *ptr) = 0;
    virtual size_t getAllocatedSize() const = 0;
    virtual void reset() = 0;

    static void *getAlignment(void *ptr, size_t alignment) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        auto alignAddr = (addr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void *>(alignAddr);
    }
};

// Adapter class for using AllocatorInterface with standard STL containers.
// enable custom memory allocation in this project. This version was generated with the
// help of AI and serves as a learning and testing tool while I deepen my
// understanding of STL allocators(9th september 2025). Future updates may refine this to achieve
// full allocator compliance and improved performance.

template <typename T> class AllocatorAdapter {
  public:
    using value_type = T;

    explicit AllocatorAdapter(AllocatorInterface *alloc) : allocator_(alloc) {}

    T *allocate(size_t n) {
        return static_cast<T *>(allocator_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T *ptr, size_t) { allocator_->deallocate(ptr); }

  private:
    AllocatorInterface *allocator_;
};
} // namespace allocator