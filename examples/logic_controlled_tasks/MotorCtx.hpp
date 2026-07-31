/**
 * @file MotorCtx.hpp
 * @brief Context for the "motor" task.
 *
 * Lives in its own header, unlike LedBlinkerCtx, because Logic needs a
 * typed view of it (via TASK_CONTEXT) to set targetSpeed before starting
 * the task. Any task whose context only its own .cpp ever touches can
 * keep that context fully local instead -- this split is opt-in, per
 * task, not a rule you apply everywhere.
 */

#pragma once

struct MotorCtx {
    int targetSpeed = 0;   // written by Logic before START_TASK
    int actualSpeed = 0;   // written by the task itself, read back by Logic
};
