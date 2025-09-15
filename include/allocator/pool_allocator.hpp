#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include "allocator/allocatorError.hpp"
#include "allocator/allocator_interface.hpp"
#include <iostream>
#include <vector>

namespace allocator {
class Pool_Allocator : public AllocatorInterface {
  public:
    explicit Pool_Allocator(size_t blockSize, size_t blockCount, size_t alignment = 0,
                            size_t maxPools = 0);
    ~Pool_Allocator() override;

    virtual void* allocate(size_t size, [[maybe_unused]] size_t alignment = 0) override;
    virtual void deallocate(void* ptr) override;
    virtual size_t getAllocatedSize() const override;
    virtual size_t getObjectSize() const override;
    virtual void reset() override;
    void releaseMemory();

  private:
    struct pool {
        std::unique_ptr<std::byte[]> memory;
        size_t size = 0;
        void* free_list_head = nullptr;
        size_t allocated_count = 0;
        size_t free_count = 0;
    };

    void allocate_new_pool();

    size_t m_blockSize;
    size_t m_blockCount;
    size_t m_alignment;
    size_t m_poolSize;
    std::vector<pool> pools;
    bool m_ownsMemory = false; // check if the allocator owns the memory
    size_t m_maxPools = 0;     // configurable
    static constexpr size_t MAX_CAPACITY = 64ull * 1024 * 1024; // 64 MB hard cap
};

} // namespace allocator

#endif // POOL_ALLOCATOR_HPP