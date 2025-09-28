#include "allocator/Pool_allocator.hpp"

allocator::pool_allocator::pool_allocator(size_t blockSize, size_t initial_capacity,
                                          size_t alignment, size_t maxPools)
    : m_blockCount(initial_capacity), m_maxPools(maxPools) {

    if (blockSize == 0 || initial_capacity == 0) {
        throw std::invalid_argument(m_allocator +
                                    ": Block size and initial capacity must be greater than zero.");
    }

    if (alignment == 0) {
        m_alignment = sizeof(void*); // 8 bytes
    } else {
        if (!isAlignmentPowerOfTwo(alignment)) {
            throw std::invalid_argument(m_allocator + ": Alignment must be a power of two.");
        }
        if (alignment < alignof(int) || alignment > alignof(max_align_t)) {
            throw std::invalid_argument(m_allocator + ": Alignment must be at least between " +
                                        std::to_string(alignof(int)) + " and " +
                                        std::to_string(alignof(max_align_t)) + " bytes.");
        }

        m_alignment = alignment;
    }

    auto alignBlock = getAlignedSize(blockSize, m_alignment);
    m_blockSize = (alignBlock >= 8) ? alignBlock : 8;

    m_poolSize = m_blockSize * m_blockCount;

    if (m_poolSize > MAX_CAPACITY) {
        throw std::invalid_argument(m_allocator +
                                    ": Requested pool size exceeds maximum capacity(64 MB).");
    }

    allocate_new_pool();
}

allocator::pool_allocator::~pool_allocator() {
    releaseMemory();
}

void* allocator::pool_allocator::allocate(size_t size, [[maybe_unused]] size_t alignment) {
    if (size > m_blockSize) {
        allocator::throwAllocationError(m_allocator, "Requested size exceeds block size");
    }

    if (!m_ownsMemory) {
        allocator::throwAllocationError(m_allocator, "Allocator has released its memory");
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

    allocate_new_pool();
    return allocate(size, alignment);
}

void allocator::pool_allocator::deallocate(void* ptr) {
    if (!ptr) {
        throw std::invalid_argument(m_allocator + ": Attempted to deallocate a null pointer");
    }

    if (!m_ownsMemory) {
        throw std::invalid_argument(m_allocator + ": Allocator does not hold any memory on heap");
    }

    for (auto& pool : pools) {

        auto start = reinterpret_cast<std::uintptr_t>(pool.memory.get());
        auto end = start + pool.size;
        auto p = reinterpret_cast<std::uintptr_t>(ptr);

        if (p >= start && p < end) {

            std::uintptr_t offset = p - start;
            if (offset % m_blockSize != 0) {
                throw std::runtime_error(
                    "Pointer is inside pool memory but does not point to the start of a block");
            }

            // Optional: debug-only double-free detection.
            // This is O(n) but invaluable during development.
            if (allocator::g_debug_checks.load(
                    std::memory_order_relaxed)) { // allow benchmarks to opt out
                for (void* walk = pool.free_list_head; walk != nullptr;
                     walk = *reinterpret_cast<void**>(walk)) {
                    if (walk == ptr) {
                        throw std::runtime_error("Double free detected");
                    }
                }
            }

            // Put the block back on the free list
            *reinterpret_cast<void**>(ptr) = pool.free_list_head;
            pool.free_list_head = ptr;

            // Update
            --pool.allocated_count;
            ++pool.free_count;

            return;
        }
    }

    throw std::runtime_error(m_allocator +
                             ": Pointer does not belong to any pools inside this allocator");
}

size_t allocator::pool_allocator::getAllocatedSize() const {
    size_t totalAllocated = 0;
    for (const auto& p : pools) {
        totalAllocated += p.allocated_count * m_blockSize;
    }
    return totalAllocated;
}

size_t allocator::pool_allocator::getObjectSize() const {
    return m_blockSize;
}

void allocator::pool_allocator::reset() {
    if (m_ownsMemory) {
        while (pools.size() > 1) {
            pools.pop_back();
        }

        auto& last_pool = pools.front();
        for (size_t i = 0; i < m_blockCount; ++i) {
            void* block = last_pool.memory.get() + i * m_blockSize;
            *reinterpret_cast<void**>(block) = last_pool.free_list_head;
            last_pool.free_list_head = block;
            last_pool.free_count++;
        }

        last_pool.allocated_count = 0;
    } else {
        allocate_new_pool();
    }
}

void allocator::pool_allocator::releaseMemory() {
    pools.clear();
    m_ownsMemory = false;
}

void allocator::pool_allocator::allocate_new_pool() {

    if (m_ownsMemory && allocator::g_capacity_checks.load(
                            std::memory_order_relaxed)) { // allow benchmarks to opt out
        if (m_poolSize * (pools.size() + 1) > MAX_CAPACITY) {
            allocator::throwAllocationError(m_allocator, "Exceeds maximum capacity(64 MB)");
        }

        if (m_maxPools != 0 && ((pools.size() + 1) > m_maxPools)) {
            allocator::throwAllocationError(m_allocator, "Exceeds maximum pool count : " +
                                                             std::to_string(m_maxPools));
        }
    }

    pool new_pool;
    new_pool.memory = std::make_unique<std::byte[]>(m_poolSize);
    m_ownsMemory = true;
    new_pool.size = m_poolSize;

    // Initialize free list
    for (size_t i = 0; i < m_blockCount; ++i) {
        void* block = new_pool.memory.get() + i * m_blockSize;
        *reinterpret_cast<void**>(block) = new_pool.free_list_head;
        new_pool.free_list_head = block;
        new_pool.free_count++;
    }

    pools.push_back(std::move(new_pool));
}

void allocator::pool_allocator::setAllocatorName(std::string_view name) {
    m_allocator = name;
}