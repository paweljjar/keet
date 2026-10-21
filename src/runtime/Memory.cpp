//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Monotonic Arena Allocator & Memory Primitives
//

#include "keet/runtime/Memory.h"
#include <algorithm>

namespace keet::mem {

    Arena::Arena(size_t defaultChunkSize)
        : _defaultChunkSize(defaultChunkSize > 0 ? defaultChunkSize : DEFAULT_CHUNK_SIZE) {
    }

    Arena::~Arena() {
        purge();
    }

    Arena::Arena(Arena&& other) noexcept
        : _defaultChunkSize(other._defaultChunkSize),
          _head(other._head),
          _current(other._current),
          _totalAllocated(other._totalAllocated),
          _dtors(std::move(other._dtors)) {
        other._head = nullptr;
        other._current = nullptr;
        other._totalAllocated = 0;
    }

    Arena& Arena::operator=(Arena&& other) noexcept {
        if (this != &other) {
            purge();
            _defaultChunkSize = other._defaultChunkSize;
            _head = other._head;
            _current = other._current;
            _totalAllocated = other._totalAllocated;
            _dtors = std::move(other._dtors);

            other._head = nullptr;
            other._current = nullptr;
            other._totalAllocated = 0;
        }
        return *this;
    }

    Arena::Chunk* Arena::_allocateChunk(size_t minCapacity) {
        size_t cap = std::max(_defaultChunkSize, minCapacity);
        auto* chunk = new Chunk();
        chunk->data = new uint8_t[cap];
        chunk->capacity = cap;
        chunk->used = 0;
        chunk->next = nullptr;
        return chunk;
    }

    void* Arena::allocate(size_t bytes, size_t alignment) {
        if (bytes == 0) return nullptr;
        if (alignment == 0) alignment = alignof(std::max_align_t);

        if (!_current) {
            if (!_head) {
                _head = _allocateChunk(bytes + alignment);
                _current = _head;
            } else {
                _current = _head;
            }
        }

        while (true) {
            uintptr_t currentAddr = reinterpret_cast<uintptr_t>(_current->data + _current->used);
            uintptr_t alignedAddr = alignPointer(currentAddr, alignment);
            size_t neededOffset = alignedAddr - reinterpret_cast<uintptr_t>(_current->data);

            if (neededOffset + bytes <= _current->capacity) {
                _current->used = neededOffset + bytes;
                _totalAllocated += bytes;
                return reinterpret_cast<void*>(alignedAddr);
            }

            // Current chunk does not have enough capacity
            if (_current->next) {
                _current = _current->next;
                _current->used = 0;
                uintptr_t nextAddr = reinterpret_cast<uintptr_t>(_current->data);
                uintptr_t nextAligned = alignPointer(nextAddr, alignment);
                size_t offset = nextAligned - nextAddr;
                if (offset + bytes <= _current->capacity) {
                    continue;
                }
            }

            // Allocate a new chunk and append it after _current
            Chunk* newChunk = _allocateChunk(bytes + alignment);
            newChunk->next = _current->next;
            _current->next = newChunk;
            _current = newChunk;
        }
    }

    void Arena::registerDestructor(void* ptr, void (*destructor)(void*)) {
        if (ptr && destructor) {
            _dtors.push_back({ptr, destructor});
        }
    }

    ArenaMarker Arena::getMarker() const noexcept {
        ArenaMarker marker;
        marker.dtorIndex = _dtors.size();
        marker.allocatedBytes = _totalAllocated;

        size_t index = 0;
        Chunk* c = _head;
        while (c) {
            if (c == _current) {
                marker.chunkIndex = index;
                marker.chunkOffset = c->used;
                break;
            }
            c = c->next;
            index++;
        }
        return marker;
    }

    void Arena::rollback(const ArenaMarker& marker) {
        // Run destructors for objects created after the marker
        while (_dtors.size() > marker.dtorIndex) {
            auto& entry = _dtors.back();
            if (entry.destructor && entry.object) {
                entry.destructor(entry.object);
            }
            _dtors.pop_back();
        }

        // Locate marker chunk
        size_t index = 0;
        Chunk* c = _head;
        while (c && index < marker.chunkIndex) {
            c = c->next;
            index++;
        }

        if (c) {
            _current = c;
            _current->used = marker.chunkOffset;

            // Reset any subsequent chunks
            Chunk* nextChunk = _current->next;
            while (nextChunk) {
                nextChunk->used = 0;
                nextChunk = nextChunk->next;
            }
        }

        _totalAllocated = marker.allocatedBytes;
    }

    void Arena::reset() {
        for (auto it = _dtors.rbegin(); it != _dtors.rend(); ++it) {
            if (it->destructor && it->object) {
                it->destructor(it->object);
            }
        }
        _dtors.clear();

        Chunk* c = _head;
        while (c) {
            c->used = 0;
            c = c->next;
        }

        _current = _head;
        _totalAllocated = 0;
    }

    void Arena::purge() {
        for (auto it = _dtors.rbegin(); it != _dtors.rend(); ++it) {
            if (it->destructor && it->object) {
                it->destructor(it->object);
            }
        }
        _dtors.clear();

        Chunk* c = _head;
        while (c) {
            Chunk* next = c->next;
            delete[] c->data;
            delete c;
            c = next;
        }

        _head = nullptr;
        _current = nullptr;
        _totalAllocated = 0;
    }

    size_t Arena::totalReserved() const noexcept {
        size_t total = 0;
        Chunk* c = _head;
        while (c) {
            total += c->capacity;
            c = c->next;
        }
        return total;
    }

    size_t Arena::chunkCount() const noexcept {
        size_t count = 0;
        Chunk* c = _head;
        while (c) {
            count++;
            c = c->next;
        }
        return count;
    }

} // namespace keet::mem
