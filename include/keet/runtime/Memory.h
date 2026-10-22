//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Monotonic Arena Allocator & Memory Primitives
//

#ifndef KEET_RUNTIME_MEMORY_H
#define KEET_RUNTIME_MEMORY_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <new>
#include <type_traits>
#include <algorithm>

namespace keet::mem {

    // =========================================================================
    // Memory Alignment Helper
    // =========================================================================

    [[nodiscard]] constexpr size_t alignUp(size_t offset, size_t alignment) noexcept {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    [[nodiscard]] inline uintptr_t alignPointer(uintptr_t ptr, size_t alignment) noexcept {
        return (ptr + alignment - 1) & ~(static_cast<uintptr_t>(alignment - 1));
    }

    // =========================================================================
    // Arena Snapshot Marker (for Scoped Rollbacks)
    // =========================================================================

    struct ArenaMarker {
        size_t chunkIndex{0};
        size_t chunkOffset{0};
        size_t dtorIndex{0};
        size_t allocatedBytes{0};
    };

    // =========================================================================
    // Arena Class (High-Performance Monotonic Bump Allocator)
    // =========================================================================

    class Arena {
        public:
            static constexpr size_t DEFAULT_CHUNK_SIZE = 64 * 1024; // 64 KB

            explicit Arena(size_t defaultChunkSize = DEFAULT_CHUNK_SIZE);
            ~Arena();

            Arena(const Arena&) = delete;
            Arena& operator=(const Arena&) = delete;
            Arena(Arena&& other) noexcept;
            Arena& operator=(Arena&& other) noexcept;

            // Raw allocation
            [[nodiscard]] void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t));

            // Strongly-typed object creation
            template <typename T, typename... Args>
            [[nodiscard]] T* create(Args&&... args) {
                void* mem = allocate(sizeof(T), alignof(T));
                T* obj = new (mem) T(std::forward<Args>(args)...);

                if constexpr (!std::is_trivially_destructible_v<T>) {
                    registerDestructor(obj, [](void* ptr) {
                        static_cast<T*>(ptr)->~T();
                    });
                }
                return obj;
            }

            // Strongly-typed array allocation (trivially constructible or default)
            template <typename T>
            [[nodiscard]] T* allocateArray(size_t count) {
                if (count == 0) return nullptr;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    size_t headerSize = alignUp(sizeof(size_t), alignof(T));
                    void* mem = allocate(headerSize + sizeof(T) * count, alignof(T));
                    *reinterpret_cast<size_t*>(mem) = count;
                    T* elements = reinterpret_cast<T*>(static_cast<uint8_t*>(mem) + headerSize);
                    for (size_t i = 0; i < count; ++i) {
                        new (&elements[i]) T();
                    }
                    registerDestructor(mem, [](void* ptr) {
                        size_t cnt = *reinterpret_cast<size_t*>(ptr);
                        size_t hdr = alignUp(sizeof(size_t), alignof(T));
                        T* arr = reinterpret_cast<T*>(static_cast<uint8_t*>(ptr) + hdr);
                        for (size_t i = 0; i < cnt; ++i) {
                            arr[i].~T();
                        }
                    });
                    return elements;
                } else {
                    void* mem = allocate(sizeof(T) * count, alignof(T));
                    if constexpr (!std::is_trivially_default_constructible_v<T>) {
                        T* elements = static_cast<T*>(mem);
                        for (size_t i = 0; i < count; ++i) {
                            new (&elements[i]) T();
                        }
                        return elements;
                    } else {
                        return static_cast<T*>(mem);
                    }
                }
            }

            // Custom destructor registration
            void registerDestructor(void* ptr, void (*destructor)(void*));

            // Scoped snapshots and rollbacks
            [[nodiscard]] ArenaMarker getMarker() const noexcept;
            void rollback(const ArenaMarker& marker);

            // Resets all allocations and runs registered destructors, retaining allocated chunk buffers for reuse
            void reset();

            // Purges all chunks back to the OS
            void purge();

            // Inspection metrics
            [[nodiscard]] size_t totalAllocated() const noexcept { return _totalAllocated; }
            [[nodiscard]] size_t totalReserved() const noexcept;
            [[nodiscard]] size_t chunkCount() const noexcept;

        private:
            struct Chunk {
                uint8_t* data{nullptr};
                size_t capacity{0};
                size_t used{0};
                Chunk* next{nullptr};
            };

            struct DtorEntry {
                void* object{nullptr};
                void (*destructor)(void*){nullptr};
            };

            Chunk* _allocateChunk(size_t minCapacity);

            size_t _defaultChunkSize;
            Chunk* _head{nullptr};
            Chunk* _current{nullptr};
            size_t _totalAllocated{0};

            std::vector<DtorEntry> _dtors;
    };

    // =========================================================================
    // ScopedArena (RAII Watermark Rollback)
    // =========================================================================

    class ScopedArena {
        public:
            explicit ScopedArena(Arena& arena)
                : _arena(arena), _marker(arena.getMarker()) {}

            ~ScopedArena() {
                _arena.rollback(_marker);
            }

            ScopedArena(const ScopedArena&) = delete;
            ScopedArena& operator=(const ScopedArena&) = delete;

            [[nodiscard]] Arena& arena() noexcept { return _arena; }

        private:
            Arena& _arena;
            ArenaMarker _marker;
    };

    // =========================================================================
    // STL Allocator Adapter (ArenaAllocator)
    // =========================================================================

    template <typename T>
    class ArenaAllocator {
        public:
            using value_type = T;

            ArenaAllocator() noexcept : _arena(nullptr) {}
            explicit ArenaAllocator(Arena& arena) noexcept : _arena(&arena) {}

            template <typename U>
            ArenaAllocator(const ArenaAllocator<U>& other) noexcept : _arena(other._arena) {}

            [[nodiscard]] T* allocate(size_t n) {
                if (!_arena) {
                    throw std::bad_alloc();
                }
                return static_cast<T*>(_arena->allocate(n * sizeof(T), alignof(T)));
            }

            void deallocate(T*, size_t) noexcept {
                // Arena allocations are reclaimed in bulk on reset/rollback
            }

            template <typename U>
            bool operator==(const ArenaAllocator<U>& other) const noexcept {
                return _arena == other._arena;
            }

            template <typename U>
            bool operator!=(const ArenaAllocator<U>& other) const noexcept {
                return _arena != other._arena;
            }

            Arena* _arena{nullptr};
    };

} // namespace keet::mem

namespace keet::memory {
    using namespace keet::mem;
}

#endif // KEET_RUNTIME_MEMORY_H
