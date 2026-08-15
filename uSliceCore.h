/**
 * @file uSliceCore.h
 * @brief Umbrella header for the C-callable scheduler and tick entry points.
 *
 * The task manager declaration is generated for the application's main file.
 * Include that generated manager header together with this umbrella header.
 * osTickISR() (time/osTickISR.h) is the library-wide C-callable entry point.
 *
 * This umbrella is for user code (main.c / main.cpp) only. Internal
 * library files never include it; time/osTickISR.cpp includes
 * time/osTickISR.h directly.
 *
 * @author uSliceCore Team
 */

#pragma once

#include "time/osTickISR.h"
