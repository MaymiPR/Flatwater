#pragma once
// For the test programs only: counts the heap allocations the calling thread makes through operator new while an
// AllocScope lives, so a test can say that a processBlock call allocated nothing.
//
//   flatwater::test::AllocScope scope;
//   proc.processBlock (buffer, midi);
//   CHECK (scope.count() == 0);
//
// This header replaces the global operator new and operator delete of the program, so one source file of a test
// program includes it, and nothing that ships does. The array and the nothrow forms reach the replaced pair through
// the standard library's defaults. What it does not see: std::malloc called directly (juce::HeapBlock, so a
// juce::AudioBuffer's setSize) and over-aligned new.
#include <cstddef>
#include <cstdlib>
#include <new>

namespace flatwater::test
{
inline thread_local bool allocArmed = false;
inline thread_local int allocCount = 0;

// counts from its construction to its destruction, on the thread that made it
class AllocScope
{
public:
    AllocScope() { allocCount = 0; allocArmed = true; }
    ~AllocScope() { allocArmed = false; }
    AllocScope (const AllocScope&) = delete;
    AllocScope& operator= (const AllocScope&) = delete;
    int count() const { return allocCount; }
};
}

void* operator new (std::size_t size)
{
    if (flatwater::test::allocArmed) ++flatwater::test::allocCount;
    if (void* p = std::malloc (size != 0 ? size : 1)) return p;
    throw std::bad_alloc();
}

void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
