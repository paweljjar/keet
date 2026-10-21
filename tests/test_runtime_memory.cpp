//
// Created by PAWEL on 21/10/2026.
// libkeet Minimal Language Base Runtime - Monotonic Arena Allocator Unit Tests
//

#include <keet/Testing.h>
#include <keet/runtime/Memory.h>
#include <vector>
#include <string>

using namespace keet::mem;

namespace {
    struct DestructorCounter {
        static inline int aliveCount = 0;
        int value{0};

        explicit DestructorCounter(int v = 0) : value(v) {
            aliveCount++;
        }

        ~DestructorCounter() {
            aliveCount--;
        }
    };
} // anonymous namespace

KEET_TEST(RuntimeMemory, RawAllocationAndAlignment) {
    Arena arena(1024); // 1 KB chunks

    void* p1 = arena.allocate(17, 8);
    KEET_ASSERT_TRUE(p1 != nullptr);
    KEET_ASSERT_EQ(reinterpret_cast<uintptr_t>(p1) % 8, 0);

    void* p2 = arena.allocate(33, 16);
    KEET_ASSERT_TRUE(p2 != nullptr);
    KEET_ASSERT_EQ(reinterpret_cast<uintptr_t>(p2) % 16, 0);

    void* p3 = arena.allocate(128, 64);
    KEET_ASSERT_TRUE(p3 != nullptr);
    KEET_ASSERT_EQ(reinterpret_cast<uintptr_t>(p3) % 64, 0);

    KEET_ASSERT_TRUE(arena.totalAllocated() >= (17 + 33 + 128));
}

KEET_TEST(RuntimeMemory, TypedObjectCreationAndDestruction) {
    DestructorCounter::aliveCount = 0;
    {
        Arena arena;
        auto* obj1 = arena.create<DestructorCounter>(42);
        auto* obj2 = arena.create<DestructorCounter>(84);

        KEET_ASSERT_EQ(obj1->value, 42);
        KEET_ASSERT_EQ(obj2->value, 84);
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 2);

        // Reset should invoke destructors
        arena.reset();
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 0);
        KEET_ASSERT_EQ(arena.totalAllocated(), 0);

        // Allocate again after reset
        auto* obj3 = arena.create<DestructorCounter>(126);
        KEET_ASSERT_EQ(obj3->value, 126);
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 1);
    }
    // Arena destructor should purge and invoke remaining destructors
    KEET_ASSERT_EQ(DestructorCounter::aliveCount, 0);
}

KEET_TEST(RuntimeMemory, ArrayAllocation) {
    DestructorCounter::aliveCount = 0;
    {
        Arena arena;
        DestructorCounter* arr = arena.allocateArray<DestructorCounter>(10);
        KEET_ASSERT_TRUE(arr != nullptr);
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 10);

        for (int i = 0; i < 10; ++i) {
            arr[i].value = i * 10;
        }

        KEET_ASSERT_EQ(arr[5].value, 50);

        arena.reset();
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 0);
    }
    KEET_ASSERT_EQ(DestructorCounter::aliveCount, 0);
}

KEET_TEST(RuntimeMemory, MultiChunkGrowth) {
    // 512 bytes per chunk
    Arena arena(512);
    KEET_ASSERT_EQ(arena.chunkCount(), 0);

    // Allocate 300 bytes (chunk 1)
    void* m1 = arena.allocate(300);
    KEET_ASSERT_TRUE(m1 != nullptr);
    KEET_ASSERT_EQ(arena.chunkCount(), 1);

    // Allocate 300 bytes (forces chunk 2)
    void* m2 = arena.allocate(300);
    KEET_ASSERT_TRUE(m2 != nullptr);
    KEET_ASSERT_EQ(arena.chunkCount(), 2);

    // Allocate 2000 bytes (forces a custom larger chunk 3)
    void* m3 = arena.allocate(2000);
    KEET_ASSERT_TRUE(m3 != nullptr);
    KEET_ASSERT_EQ(arena.chunkCount(), 3);
    KEET_ASSERT_TRUE(arena.totalReserved() >= 2600);

    // Reset keeps reserved capacity
    arena.reset();
    KEET_ASSERT_EQ(arena.totalAllocated(), 0);
    KEET_ASSERT_EQ(arena.chunkCount(), 3);

    // Purge returns everything to OS
    arena.purge();
    KEET_ASSERT_EQ(arena.chunkCount(), 0);
    KEET_ASSERT_EQ(arena.totalReserved(), 0);
}

KEET_TEST(RuntimeMemory, ScopedArenaRollback) {
    DestructorCounter::aliveCount = 0;
    Arena arena;

    auto* outerObj = arena.create<DestructorCounter>(1);
    KEET_ASSERT_EQ(outerObj->value, 1);
    KEET_ASSERT_EQ(DestructorCounter::aliveCount, 1);
    size_t outerAllocated = arena.totalAllocated();

    {
        ScopedArena scope(arena);
        auto* innerObj1 = arena.create<DestructorCounter>(2);
        auto* innerObj2 = arena.create<DestructorCounter>(3);

        KEET_ASSERT_EQ(innerObj1->value, 2);
        KEET_ASSERT_EQ(innerObj2->value, 3);
        KEET_ASSERT_EQ(DestructorCounter::aliveCount, 3);
        KEET_ASSERT_TRUE(arena.totalAllocated() > outerAllocated);
    }

    // Exited scope: inner objects destructed and memory reclaimed
    KEET_ASSERT_EQ(DestructorCounter::aliveCount, 1);
    KEET_ASSERT_EQ(outerObj->value, 1);
    KEET_ASSERT_EQ(arena.totalAllocated(), outerAllocated);

    arena.purge();
    KEET_ASSERT_EQ(DestructorCounter::aliveCount, 0);
}

KEET_TEST(RuntimeMemory, STLContainerAdapter) {
    Arena arena;
    ArenaAllocator<int> alloc(arena);

    std::vector<int, ArenaAllocator<int>> vec(alloc);
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i * 2);
    }

    KEET_ASSERT_EQ(vec.size(), 100);
    KEET_ASSERT_EQ(vec[0], 0);
    KEET_ASSERT_EQ(vec[50], 100);
    KEET_ASSERT_EQ(vec[99], 198);
    KEET_ASSERT_TRUE(arena.totalAllocated() > 0);
}

KEET_TEST_MAIN()
