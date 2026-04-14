#pragma once

// Framework-layer umbrella — for framework self-composition and internal use.
// Product code includes specific headers or blusys/blusys.hpp.

#include "blusys/framework/app/app.hpp"
#include "blusys/framework/capabilities/capabilities.hpp"
#include "blusys/framework/flows/flows.hpp"
#include "blusys/framework/engine/router.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/framework/feedback/feedback.hpp"
#include "blusys/framework/feedback/presets.hpp"
#include "blusys/framework/containers.hpp"
#include "blusys/framework/callbacks.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/composition/view.hpp"
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/framework/ui/primitives.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#endif
