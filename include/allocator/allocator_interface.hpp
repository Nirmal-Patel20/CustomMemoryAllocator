#include <memory>

namespace allocator {
class AllocatorInterface {
  public:
    virtual ~AllocatorInterface() = default;

    virtual void* allocate(size_t size, size_t alignment = 0) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual size_t getAllocatedSize() const = 0;
    virtual void reset() = 0;

    static void* getAlignment(void* ptr, size_t alignment) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        auto alignAddr = (addr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void*>(alignAddr);
    }

    static size_t getAlignedSize(size_t size, size_t alignment) {
        auto alignedSize = (size + alignment - 1) & ~(alignment - 1);
        return alignedSize;
    }

    static size_t getAlignmentOfNativeType(size_t size) {
        if (size <= 1) return alignof(uint8_t);
        if (size <= 2) return alignof(uint16_t);
        if (size <= 4) return alignof(uint32_t);
        if (size <= 8) return alignof(uint64_t);
        if (size <= 16) return 16;
        return alignof(max_align_t);
    }

    static bool isAlignmentPowerOfTwo(size_t alignment) {
        return (alignment != 0) && ((alignment & (alignment - 1)) == 0);
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

    explicit AllocatorAdapter(AllocatorInterface* alloc) : allocator_(alloc) {}

    T* allocate(size_t n) {
        return static_cast<T*>(allocator_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* ptr, size_t) { allocator_->deallocate(ptr); }

  private:
    AllocatorInterface* allocator_;
};
} // namespace allocator