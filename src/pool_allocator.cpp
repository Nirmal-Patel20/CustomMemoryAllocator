#include "allocator/pool_allocator.hpp"

allocator::Pool_Allocator::Pool_Allocator(size_t m_blockSize, size_t initial_capacity,
                                          size_t alignment)
    : m_blockSize(m_blockSize), m_blockCount(initial_capacity), m_Alignment(alignment) {

    std::cout << std::string(40,'-') << std::endl;
    m_blockSize =
        ((m_blockSize >= 8) ? m_blockSize : sizeof(void*)); // Ensure minimum size for free list
    m_poolSize = m_blockSize * m_blockCount;

    allocate_new_pool();
}

allocator::Pool_Allocator::~Pool_Allocator() {
    pools.clear();
}

void* allocator::Pool_Allocator::allocate(size_t size,
                                [[maybe_unused]]  size_t alignment) {
    if (size > m_blockSize) {
        throw std::bad_alloc(); // Requested size exceeds block size
    }

    for (auto& p : pools) {
        if (p.free_list_head != nullptr) {
            void* block = p.free_list_head;
            p.free_list_head = *reinterpret_cast<void**>(block);
            p.allocated_count++;
            p.free_count--;

            return block;
        }
    }
    return nullptr;
}

void allocator::Pool_Allocator::deallocate(void* ptr) {
    for (auto& pool : pools) {
        auto start = pool.memory.get();
        auto end = pool.memory.get() + pool.size;

        if (ptr >= start && ptr <= end) {
            *reinterpret_cast<void**>(ptr) = pool.free_list_head;
            pool.free_list_head = ptr;
            return;
        }
    }

    throw std::runtime_error("Pointer does not belong to any pools inside this allocator");
}

size_t allocator::Pool_Allocator::getAllocatedSize() const {
    size_t totalAllocatedSize = 0;
    for(auto& pool : pools){
        totalAllocatedSize += pool.size;
    }

    return totalAllocatedSize;
}

void allocator::Pool_Allocator::reset() {
    pools.clear();
    m_poolSize = m_blockSize * m_blockCount;
    allocate_new_pool();
}

void allocator::Pool_Allocator::allocate_new_pool() {

    pool new_pool;
    new_pool.memory = std::make_unique<std::byte[]>(m_poolSize);
    new_pool.size = m_poolSize;

    // Initialize free list
    for (size_t i = 0; i < m_blockCount; ++i) {
        void* block = new_pool.memory.get() + i * m_blockSize;
        void* aligned_block = getAlignment(block, m_Alignment); // Ensure alignment
        *reinterpret_cast<void**>(aligned_block) = new_pool.free_list_head;
        new_pool.free_list_head = aligned_block;
        new_pool.free_count++;
    }

    pools.push_back(std::move(new_pool));
}