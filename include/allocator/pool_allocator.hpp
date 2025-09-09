#ifndef POOL_ALLOCATOR_HPP
#define POOL_ALLOCATOR_HPP

#include "allocator/allocator_interface.hpp"
#include <iostream>
#include <vector>

namespace allocator {
class Pool_Allocator : public AllocatorInterface {
  public:
    explicit Pool_Allocator(size_t blockSize, size_t blockCount,
                            size_t alignment = alignof(std::max_align_t));
    ~Pool_Allocator() override;

    virtual void* allocate(size_t size, [[maybe_unused]] size_t alignment = alignof(std::max_align_t)) override;
    virtual void deallocate(void* ptr) override;
    virtual size_t getAllocatedSize() const override;
    virtual void reset() override;

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
    size_t m_Alignment;
    size_t m_poolSize;
    std::vector<pool> pools;
};

} // namespace allocator

#endif // POOL_ALLOCATOR_HPP