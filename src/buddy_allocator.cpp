#include "allocator/buddy_allocator.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#if ALLOCATOR_DEBUG
#define handle_allocation_error(msg) throwAllocationError(m_allocator, msg)
#else
#define handle_allocation_error(msg) return nullptr
#endif

allocator::buddy_allocator::buddy_allocator(size_t buffersize) {
    if (buffersize < MIN_CAPACITY || buffersize > MAX_CAPACITY) {
        throw std::invalid_argument("Buffer size must be between" +
                                    std::to_string(MIN_CAPACITY / 1024) + "KB and " +
                                    std::to_string(MAX_CAPACITY / 1024 * 1024) + "MB");
    }
    m_buffersize = get_power_of_two(buffersize);

    allocate_new_buffer();
}

allocator::buddy_allocator::~buddy_allocator() {
    releaseMemory();
}

void* allocator::buddy_allocator::allocate(size_t size, [[maybe_unused]] size_t alignment) {

    // this function takes alignment parameter for polymorphism, but alignment is ignored in buddy
    // allocator

    if (!m_ownsMemory) {
        handle_allocation_error("Allocator has released its memory");
    }

    if (size > m_buffersize) {
        handle_allocation_error("Requested size exceeds buffer size");
    }

    // Find the appropriate free block

    auto actualSize = get_power_of_two(size);
    int level = get_level(actualSize);
    Buddy* buddy = get_first_free_buddy(level);
    if (!buddy) {
        // No free block at this level, try to find a larger block and split it
        int nonEmptyLevel = find_non_empty_level(level + 1);
        if (nonEmptyLevel == -1) {
            handle_allocation_error("No sufficient block available for allocation(" +
                                    std::to_string(actualSize) + ")");
        }

        Buddy* largerBuddy = nullptr;

        // Split blocks down to the desired level
        while (nonEmptyLevel > level) {

            if (!largerBuddy) {
                largerBuddy = get_first_free_buddy(nonEmptyLevel);
            } else {
                largerBuddy = buddy; // use the previously split buddy
            }

            if (!largerBuddy) {
                handle_allocation_error("Inconsistent state: expected a free block");
            }
            buddy = split_buddy(largerBuddy, nonEmptyLevel);
            nonEmptyLevel--;
        }
    }

    // Mark the block as allocated
    allocatedBuddies[reinterpret_cast<void*>(buddy)] = level;
    return reinterpret_cast<void*>(buddy);
}

void allocator::buddy_allocator::deallocate(void* ptr) {

    if (!ptr) {
        throw std::invalid_argument(m_allocator + ": Attempted to deallocate a null pointer");
    }

    if (!m_ownsMemory) {
        throw std::invalid_argument(m_allocator + ": Allocator has released its memory");
    }

    auto it = allocatedBuddies.find(ptr);
    if (it == allocatedBuddies.end()) {
        throw std::invalid_argument(m_allocator + ": Pointer not allocated by this allocator");
    }

    int level = it->second;
    allocatedBuddies.erase(it);

    Buddy* buddy = reinterpret_cast<Buddy*>(ptr);
    add_to_free_list(buddy, level);

    // Try to merge with buddy
    try_merge_buddies(buddy, level);
}

size_t allocator::buddy_allocator::getAllocatedSize() const {
    size_t totalAllocated = 0;
    for (const auto& [ptr, level] : allocatedBuddies) {
        totalAllocated += get_level_size(level);
    }
    return totalAllocated;
}

void allocator::buddy_allocator::setAllocatorName(std::string_view name) {
    m_allocator = name;
}

void allocator::buddy_allocator::reset() {
    if (m_ownsMemory) {
        // Clear allocated buddies
        allocatedBuddies.clear();

        // Clear free lists
        for (auto& list : freeLists) {
            list = nullptr;
        }

#if ALLOCATOR_DEBUG
        std::memset(m_buffer.start_address, 0,
                    m_buffer.size); // Optional: Clear memory for debugging
#endif

        // Reinitialize the free list with a single large block
        int level = get_level(m_buffersize);
        Buddy* buddy = reinterpret_cast<Buddy*>(m_buffer.start_address);
        add_to_free_list(buddy, level);
    } else {
        allocate_new_buffer();
    }
}

void allocator::buddy_allocator::releaseMemory() {
    m_buffer.memory.reset();
    m_buffer.size = 0;
    m_buffer.start_address = nullptr;
    m_ownsMemory = false;

    // Clear allocated buddies
    allocatedBuddies.clear();

    // Clear free lists
    for (auto& list : freeLists) {
        list = nullptr;
    }

#if ALLOCATOR_DEBUG
    std::memset(m_buffer.start_address, 0, m_buffer.size); // Optional: Clear memory for debugging
#endif
}

void allocator::buddy_allocator::allocate_new_buffer() {
    if (m_ownsMemory) {
        releaseMemory();
    }

    m_buffer.memory = std::make_unique<std::byte[]>(m_buffersize);
    int level = get_level(m_buffersize);

    m_buffer.size = m_buffersize;
    m_buffer.start_address = m_buffer.memory.get();
    m_buffer.initial_level = level;
    m_buffer.start_address_int = reinterpret_cast<uintptr_t>(m_buffer.start_address);
    m_ownsMemory = true;

    // Initialize the free list with a single large block
    Buddy* buddy = reinterpret_cast<Buddy*>(m_buffer.start_address);
    add_to_free_list(buddy, level);
}

int allocator::buddy_allocator::get_level(size_t size) {
    // level 0 is 1KB, level 1 is 2KB, ..., level 17 is 128MB
    return log2(size / MIN_CAPACITY);
}

size_t allocator::buddy_allocator::get_power_of_two(size_t size) {
    // bit twiddling to get next power of two
    if (size <= MIN_CAPACITY)
        return MIN_CAPACITY;

    size_t power = 1;
    while (power < size)
        power <<= 1;
    return power;
}

size_t allocator::buddy_allocator::get_level_size(int level) {
    return MIN_CAPACITY << level; // 1KB * 2^level
}

void allocator::buddy_allocator::add_to_free_list(Buddy* buddy, int level) {
    buddy->next_free = freeLists[level];
    freeLists[level] = buddy;
}

void allocator::buddy_allocator::remove_from_free_list(Buddy* buddy, int level) {
    if (!freeLists[level])
        return;

    if (freeLists[level] == buddy) {
        freeLists[level] = buddy->next_free;
        return;
    }

    Buddy* current = freeLists[level];
    while (current && current->next_free != buddy) {
        current = current->next_free;
    }

    if (current && current->next_free == buddy) {
        current->next_free = buddy->next_free;
    } else {
        throw std::runtime_error("Attempted to remove a buddy not in free list");
    }
}

allocator::buddy_allocator::Buddy* allocator::buddy_allocator::get_first_free_buddy(int level) {
    if (!freeLists[level])
        return nullptr;

    Buddy* buddy = freeLists[level];
    remove_from_free_list(buddy, level);
    return buddy;
}

int allocator::buddy_allocator::find_non_empty_level(int startLevel) {

    int freeListsSize = static_cast<int>(freeLists.size());

    for (int i = startLevel; i < freeListsSize; ++i) {
        if (freeLists[i] != nullptr) {
            return i;
        }
    }
    return -1; // No non-empty level found
}

allocator::buddy_allocator::Buddy* allocator::buddy_allocator::split_buddy(Buddy* b, int level) {
    size_t halfSize = get_level_size(level) / 2;
    Buddy* buddy1 = b;
    assert(halfSize % alignof(Buddy) == 0);
    Buddy* buddy2 = reinterpret_cast<Buddy*>(reinterpret_cast<std::byte*>(b) + halfSize);
    add_to_free_list(buddy2, level - 1);
    return buddy1; // return the first half to be added to free list
}

allocator::buddy_allocator::Buddy* allocator::buddy_allocator::find_buddy(Buddy* b, int level) {

    if (level == m_buffer.initial_level) {
        return nullptr; // No buddy exists for the largest block
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(m_buffer.start_address);
    uintptr_t addr = reinterpret_cast<uintptr_t>(b);
    size_t block_size = get_level_size(level);

    uintptr_t offset = addr - base;
    uintptr_t buddy_offset = offset ^ block_size;

    if (buddy_offset >= m_buffersize) {
        return nullptr; // Buddy is out of bounds
    }

    return reinterpret_cast<Buddy*>(base + buddy_offset);
}

void allocator::buddy_allocator::try_merge_buddies(Buddy* buddy, int level) {
    Buddy* buddyPair = find_buddy(buddy, level);
    if (buddyPair) {
        // Merge the buddies
        remove_from_free_list(buddy, level);
        remove_from_free_list(buddyPair, level);

        Buddy* merged_buddy = std::min(buddy, buddyPair);

        add_to_free_list(merged_buddy, level + 1);
        try_merge_buddies(merged_buddy, level + 1);
    }
}