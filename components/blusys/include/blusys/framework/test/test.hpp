/* blusys/framework/test/test.hpp — umbrella for the headless test surface.
 *
 * Include this from a blusys test source to pull in the headless harness,
 * fake clock, fake HAL inject API, and canonical capability mocks. Nothing
 * in product code ever includes this.
 */

#ifndef BLUSYS_FRAMEWORK_TEST_TEST_HPP
#define BLUSYS_FRAMEWORK_TEST_TEST_HPP

#include "blusys/framework/test/fake_clock.hpp"
#include "blusys/framework/test/fake_hal.h"
#include "blusys/framework/test/harness.hpp"
#include "blusys/framework/test/mocks/mock_capability.hpp"

#endif  // BLUSYS_FRAMEWORK_TEST_TEST_HPP
