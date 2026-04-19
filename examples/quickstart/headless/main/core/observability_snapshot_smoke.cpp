#include <blusys/blusys.h>

#include <cstring>

int main()
{
    blusys_observe_clear();

    BLUSYS_LOG(BLUSYS_LOG_ERROR, err_domain_framework_observe, "snapshot test %d", 42);
    blusys_counter_inc(err_domain_framework_observe, 7u);

    blusys_observe_snapshot_t snapshot{};
    blusys_observe_snapshot(&snapshot);

    if (snapshot.log_count == 0u) {
        return 1;
    }
    if (!snapshot.last_errors[err_domain_framework_observe].valid) {
        return 2;
    }
    if (snapshot.counters.values[err_domain_framework_observe] < 8u) {
        return 3;
    }

    blusys_observe_clear();
    blusys_observe_snapshot_t restored{};
    blusys_observe_restore(&snapshot);
    blusys_observe_snapshot(&restored);

    if (restored.log_count != snapshot.log_count) {
        return 4;
    }
    if (restored.counters.values[err_domain_framework_observe] !=
        snapshot.counters.values[err_domain_framework_observe]) {
        return 5;
    }
    if (!restored.last_errors[err_domain_framework_observe].valid) {
        return 6;
    }
    if (std::strcmp(restored.last_errors[err_domain_framework_observe].text,
                    snapshot.last_errors[err_domain_framework_observe].text) != 0) {
        return 7;
    }

    return 0;
}
