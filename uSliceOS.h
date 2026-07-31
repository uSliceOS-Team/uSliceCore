/**
 * @file uSliceOS.h
 * @brief Umbrella header: both C-callable entry points in one include.
 *
 * osTaskManager() (tasks/osTaskManager.h) and osTickISR()
 * (time/osTickISR.h) are the only two calls exposed with C linkage;
 * everything else (TASK, CASES, ADD_TASK, ...) is C++-only and lives
 * under tasks/ and time/.
 *
 * This umbrella is for user code (main.c / main.cpp) only. Internal
 * library files never include it: tasks/osTaskManager.cpp includes
 * tasks/osTaskManager.h directly, time/osTickISR.cpp includes
 * time/osTickISR.h directly, each file pulling in only the one
 * declaration it actually implements.
 *
 * @author uSliceOS Team
 */

#pragma once

#include "tasks/osTaskManager.h"
#include "time/osTickISR.h"
