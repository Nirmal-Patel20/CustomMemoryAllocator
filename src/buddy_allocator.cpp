#include "allocator/buddy_allocator.hpp"
#include <cmath>
#include <cstring>

#if ALLOCATOR_DEBUG
#define handle_allocation_error(msg) throwAllocationError(m_allocator, msg)
#else
#define handle_allocation_error(msg) return nullptr
#endif

allocator::buddy_allocator::buddy_allocator(size_t buffersize) {
    if (buffersize < MIN_CAPACITY || buffersize > MAX_CAPACITY) {
        throwAllocationError(m_allocator, "Buffer size must be between" +
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

    if (size > m_buffersize) {
        handle_allocation_error("Requested size exceeds buffer size");
    }

    // Find the appropriate free block
    int level = get_level(size);
    Buddy* buddy = get_first_free_buddy(level);
    if (!buddy) {
        // No free block at this level, try to find a larger block and split it
        int nonEmptyLevel = find_non_empty_level(level + 1);
        if (nonEmptyLevel == -1) {
            handle_allocation_error("Out of memory");
        }

        // Split blocks down to the desired level
        while (nonEmptyLevel > level) {
            Buddy* largerBuddy = nullptr;

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

    auto it = allocatedBuddies.find(ptr);
    if (it == allocatedBuddies.end()) {
        throwAllocationError(m_allocator, "Pointer not allocated by this allocator");
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
    m_buffer.size = m_buffersize;
    m_buffer.start_address = m_buffer.memory.get();
    m_ownsMemory = true;

    // Initialize the free list with a single large block
    int level = get_level(m_buffersize);
    Buddy* buddy = reinterpret_cast<Buddy*>(m_buffer.start_address);
    add_to_free_list(buddy, level);
}

int allocator::buddy_allocator::get_level(size_t size) {
    // level 0 is 1KB, level 1 is 2KB, ..., level 17 is 128MB
    return log2(size / MIN_CAPACITY);
}

size_t allocator::buddy_allocator::get_power_of_two(size_t size) {
    // bit twiddling to get next power of two
    if (size <= 1)
        return 1;
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
    }
}

allocator::buddy_allocator::Buddy* allocator::buddy_allocator::get_first_free_buddy(int level) {
    if (!freeLists[level])
        return nullptr;

    Buddy* buddy = freeLists[level];
    freeLists[level] = buddy->next_free;
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
    Buddy* buddy2 = reinterpret_cast<Buddy*>(reinterpret_cast<std::byte*>(b) + halfSize);
    add_to_free_list(buddy1, level - 1);
    return buddy2; // return the second half to be added to free list
}

allocator::buddy_allocator::Buddy* allocator::buddy_allocator::find_buddy(Buddy* b, int level) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(b);
    size_t blockSize = get_level_size(level);
    uintptr_t buddyAddr = addr ^ blockSize;
    if (allocatedBuddies.find(reinterpret_cast<void*>(buddyAddr)) != allocatedBuddies.end()) {
        return nullptr; // Buddy is allocated
    }
    return reinterpret_cast<Buddy*>(buddyAddr);
}

void allocator::buddy_allocator::try_merge_buddies(Buddy* buddy, int level) {
    Buddy* buddyPair = find_buddy(buddy, level);
    if (buddyPair) {
        // Merge the buddies
        remove_from_free_list(buddy, level);
        remove_from_free_list(buddyPair, level);
        add_to_free_list(reinterpret_cast<Buddy*>(std::min(reinterpret_cast<uintptr_t>(buddy),
                                                           reinterpret_cast<uintptr_t>(buddyPair))),
                         level + 1);
    }
}