/**
 * @file Registrations.hpp
 * @brief The single header Logic includes to reach every task by name.
 *
 * DECLARE_TASK never needs a task's context type -- Task itself is
 * context-agnostic. MotorCtx.hpp is included here only because Logic
 * happens to configure that one task's parameters (see Logic.cpp); a
 * project where Logic only starts/stops tasks blindly wouldn't need any
 * context headers here at all.
 */

#pragma once

#include "tasks/osTaskMgmtMacros.hpp"
#include "MotorCtx.hpp"

DECLARE_TASK(ledBlinker);
DECLARE_TASK(motor);
DECLARE_TASK(sensorMonitor);
