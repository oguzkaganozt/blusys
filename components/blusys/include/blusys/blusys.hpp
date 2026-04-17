/* blusys/blusys.hpp — C++ umbrella header (everything)
 *
 * Include this from C++ code to pull in HAL, drivers, services, and framework.
 * For finer-grained control, include specific headers directly.
 *
 * The threading contract that governs reducer / effect / capability / ISR
 * boundaries and queue budgets is documented in docs/internals/threading.md.
 * Every subsystem in this header is written against that contract.
 */

#pragma once

#include "blusys/blusys.h"
#include "blusys/framework/framework.hpp"
#include "blusys/framework/observe/budget.hpp"
