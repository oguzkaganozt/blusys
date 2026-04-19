/* src/framework/test/test_stubs.cpp — weak stubs for capability-shaped
 * symbols that test binaries may reference without linking the real
 * *_host.cpp capability translation units.
 *
 * Tests link their own mocks (see mocks/mock_capability.hpp); they do NOT
 * pull in the real capability implementations. That leaves a handful of
 * symbols (`connectivity_capability::request_reconnect`, …) that some
 * framework helpers still reference unconditionally.
 *
 * This file provides inert implementations: they satisfy the linker and
 * return a failure code if ever called. As the typed-effect cleanup grows,
 * this file shrinks and can eventually disappear.
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
