#include "allocator/allocator_interface.hpp"
#include <vector>

namespace allocator {

class stack_allocator : public AllocatorInterface {
  public:
    stack_allocator(size_t bufferSize, size_t alignment = 0, bool m_resizable = false);
    ~stack_allocator() override;

    [[nodiscard]] virtual void* allocate(size_t size, size_t alignment = 0) override;
    virtual void deallocate(void* ptr) override;
    virtual size_t getAllocatedSize() const override;
    virtual size_t getObjectSize() const override;
    virtual void reset() override;
    void releaseMemory();
    void setAllocatorName(std::string_view name);
    const std::pair<size_t, size_t> mark();
    void reset_to_mark(const std::pair<size_t, size_t>& mark);

  private:
    void allocate_new_buffer();

#if ALLOCATOR_DEBUG
    // To track last allocation for deallocation
    struct allocation_info {
        void* ptr;   // Pointer to allocated memory
        size_t size; // Size of allocation
    };

    std::vector<allocation_info> allocation_history; // Track allocations for LIFO deallocation
#endif

    // Pre-allocated memory buffer
    struct buffer {
        std::unique_ptr<std::byte[]> memory; // Contiguous memory
        size_t size;                         // Total buffer size
        size_t offset = 0;                   // Current allocation offset
    };

    size_t m_alignment;          // Default alignment
    size_t m_bufferSize;         // Default buffer size
    std::vector<buffer> buffers; // All allocated buffers
    bool m_resizable = false;    // configurable
    bool m_ownsMemory = false;   // check if the allocator owns the memory
    static constexpr size_t MAX_CAPACITY =
        64ull * 1024 * 1024;                     // 64 MB Max capacity if it's not resizable
    std::string m_allocator = "stack_allocator"; // Custom Name for debugging
};

} // namespace allocator