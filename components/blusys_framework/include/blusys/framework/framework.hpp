#pragma once

#include "blusys/framework/core/controller.hpp"
#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/core/runtime.hpp"

namespace blusys::framework {

// Optional umbrella hook for early startup. Currently a no-op; kept as a stable
// call site for examples and future global framework setup. Safe to call once
// before constructing runtime/controllers. Product apps using blusys::app entry
// macros do not need this.
void init();

// Optional hook after tearing down a minimal runtime demo. Currently a no-op;
// reserved for symmetry with init() in validation examples.
void run_once();

}  // namespace blusys::framework
