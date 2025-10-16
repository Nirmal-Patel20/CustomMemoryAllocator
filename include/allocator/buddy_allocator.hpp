#ifndef BUDDY_ALLOCATOR_HPP
#define BUDDY_ALLOCATOR_HPP

#include "allocator/allocator_interface.hpp"
#include <array>
#include <list>
#include <unordered_map>

namespace allocator {
class buddy_allocator : public allocator::AllocatorInterface {
  public:
    buddy_allocator(size_t bufferSize);
    ~buddy_allocator() override;

    [[nodiscard]] virtual void* allocate(size_t size, // here alignment is ignored
                                         [[maybe_unused]] size_t alignment = 0) override;
    virtual void deallocate(void* ptr) override;
    virtual size_t getAllocatedSize() const override;
    virtual size_t getObjectSize() const override { return 0; } // not tracked;
    virtual void reset() override;
    virtual void setAllocatorName(std::string_view name) override;
    void releaseMemory();

    // disable copy and move
    buddy_allocator(const buddy_allocator&) = delete;
    buddy_allocator& operator=(const buddy_allocator&) = delete;
    buddy_allocator(buddy_allocator&&) = delete;
    buddy_allocator& operator=(buddy_allocator&&) = delete;

  private:
    // helper functions
    static int get_level(size_t size);
    static size_t get_power_of_two(size_t size);
    static size_t get_level_size(int level); // size of block at given level

    struct Buddy {
        Buddy* next_free = nullptr;
    };

    // freelist for each level
    std::array<Buddy*, 18>
        freeLists{}; // from 1KB to 128MB (level 0 to level 17, which is 2^10 to 2^27)
    std::unordered_map<void*, int> allocatedBuddies; // map of allocated buddies and their levels

    // freelist helper functions
    void add_to_free_list(Buddy* buddy, int level);
    void remove_from_free_list(Buddy* buddy, int level);
    Buddy* get_first_free_buddy(int level);
    int find_non_empty_level(int startLevel);

    // buddy helper functions
    Buddy* split_buddy(Buddy* b, int level);
    void try_merge_buddies(Buddy* buddy, int level);
    Buddy* find_buddy(Buddy* b, int level);

    // Pre-allocated memory buffer
    struct buffer {
        std::unique_ptr<std::byte[]> memory; // Contiguous memory
        size_t size = 0;                     // Total buffer size
        void* start_address = nullptr;       // Starting address of the buffer
    } m_buffer;

    void allocate_new_buffer();
    size_t m_buffersize;
    bool m_ownsMemory = false;
    static constexpr size_t MIN_CAPACITY = 1024;                 // 1KB
    static constexpr size_t MAX_CAPACITY = 128ull * 1024 * 1024; // 128 MB
    std::string m_allocator = "buddy_allocator";                 // Custom Name for debugging
};
} // namespace allocator

#endif // BUDDY_ALLOCATOR_HPP