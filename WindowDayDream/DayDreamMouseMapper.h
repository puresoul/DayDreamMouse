#pragma once

#include "ProjectConsts.h"
#include "MouseController.h"
#include "GyroCalibrator.h"
#include "MCommand.h"
#include <iostream>

// ============================================================================
// DayDream Mouse Input Mapper
// Maps DayDream sensors to mouse and keyboard
// ============================================================================

class DayDreamMouseMapper
{
private:
	MouseController* m_mouseController = nullptr;
	GyroCalibrator m_gyroCalibrator;
	
	// Calibration states
	enum CalibrationState
	{
		CALIB_NONE,
		CALIB_GYRO_DRIFT,     // Gyroscope drift calibration
		CALIB_ORIENTATION,    // Orientation calibration (forward)
	} m_calibrationState = CALIB_NONE;

	// State tracking
	struct InputState
	{
		bool trackpadTouching = false;
		bool leftClickActive = false;
		bool rightClickActive = false;
		float lastTrackpadX = 0.0f;
		float lastTrackpadY = 0.0f;
		bool gyroMode = true;  // true = gyroscope controls mouse, false = trackpad
	} m_inputState;

	// Configuration (initialized from ProjectConsts)
	float m_trackpadSensitivity = MAPPER_TRACKPAD_SENSITIVITY;
	float m_scrollSensitivity = MAPPER_SCROLL_SENSITIVITY;
	float m_gyroSensitivity = MAPPER_GYRO_SENSITIVITY;
	int m_clickThreshold = MAPPER_CLICK_THRESHOLD;
	bool m_invertGyroX = MAPPER_INVERT_GYRO_X;
	bool m_invertGyroY = MAPPER_INVERT_GYRO_Y;

	// Long button press detection for calibration
	struct LongPressState
	{
		bool menuButtonPressed = false;
		int menuButtonPressFrames = 0;
		const int FRAMES_FOR_CALIBRATION = MAPPER_FRAMES_FOR_CALIBRATION;
		bool calibrationStarted = false;
	} m_longPressState;

public:
	DayDreamMouseMapper(MouseController* mouseController)
		: m_mouseController(mouseController)
	{
		if (!m_mouseController)
		{
			throw std::runtime_error("MouseController is null");
		}
		std::cout << "DayDream Mouse Mapper initialized (Gyro Mode: ON)" << std::endl;
	}

	~DayDreamMouseMapper() {}

	// Process DayDream trackpad for cursor movement
	// trackpadX, trackpadY: normalized values (0-1)
	void ProcessTrackpad(float trackpadX, float trackpadY, bool isTouching, bool isClicking)
	{
		if (!m_mouseController) return;

		// Convert from [0, 1] to [-1, 1]
		float normalizedX = (trackpadX * 2.0f) - 1.0f;
		float normalizedY = (trackpadY * 2.0f) - 1.0f;

		// Touch detection
		if (isTouching && !m_inputState.trackpadTouching)
		{
			m_inputState.trackpadTouching = true;
			m_inputState.lastTrackpadX = trackpadX;
			m_inputState.lastTrackpadY = trackpadY;
			std::cout << "Trackpad touched" << std::endl;
		}
		else if (!isTouching && m_inputState.trackpadTouching)
		{
			m_inputState.trackpadTouching = false;
			std::cout << "Trackpad released" << std::endl;
		}

		// Cursor movement only if not in gyro mode
		if (!m_inputState.gyroMode && m_inputState.trackpadTouching && !m_inputState.leftClickActive)
		{
			m_mouseController->MoveMouse(normalizedX, normalizedY);
		}

		// Trackpad click
		if (isClicking && !m_inputState.leftClickActive)
		{
			m_inputState.leftClickActive = true;
			m_mouseController->LeftClick();
		}
		else if (!isClicking && m_inputState.leftClickActive)
		{
			m_inputState.leftClickActive = false;
			m_mouseController->LeftClickRelease();
		}

		m_inputState.lastTrackpadX = trackpadX;
		m_inputState.lastTrackpadY = trackpadY;
	}

	// Process IMU - gyroscope for mouse movement
	void ProcessIMU(const TripleXYZ<float>& ori, const TripleXYZ<float>& acc, const TripleXYZ<float>& gyr)
	{
		if (!m_mouseController) return;

		// During gyro drift calibration, collect data
		if (m_calibrationState == CALIB_GYRO_DRIFT)
		{
			if (m_gyroCalibrator.AddSample(gyr))
			{
			// Gyro drift calibration complete, start orientation calibration
				m_calibrationState = CALIB_ORIENTATION;
				m_gyroCalibrator.StartOrientationCalibration();
				return;
			}
			return; 
		}

		// During orientation calibration, collect data
		if (m_calibrationState == CALIB_ORIENTATION)
		{
			if (m_gyroCalibrator.AddOrientationSample(ori))
			{
				// All calibrations complete
				m_calibrationState = CALIB_NONE;
				m_gyroCalibrator.PrintCalibrationStatus();
				
				
				// Calibration complete - let user set axes manually
				PrintAxisInversionHelp();
				return;
			}
			return;
		}

		TripleXYZ<float> calibratedOri = m_gyroCalibrator.GetCalibratedOrientation(ori);

		if (m_inputState.gyroMode)
		{

			float gyroX = m_invertGyroX ? -calibratedOri.x : calibratedOri.x;
			float gyroY = m_invertGyroY ? -calibratedOri.y : calibratedOri.y;
			
			m_mouseController->MoveMouseGyro(gyroX, gyroY, calibratedOri.z);
		}
	}

	// Zpracovani tlacítek DayDream
	void ProcessButtons(const DataButtons& buttons)
	{
		if (!m_mouseController) return;

		// bNearRun = Menu button - Long press to start calibration, short press for right click
		if (buttons.bNearRun)
		{
			if (!m_longPressState.menuButtonPressed)
			{
				// Button just pressed
				m_longPressState.menuButtonPressed = true;
				m_longPressState.menuButtonPressFrames = 0;
				m_longPressState.calibrationStarted = false;
			}
			else
			{
				// Button is being held
				m_longPressState.menuButtonPressFrames++;
				
				// Start calibration after holding for FRAMES_FOR_CALIBRATION frames
				if (m_longPressState.menuButtonPressFrames >= m_longPressState.FRAMES_FOR_CALIBRATION 
					&& !m_longPressState.calibrationStarted)
				{
					m_longPressState.calibrationStarted = true;
					std::cout << "\n>>> Menu button held for 2 seconds - Starting Gyro Calibration! <<<" << std::endl;
					StartGyroCalibration();
				}
			}
		}
		else
		{
			if (m_longPressState.menuButtonPressed)
			{
				// Button was released
				if (m_longPressState.menuButtonPressFrames < m_longPressState.FRAMES_FOR_CALIBRATION)
				{
					// Short press - perform right click
					m_mouseController->RightClick();
					m_mouseController->RightClickRelease();
					std::cout << "Menu button short press -> Right click" << std::endl;
				}
				m_longPressState.menuButtonPressed = false;
				m_longPressState.menuButtonPressFrames = 0;
			}
		}


		// bNearTouch = Back -> Double click
		static bool lastNearTouch = false;
		if (buttons.bNearTouch && !lastNearTouch)
		{
			m_mouseController->LeftDoubleClick();
			std::cout << "Back button pressed -> Double click" << std::endl;
		}
		lastNearTouch = buttons.bNearTouch;

		// bSoundPlus = Scroll Up
		static bool lastSoundPlus = false;
		if (buttons.bSoundPlus && !lastSoundPlus)
		{
			m_mouseController->ScrollUp(3);
			std::cout << "Sound+ button pressed -> Scroll up" << std::endl;
		}
		lastSoundPlus = buttons.bSoundPlus;

		// bSoundMinus = Scroll Down
		static bool lastSoundMinus = false;
		if (buttons.bSoundMinus && !lastSoundMinus)
		{
			m_mouseController->ScrollDown(3);
			std::cout << "Sound- button pressed -> Scroll down" << std::endl;
		}
		lastSoundMinus = buttons.bSoundMinus;
	}

	void StartGyroCalibration()
	{
		m_calibrationState = CALIB_GYRO_DRIFT;
		m_gyroCalibrator.StartCalibration();
	}

	void StartOrientationCalibrationOnly()
	{
		m_calibrationState = CALIB_ORIENTATION;
		m_gyroCalibrator.StartOrientationCalibration();
	}


	void SetTrackpadSensitivity(float sensitivity)
	{
		m_trackpadSensitivity = sensitivity;
		std::cout << "Trackpad sensitivity set to: " << sensitivity << std::endl;
	}


	void SetScrollSensitivity(float sensitivity)
	{
		m_scrollSensitivity = sensitivity;
		m_mouseController->SetScrollSensitivity(sensitivity);
	}


	void SetGyroSensitivity(float sensitivity)
	{
		m_gyroSensitivity = sensitivity;
		m_mouseController->SetGyroSensitivity(sensitivity);
		std::cout << "Gyro sensitivity set to: " << sensitivity << std::endl;
	}


	void SetGyroFilterAlpha(float alpha)
	{
		if (m_mouseController)
		{
			m_mouseController->SetGyroFilterAlpha(alpha);
		}
	}


	void SetGyroMode(bool enabled)
	{
		m_inputState.gyroMode = enabled;
		std::cout << "Gyro mode: " << (enabled ? "ON" : "OFF") << std::endl;
	}

	void Reset()
	{
		m_inputState.trackpadTouching = false;
		m_inputState.leftClickActive = false;
		m_inputState.rightClickActive = false;
		m_mouseController->Reset();
	}


	bool IsCalibrating() const
	{
		return m_calibrationState != CALIB_NONE;
	}


	void SetInvertGyroX(bool invert)
	{
		m_invertGyroX = invert;
		std::cout << "Gyro X axis invert: " << (invert ? "ON" : "OFF") << std::endl;
	}


	void SetInvertGyroY(bool invert)
	{
		m_invertGyroY = invert;
		std::cout << "Gyro Y axis invert: " << (invert ? "ON" : "OFF") << std::endl;
	}


	bool IsGyroXInverted() const { return m_invertGyroX; }
	bool IsGyroYInverted() const { return m_invertGyroY; }


	static void PrintAxisInversionHelp()
	{
		std::cout << "\n========== AXIS INVERSION SETUP ==========" << std::endl;
		std::cout << "If the cursor moves in the opposite direction:" << std::endl;
		std::cout << "  - Upside down (opposite vertical):  SetInvertGyroX(true)" << std::endl;
		std::cout << "  - Backwards (opposite horizontal): SetInvertGyroY(true)" << std::endl;
		std::cout << "Example in code:" << std::endl;
		std::cout << "  g_mouseMapper->SetInvertGyroX(true);  // For inverted vertical motion" << std::endl;
		std::cout << "  g_mouseMapper->SetInvertGyroY(true);  // For inverted horizontal motion" << std::endl;
		std::cout << "=========================================\n" << std::endl;
	}

	static void PrintHelp()
	{
		std::cout << "\n========== DayDream Mouse Controls (Gyro Mode) ==========" << std::endl;
		std::cout << "Gyroskop:     - Move cursor based on gyro orientation" << std::endl;
		std::cout << "Trackpad:     - Left click (touch surface)" << std::endl;
		std::cout << "Menu button:  - Short press = Right click | Long press (2s) = Start calibration" << std::endl;
		std::cout << "Back button:  - Double click" << std::endl;
		std::cout << "Sound+ btn:   - Scroll up" << std::endl;
		std::cout << "Sound- btn:   - Scroll down" << std::endl;
		std::cout << "=== CALIBRATION ===" << std::endl;
		std::cout << "To start gyro calibration: HOLD MENU button for 2 seconds!" << std::endl;
		std::cout << "Calibration has 2 stages:" << std::endl;
		std::cout << "1. Gyro Drift - Keep controller still for ~5 seconds" << std::endl;
		std::cout << "2. Orientation - Point controller forward (screen facing you)" << std::endl;
		std::cout << "=========================================================\n" << std::endl;
	}
};

