#pragma once

// Only exists because test_cross_file_main.cpp needs typed access to the
// context defined in another translation unit. A task nobody configures
// externally can keep its context fully local to its own .cpp instead (see
// test_lifecycle_autostart.cpp).
struct CrossFileCtx {
    int targetSpeed = 0;
    int actualSpeed = 0;
    int loopSawTargetSpeed = -1; // proves Loop ran after main.cpp's write
};
