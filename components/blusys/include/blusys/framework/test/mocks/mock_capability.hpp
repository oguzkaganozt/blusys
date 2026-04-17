/* blusys/framework/test/mocks/mock_capability.hpp — canonical capability mock.
 *
 * One template, one shape, three canned instantiations. Each mock records
 * the sequence of lifecycle calls (start / poll / stop) and lets tests emit
 * synthetic integration events through the real post_integration_event()
 * pipeline — so a test can exercise `on_event` end to end without
 * standing up a real capability backend.
 *
 * Threading: identical to any real capability. start/poll/stop run on the
 * reducer thread; emit_integration_event must also run there in tests (the
 * P0c harness has no worker threads). See docs/internals/threading.md.
 */

#ifndef BLUSYS_FRAMEWORK_TEST_MOCKS_MOCK_CAPABILITY_HPP
#define BLUSYS_FRAMEWORK_TEST_MOCKS_MOCK_CAPABILITY_HPP

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

namespace blusys::test {

struct mock_storage_status : capability_status_base {
    bool spiffs_mounted = false;
    bool fatfs_mounted  = false;
};

struct mock_diagnostics_status : capability_status_base {
    std::uint32_t uptime_ms = 0;
    std::uint32_t free_heap = 0;
};

struct mock_telemetry_status : capability_status_base {
    std::uint32_t buffered_count   = 0;
    std::uint32_t total_delivered  = 0;
};

template <blusys::capability_kind Kind, typename Status>
class mock_capability final : public blusys::capability_base {
public:
    static constexpr blusys::capability_kind kind_value = Kind;

    [[nodiscard]] blusys::capability_kind kind() const override { return Kind; }

    blusys_err_t start(blusys::runtime &rt) override
    {
        rt_ = &rt;
        ++start_calls;
        status.capability_ready = true;
        return BLUSYS_OK;
    }

    void poll(std::uint32_t now_ms) override
    {
        ++poll_calls;
        last_poll_ms = now_ms;
    }

    void stop() override
    {
        ++stop_calls;
        status.capability_ready = false;
        rt_ = nullptr;
    }

    /* Test helper: emit an integration event as if a real backend produced
     * one. Routes through the base class's post_integration_event so the
     * event goes into the same queue the real runtime drains. */
    void emit(std::uint32_t event_id,
              std::uint32_t event_code = 0,
              const void   *payload = nullptr)
    {
        post_integration_event(event_id, event_code, payload);
    }

    /* Observable state — tests read these directly. */
    Status        status{};
    std::uint32_t start_calls  = 0;
    std::uint32_t poll_calls   = 0;
    std::uint32_t stop_calls   = 0;
    std::uint32_t last_poll_ms = 0;
};

using storage_mock     = mock_capability<blusys::capability_kind::storage,     mock_storage_status>;
using diagnostics_mock = mock_capability<blusys::capability_kind::diagnostics, mock_diagnostics_status>;
using telemetry_mock   = mock_capability<blusys::capability_kind::telemetry,   mock_telemetry_status>;

}  // namespace blusys::test

#endif  // BLUSYS_FRAMEWORK_TEST_MOCKS_MOCK_CAPABILITY_HPP
