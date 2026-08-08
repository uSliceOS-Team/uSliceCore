#pragma once

// Only exists because test_cross_file_main.cpp needs typed access via
// TASK_CONTEXT -- a task nobody configures externally can keep its
// context fully local to its own .cpp instead (see
// test_lifecycle_autostart.cpp).
struct CrossFileCtx {
    int targetSpeed = 0;
    int actualSpeed = 0;
    int entrySawTargetSpeed = -1; // proves Entry ran after main.cpp's write
};
