// Minimal host-side tests for framework header-only / core primitives (no LVGL).
// Run: ctest -R host_spine_test (after building scripts/host).

#include "blusys/framework/core/containers.hpp"

#include <cstdio>

namespace {

bool test_ring_buffer()
{
    blusys::ring_buffer<int, 4> rb;
    int                           v = 0;

    for (int i = 0; i < 4; ++i) {
        if (!rb.push_back(i)) {
            std::fprintf(stderr, "host_spine_test: push_back should succeed at %d\n", i);
            return false;
        }
    }
    if (rb.push_back(99)) {
        std::fputs("host_spine_test: push_back should fail when full\n", stderr);
        return false;
    }

    for (int expect = 0; expect < 4; ++expect) {
        if (!rb.pop_front(&v) || v != expect) {
            std::fprintf(stderr, "host_spine_test: pop_front expected %d\n", expect);
            return false;
        }
    }
    if (rb.pop_front(&v)) {
        std::fputs("host_spine_test: pop_front should fail when empty\n", stderr);
        return false;
    }

    rb.clear();
    if (!rb.push_back(7) || !rb.pop_front(&v) || v != 7) {
        std::fputs("host_spine_test: clear + single push/pop\n", stderr);
        return false;
    }
    return true;
}

}  // namespace

int main()
{
    if (!test_ring_buffer()) {
        return 1;
    }
    return 0;
}
