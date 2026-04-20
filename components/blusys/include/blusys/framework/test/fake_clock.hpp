/* blusys/framework/test/fake_clock.hpp — deterministic monotonic u32 clock.
 *
 * Tests drive time forward explicitly instead of waiting on a real timer. The
 * fake_clock sits between the test harness and any subsystem that wants to
 * observe "now", so a test can write `clock.advance(250)` and every subsequent
 * `now_ms()` sees the new time. This is the reason headless tests are fast
 * and reproducible — there is no sleeping anywhere in the loop.
 *
 * Header-only. Thread-unsafe by design: reducer + tests run on one task.
 */

#ifndef BLUSYS_FRAMEWORK_TEST_FAKE_CLOCK_HPP
#define BLUSYS_FRAMEWORK_TEST_FAKE_CLOCK_HPP

#include <cstdint>

namespace blusys::test {

class fake_clock {
public:
    fake_clock() = default;
    explicit fake_clock(std::uint32_t start_ms) : now_ms_(start_ms) {}

    [[nodiscard]] std::uint32_t now_ms() const { return now_ms_; }

    void advance(std::uint32_t delta_ms) { now_ms_ += delta_ms; }
    void set(std::uint32_t new_ms)       { now_ms_ = new_ms; }

private:
    std::uint32_t now_ms_ = 0;
};

}  // namespace blusys::test

#endif  // BLUSYS_FRAMEWORK_TEST_FAKE_CLOCK_HPP
