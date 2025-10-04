#include "allocator/stack_allocator.hpp"

allocator::stack_allocator::stack_allocator(size_t bufferSize, size_t alignment, bool resizable)
    : m_bufferSize(bufferSize), m_resizable(resizable) {

    if (bufferSize > MAX_CAPACITY) {
        throw std::invalid_argument(m_allocator + ": Requested size exceeds maximum capacity(" +
                                    std::to_string(MAX_CAPACITY / (1024 * 1024)) + " MB).");
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

    allocate_new_buffer();
}

allocator::stack_allocator::~stack_allocator() {
    releaseMemory();
}

void* allocator::stack_allocator::allocate(size_t size, size_t alignment) {
    if (!m_ownsMemory) {
        throwAllocationError(m_allocator, "Allocator has released its memory");
    }

    if (alignment == 0) {
        alignment = m_alignment;
    } else {
        if (!isAlignmentPowerOfTwo(alignment)) {
            throw std::invalid_argument(m_allocator + ": Alignment must be a power of two.");
        }
        if (alignment < alignof(int)) {
            throw std::invalid_argument(m_allocator + ": Alignment must be greater than " +
                                        std::to_string(alignof(int)) + " bytes.");
        }
    }

    auto alignSize = getAlignedSize(size, alignment);

    if (alignSize > m_bufferSize) {
        throwAllocationError(m_allocator, "Requested size exceeds buffer size(" +
                                              std::to_string(m_bufferSize) + " bytes");
    }

    auto& lastbuffer = buffers.back();

    if (lastbuffer.offset + alignSize <= lastbuffer.size) {
        void* ptr = lastbuffer.memory.get() + lastbuffer.offset;
        lastbuffer.offset += alignSize;

#if ALLOCATOR_DEBUG
        if (allocatorChecks::g_debug_checks.load(std::memory_order_relaxed)) {
            allocation_history.push_back({ptr, alignSize});
        }
#endif

        return ptr;
    }

    // Current buffer full, need new one
    allocate_new_buffer();
    return allocate(size, alignment);
}

void allocator::stack_allocator::deallocate(void* ptr) {

    if (!ptr) {
        throw std::invalid_argument(m_allocator + ": Attempted to deallocate a null pointer");
    }

    if (!m_ownsMemory) {
        throw std::invalid_argument(m_allocator + ": Allocator does not hold any memory on heap");
    }

    auto& lastbuffer = buffers.back();

#if ALLOCATOR_DEBUG
    if (allocatorChecks::g_debug_checks.load(std::memory_order_relaxed)) {
        auto& last_alloc = allocation_history.back();
        if (last_alloc.ptr != ptr) {
            throw std::invalid_argument(m_allocator + ": Invalid LIFO deallocation order");
        }

        lastbuffer.offset -= last_alloc.size;
        allocation_history.pop_back();
    } else
#endif
    {
        // Note: The 'else' above pairs with the debug 'if' only if ALLOCATOR_DEBUG is defined.
        // If ALLOCATOR_DEBUG is not defined, this block always runs.

        // release or debug check are off:
        // Assume ptr is the last allocation (LIFO)
        auto raw_ptr = static_cast<std::byte*>(ptr);
        auto top_ptr = lastbuffer.memory.get() + lastbuffer.offset;

        if (raw_ptr >= top_ptr) {
            throw std::invalid_argument(
                m_allocator + ": Pointer is beyond current top; memory corruption suspected");
        }

        size_t size = top_ptr - raw_ptr;
        if (size > lastbuffer.offset) {
            std::cout << "size: " << size << ", " << "offset: " << lastbuffer.offset << "\n";
            throw std::runtime_error(m_allocator + ": Calculated deallocation size exceeds current "
                                                   "allocated offset; memory corruption suspected");
        }

        lastbuffer.offset -= size;
    }

    // Drop empty buffer unless it's the only one
    if (lastbuffer.offset == 0 && buffers.size() > 1) {
        buffers.pop_back();
    }
}

size_t allocator::stack_allocator::getAllocatedSize() const {
    size_t totalAllocated = 0;

    for (auto& buffer : buffers) {
        totalAllocated += buffer.offset;
    }
    return totalAllocated;
}

size_t allocator::stack_allocator::getObjectSize() const {
    size_t lastObjectSize = 0;

#if ALLOCATOR_DEBUG
    if (!allocation_history.empty()) {
        lastObjectSize = allocation_history.back().size;
    }
#else
    throw std::runtime_error(m_allocator + ": getObjectSize() is only available in debug mode.");
#endif

    return lastObjectSize; // Size of the last allocated object
}

void allocator::stack_allocator::reset() {

    // if memory own it would reuse it otherwise create new one
    if (m_ownsMemory) {
        // Retain only one buffer
        while (buffers.size() != 1) {
            buffers.pop_back();
        }

#if ALLOCATOR_DEBUG
        // Clear allocation history
        allocation_history.clear();
#endif

        auto& lastbuffer = buffers.back();
        lastbuffer.offset = 0; // Reset offset
    } else {
        allocate_new_buffer();
    }
}

void allocator::stack_allocator::releaseMemory() {
    buffers.clear();
#if ALLOCATOR_DEBUG
    // Clear allocation history
    allocation_history.clear();
#endif
    m_ownsMemory = false;
}

void allocator::stack_allocator::allocate_new_buffer() {
    if (m_ownsMemory && allocatorChecks::g_capacity_checks.load(
                            std::memory_order_relaxed)) { // allow benchmarks to opt out
        if (!m_resizable) {
            throwAllocationError(m_allocator, "Cannot allocate new buffer in non-resizable mode");
        } else if (m_resizable && (m_bufferSize * (buffers.size() + 1) > MAX_CAPACITY)) {
            throwAllocationError(m_allocator, "Exceeds maximum capacity(" +
                                                  std::to_string(MAX_CAPACITY / (1024 * 1024)) +
                                                  " MB)");
        }
    }

    buffer new_buffer;
    new_buffer.memory = std::make_unique<std::byte[]>(m_bufferSize);
    m_ownsMemory = true;
    new_buffer.size = m_bufferSize;
    buffers.push_back(std::move(new_buffer));
}

void allocator::stack_allocator::setAllocatorName(std::string_view name) {
    m_allocator = name;
}

const std::pair<size_t, size_t> allocator::stack_allocator::mark() {
    return {buffers.size(), buffers.back().offset};
}

void allocator::stack_allocator::reset_to_mark(const std::pair<size_t, size_t>& mark) {

    if (buffers.empty()) {
        throw std::invalid_argument(m_allocator +
                                    ": reset_to_mark() called on a non-memory owning allocator");
    }

    auto [markTimeSize, offset] = mark;
    auto bufferSize = buffers.size();

    int sizeDiff = bufferSize - markTimeSize;

    // Negative sizeDiff means the mark was taken when more buffers existed.
    // This usually indicates an invalid usage: trying to deallocate/reset
    // to a mark from the "future" (after some buffers have already been released
    if (sizeDiff < 0) {
        throw std::runtime_error(
            m_allocator + ": invalid mark. Current buffer count = " + std::to_string(bufferSize) +
            ", mark expected at least = " + std::to_string(markTimeSize));
    }

    if (sizeDiff == 0 && buffers.back().offset < offset) {
        throw std::runtime_error(m_allocator + ": invalid mark. Mark offset (" +
                                 std::to_string(offset) + ") is ahead of current buffer offset (" +
                                 std::to_string(buffers.back().offset) + ")");
    }

    while (buffers.size() > markTimeSize) {
        buffers.pop_back();
    }

    buffers.back().offset = offset;
}