/**
 * @file TaskDefinitions.hpp
 * @brief Context types inferred from task names by generated code.
 */

#pragma once

struct LedBlinkerContext {
    int blinksRemaining = 5;
    bool initialized = false;
};

struct MotorContext {
    int targetSpeed = 0;
    int actualSpeed = 0;
};

struct SensorMonitorContext {
    int reading = 0;
};

struct LogicContext {};
