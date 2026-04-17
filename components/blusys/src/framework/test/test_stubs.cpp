/* src/framework/test/test_stubs.cpp — weak stubs for capability-shaped
 * symbols that app_ctx and app_runtime reference through the current
 * service-locator path.
 *
 * Tests link their own mocks (see mocks/mock_capability.hpp); they do NOT
 * pull in the real *_host.cpp capability translation units. That leaves a
 * handful of symbols (`connectivity_capability::request_reconnect`, …)
 * referenced by ctx.cpp through `get<T>()` calls whose type-specific
 * instantiations are emitted unconditionally.
 *
 * This file provides inert implementations: they satisfy the linker and
 * return a failure code if ever called. P3's typed effects remove this
 * coupling entirely, at which point this file gets deleted along with the
 * service locator.
 */

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/hal/error.h"

namespace blusys {

blusys_err_t connectivity_capability::request_reconnect()
{
    /* Test binaries do not exercise the reconnect path; real capability
     * sources provide the real implementation when linked. */
    return BLUSYS_ERR_NOT_SUPPORTED;
}

}  // namespace blusys
