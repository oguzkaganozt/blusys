// operational_phase_smoke — host-only assertions for the headless-telemetry-style
// operational phase machine used by the validation examples.

#include <cstdint>
#include <cstdlib>

namespace {

enum class op_state : std::uint8_t {
    idle,
    provisioning,
    connecting,
    connected,
    reporting,
    updating,
};

struct app_state {
    bool ota_in_progress = false;
    bool provisioned     = false;
    bool conn_ready      = false;
    bool has_ip          = false;
    bool time_synced     = false;
};

op_state compute_phase(const app_state &state)
{
    if (state.ota_in_progress) {
        return op_state::updating;
    }
    if (!state.provisioned && !state.conn_ready) {
        return op_state::provisioning;
    }
    if (!state.has_ip) {
        return op_state::connecting;
    }
    if (state.has_ip && state.time_synced && state.conn_ready) {
        return op_state::reporting;
    }
    if (state.has_ip) {
        return op_state::connected;
    }
    return op_state::idle;
}

bool run_tests()
{
    app_state s{};

    if (compute_phase(s) != op_state::provisioning) {
        return false;
    }

    s.provisioned = true;
    if (compute_phase(s) != op_state::connecting) {
        return false;
    }

    s.has_ip = true;
    if (compute_phase(s) != op_state::connected) {
        return false;
    }

    s.time_synced = true;
    s.conn_ready  = false;
    if (compute_phase(s) != op_state::connected) {
        return false;
    }

    s.conn_ready = true;
    if (compute_phase(s) != op_state::reporting) {
        return false;
    }

    s.ota_in_progress = true;
    if (compute_phase(s) != op_state::updating) {
        return false;
    }

    return true;
}

}  // namespace

int main()
{
    return run_tests() ? EXIT_SUCCESS : EXIT_FAILURE;
}
