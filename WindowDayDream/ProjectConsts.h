#pragma once

#pragma warning (disable: 26812)

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
#define BASE_INTEGRAL 32762 //SHORT_MAX - 5 
#define DELEGATE_CHECK
#define ANGLE_TO_SCREEN 90.0 
#define STEP_ROUND 8 

// ============================================================================
// MOUSE CONTROLLER CONFIGURATION
// ============================================================================
// Sensitivity settings (0.1 - 10.0 recommended)
#define MOUSE_SENSITIVITY 0.7f              // Cursor movement sensitivity
#define MOUSE_SCROLL_SENSITIVITY 0.2f       // Scrolling sensitivity
#define MOUSE_GYRO_SENSITIVITY 0.7f         // Gyroscope sensitivity (X,Y axes)
#define MOUSE_DEADZONE 80                   // Dead zone for small movements (in pixels)
#define MOUSE_GYRO_DEADZONE 0.01f           // Dead zone pro gyroskop

// Mouse smoothing mode (0=NONE, 1=EMA, 2=INERTIA, 3=SPLINE)
#define MOUSE_SMOOTHING_MODE 1              // SMOOTHING_INERTIA

// ============================================================================
// GYRO FILTER CONFIGURATION
// ============================================================================
#define GYRO_FILTER_ALPHA 0.15f             // EMA smoothing (0.0-1.0, nižší=hladší)
#define GYRO_FILTER_THRESHOLD 0.02f         // Minimum movement detection (lower than original 0.8f!)
#define GYRO_VELOCITY_DAMPING 0.15f         // Velocity damping (0.8-0.99)

// Kalman filter parameters
#define GYRO_KALMAN_ESTIMATE_ERROR 0.5f
#define GYRO_KALMAN_MEASUREMENT_ERROR 0.05f
#define GYRO_KALMAN_PROCESS_VARIANCE 0.001f

// ============================================================================
// MOUSE MOTION FILTER CONFIGURATION
// ============================================================================
#define MOTION_SMOOTHING_ALPHA 0.20f        // EMA smoothing (0.1=velmi hladké, 0.9=responsivní)
#define MOTION_VELOCITY_DAMPING 0.75f       // Inertia slowdown (0.8-0.99, higher=more inertia)
#define MOTION_MIN_THRESHOLD 0.5f           // Minimum movement for registration
#define MOTION_HISTORY_SIZE 5               // Size of motion history
#define MOTION_STOP_THRESHOLD 3             // Frames without motion to reset

// ============================================================================
// DAYDREAM MOUSE MAPPER CONFIGURATION
// ============================================================================
#define MAPPER_TRACKPAD_SENSITIVITY 1.5f    // Trackpad sensitivity
#define MAPPER_SCROLL_SENSITIVITY 1.0f      // Scroll sensitivity
#define MAPPER_GYRO_SENSITIVITY 0.8f        // Gyro sensitivity (reduced from 2.0f for better control)
#define MAPPER_CLICK_THRESHOLD 100          // Pixels motion before click is cancelled
#define MAPPER_INVERT_GYRO_X false          // Invert X axis of gyroscope
#define MAPPER_INVERT_GYRO_Y false          // Invert Y axis of gyroscope
#define MAPPER_GYRO_MODE_ENABLED true       // Gyroscope controls mouse (true/false)

// Long button press timing for calibration
#define MAPPER_FRAMES_FOR_CALIBRATION 40    // ~2 seconds at 20 FPS (50ms per frame)

// ============================================================================
// GYRO CALIBRATOR CONFIGURATION
// ============================================================================
#define CALIB_GYRO_SAMPLES_TO_COLLECT 100   // Samples for drift calibration
#define CALIB_ORIENTATION_SAMPLES 150       // Samples for orientation calibration

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================
using bigNum = int;
using smallNum = short;
using decimal = float;
static constexpr decimal decimalMax = (std::numeric_limits<decimal>::max)();
static constexpr bigNum bigNumMax = (std::numeric_limits<bigNum>::max)();
static constexpr smallNum smallNumMax = (std::numeric_limits<smallNum>::max)();

unsigned long long cycleCount = 0;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
void outEventMessage(const char* name, const char* n) {
	const std::string CC = std::string(std::to_string(++cycleCount) + "_" + n + "_" + name);
	std::cout << "[" << CC << "]" << std::endl;
};

inline void DoEvents() {
	// Nothing needed for console version
};