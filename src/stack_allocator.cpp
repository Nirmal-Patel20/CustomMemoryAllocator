#include "allocator/stack_allocator.hpp"

allocator::stack_allocator::stack_allocator(size_t bufferSize, size_t alignment, bool resizable)
    : m_bufferSize(bufferSize), m_resizable(resizable) {

    if (bufferSize > MAX_CAPACITY) {
        throw std::invalid_argument("stack_allocator: Requested size exceeds maximum capacity(" +
                                    std::to_string(MAX_CAPACITY) + " MB).");
    }

    if (alignment == 0) {
        m_alignment = sizeof(void*); // 8 bytes
    } else {
        if (!isAlignmentPowerOfTwo(alignment)) {
            throw std::invalid_argument("stack_allocator: Alignment must be a power of two.");
        }
        if (alignment < alignof(int) || alignment > alignof(max_align_t)) {
            throw std::invalid_argument("stack_allocator: Alignment must be at least between " +
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
        allocator::throwAllocationError("stack_allocator", "Allocator has released its memory");
    }

    if (size > m_bufferSize) {
        allocator::throwAllocationError("stack_allocator", "Requested size exceeds buffer size(" +
                                                               std::to_string(m_bufferSize) +
                                                               " bytes");
    }

    if (alignment == 0) {
        alignment = m_alignment;
    } else {
        if (!isAlignmentPowerOfTwo(alignment)) {
            throw std::invalid_argument("stack_allocator: Alignment must be a power of two.");
        }
        if (alignment < alignof(int)) {
            throw std::invalid_argument("stack_allocator: Alignment must be greater than " +
                                        std::to_string(alignof(int)) + " bytes.");
        }
    }

    auto& lastbuffer = buffers.back();
    auto alignSize = getAlignedSize(size, alignment);

    if (lastbuffer.offset + alignSize <= lastbuffer.size) {
        void* ptr = lastbuffer.memory.get() + lastbuffer.offset;
        lastbuffer.offset += alignSize;

        allocation_history.push_back({ptr, alignSize});
        return ptr;
    }

    // Current buffer full, need new one
    allocate_new_buffer();
    return allocate(size, alignment);
}

void allocator::stack_allocator::deallocate(void* ptr) {

    if (allocation_history.back().ptr != ptr) {
        throw std::invalid_argument("Stack allocator: Invalid LIFO deallocation order");
    }

    auto& lastbuffer = buffers.back();

    lastbuffer.offset -= allocation_history.back().size;
    if (lastbuffer.offset == 0 && buffers.size() > 1) {
        buffers.pop_back();
    }
    allocation_history.pop_back();
}

size_t allocator::stack_allocator::getAllocatedSize() const {
    size_t totalAllocated = 0;

    for (auto& buffer : buffers) {
        totalAllocated = buffer.offset;
    }
    return totalAllocated;
}

size_t allocator::stack_allocator::getObjectSize() const {
    return allocation_history.back().size; // Size of the last allocated object
}

void allocator::stack_allocator::reset() {

    // if memory own it would reuse it otherwise create new one
    if (m_ownsMemory) {
        // Retain only one buffer
        while (buffers.size() != 1) {
            buffers.pop_back();
        }

        // Clear allocation history
        allocation_history.clear();

        auto& lastbuffer = buffers.back();
        lastbuffer.offset = 0; // Reset offset
    } else {
        allocate_new_buffer();
    }
}

void allocator::stack_allocator::releaseMemory() {
    buffers.clear();
    allocation_history.clear();
    m_ownsMemory = false;
}

void allocator::stack_allocator::allocate_new_buffer() {
    if (m_ownsMemory) {
        if (!m_resizable) {
            allocator::throwAllocationError("stack_allocator",
                                            "Cannot allocate new buffer in non-resizable mode");
        } else if (m_resizable && (m_bufferSize * (buffers.size() + 1) > MAX_CAPACITY)) {
            allocator::throwAllocationError("stack_allocator", "Exceeds maximum capacity(" +
                                                                   std::to_string(MAX_CAPACITY) +
                                                                   " MB)");
        }
    }

    buffer new_buffer;
    new_buffer.memory = std::make_unique<std::byte[]>(m_bufferSize);
    m_ownsMemory = true;
    new_buffer.size = m_bufferSize;
    buffers.push_back(std::move(new_buffer));
}