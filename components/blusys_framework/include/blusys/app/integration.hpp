#pragma once

// Optional helpers for capability integration (map_event wiring, typed event IDs) and
// std::variant-based actions. Include explicitly when you need these — they are not
// pulled in by blusys/app/app.hpp to keep default builds lighter.

#include "blusys/app/integration_dispatch.hpp"
#include "blusys/app/variant_dispatch.hpp"
