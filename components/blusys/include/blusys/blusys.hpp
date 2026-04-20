/**
 * @file blusys.hpp
 * @brief C++ umbrella header — HAL, drivers, services, and the framework layer.
 *
 * Include this from C++ code to get the full platform. For finer-grained
 * control, include specific headers directly.
 *
 * The threading contract that governs reducer / effect / capability / ISR
 * boundaries and queue budgets is documented in `docs/internals/threading.md`.
 * Every subsystem in this header is written against that contract.
 */

#pragma once

#include "blusys/blusys.h"
#include "blusys/framework/framework.hpp"
#include "blusys/observe/budget.hpp"
