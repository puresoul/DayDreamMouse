#pragma once

#include <windows.h>
#include <iostream>
#include <cmath>
#include "ProjectConsts.h"
#include "GyroFilter.h"
#include "MouseMotionFilter.h"

// ============================================================================
// Mouse Control Manager - Mouse cursor control using DayDream controller
// ============================================================================

class MouseController
{
private:
	// Mouse state
	struct MouseState
	{
		int lastX = 0;
		int lastY = 0;
		bool leftClickPressed = false;
		bool rightClickPressed = false;
		bool scrolling = false;
	} m_state;

	// Sensitivity settings (initialized from ProjectConsts)
	float m_sensitivity = MOUSE_SENSITIVITY;
	float m_scrollSensitivity = MOUSE_SCROLL_SENSITIVITY;
	float m_gyroSensitivity = MOUSE_GYRO_SENSITIVITY;
	int m_deadzone = MOUSE_DEADZONE;
	float m_gyroDeadzone = MOUSE_GYRO_DEADZONE;

	// Screen dimensions
	int m_screenWidth = 0;
	int m_screenHeight = 0;
	int m_screenOffsetX = 0;  // Virtual screen offset (minimum X)
	int m_screenOffsetY = 0;  // Virtual screen offset (minimum Y)

	// Click state
	bool m_leftButtonDown = false;
	bool m_rightButtonDown = false;

	// Filter for gyroscope smoothing
	GyroFilter m_gyroFilter;
	
	// Filter for cursor motion smoothing
	MouseMotionFilter m_motionFilter;
	
	// Cursor motion smoothing mode
	enum SmoothingMode
	{
		SMOOTHING_NONE,       // No smoothing
		SMOOTHING_EMA,        // Exponential Moving Average
		SMOOTHING_INERTIA,    // With inertia
		SMOOTHING_SPLINE      // Spline interpolation
	} m_smoothingMode = static_cast<SmoothingMode>(MOUSE_SMOOTHING_MODE);

public:
	MouseController()
	{
		// Load display resolution - entire virtual screen for multiple monitors
		m_screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		m_screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		m_screenOffsetX = GetSystemMetrics(SM_XVIRTUALSCREEN);  // Minimum X coordinate
		m_screenOffsetY = GetSystemMetrics(SM_YVIRTUALSCREEN);  // Minimum Y coordinate
		
		// If virtual screen is empty (which shouldn't happen), use primary monitor
		if (m_screenWidth <= 0) m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
		if (m_screenHeight <= 0) m_screenHeight = GetSystemMetrics(SM_CYSCREEN);
		
		// Initial cursor position
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		m_state.lastX = cursorPos.x;
		m_state.lastY = cursorPos.y;

		// Set filter from configuration
		m_gyroFilter.SetAlpha(GYRO_FILTER_ALPHA);
		m_gyroFilter.SetThreshold(GYRO_FILTER_THRESHOLD);
		
		// Set motion filter
		m_motionFilter.SetSmoothingAlpha(MOTION_SMOOTHING_ALPHA);
		m_motionFilter.SetVelocityDamping(MOTION_VELOCITY_DAMPING);
		m_motionFilter.SetMinMotion(MOTION_MIN_THRESHOLD);

		std::cout << "Mouse Controller initialized" << std::endl;
		std::cout << "Virtual screen resolution: " << m_screenWidth << " x " << m_screenHeight << std::endl;
		std::cout << "Virtual screen offset: (" << m_screenOffsetX << ", " << m_screenOffsetY << ")" << std::endl;
	}

	~MouseController() {}

	// Set cursor sensitivity (0.1 - 10.0)
	void SetSensitivity(float sensitivity)
	{
		m_sensitivity = sensitivity;
		std::cout << "Mouse sensitivity set to: " << sensitivity << std::endl;
	}

	// Set gyroscope sensitivity
	void SetGyroSensitivity(float sensitivity)
	{
		m_gyroSensitivity = sensitivity;
		std::cout << "Gyro sensitivity set to: " << sensitivity << std::endl;
	}

	// Set scroll sensitivity
	void SetScrollSensitivity(float sensitivity)
	{
		m_scrollSensitivity = sensitivity;
	}

	// Set dead zone for small movements (in pixels)
	void SetDeadzone(int deadzone)
	{
		m_deadzone = deadzone;
	}

	// Set dead zone for gyroscope
	void SetGyroDeadzone(float deadzone)
	{
		m_gyroDeadzone = deadzone;
	}

	// Set gyroscope filter (0.1 = very smooth, 0.9 = responsive)
	void SetGyroFilterAlpha(float alpha)
	{
		m_gyroFilter.SetAlpha(alpha);
		std::cout << "Gyro filter alpha set to: " << alpha << std::endl;
	}

	// Set cursor motion smoothing
	void SetMotionSmoothingAlpha(float alpha)
	{
		m_motionFilter.SetSmoothingAlpha(alpha);
		std::cout << "Motion smoothing alpha set to: " << alpha << std::endl;
	}

	// Set inertia damping (0.8-0.99, higher = more inertia)
	void SetMotionDamping(float damping)
	{
		m_motionFilter.SetVelocityDamping(damping);
		std::cout << "Motion damping set to: " << damping << std::endl;
	}

	// Set minimum motion for registration
	void SetMotionMinThreshold(float minMotion)
	{
		m_motionFilter.SetMinMotion(minMotion);
	}

	// Set smoothing mode
	void SetSmoothingMode(int mode)
	{
		if (mode >= 0 && mode <= 3)
		{
			m_smoothingMode = static_cast<SmoothingMode>(mode);
			const char* modeNames[] = { "NONE", "EMA", "INERTIA", "SPLINE" };
			std::cout << "Motion smoothing mode: " << modeNames[mode] << std::endl;
		}
	}

	// Move cursor based on trackpad
	// trackpadX, trackpadY: normalized values (-1.0 to 1.0)
	void MoveMouse(float trackpadX, float trackpadY)
	{
		
		// Calculate movement in pixels
		int deltaX = static_cast<int>(trackpadX * m_screenWidth * m_sensitivity * 0.01f);
		int deltaY = static_cast<int>(trackpadY * m_screenHeight * m_sensitivity * 0.02f);

		// Apply dead zone on distance
		if (abs(deltaX) < m_deadzone) deltaX = 0;
		if (abs(deltaY) < m_deadzone) deltaY = 0;

		// Apply motion smoothing
		float smoothDeltaX = static_cast<float>(deltaX);
		float smoothDeltaY = static_cast<float>(deltaY);

		if (m_smoothingMode != SMOOTHING_NONE)
		{
			m_motionFilter.FilterMotion(smoothDeltaX, smoothDeltaY);
			deltaX = static_cast<int>(smoothDeltaX);
			deltaY = static_cast<int>(smoothDeltaY);
		}

		// Only if motion exists
		if (deltaX != 0 || deltaY != 0)
		{
			// Get current cursor position
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			// Calculate new position
			int newX = cursorPos.x + deltaX;
			int newY = cursorPos.y + deltaY;

		// Limit to screen boundaries (including offset for multiple monitors)
			int maxX = m_screenOffsetX + m_screenWidth - 2;
			int maxY = m_screenOffsetY + m_screenHeight - 2;
			newX = max(m_screenOffsetX, min(newX, maxX));
			newY = max(m_screenOffsetY, min(newY, maxY));

			// Set new cursor position
			SetCursorPos(newX, newY);
		}
	}

	// Move cursor based on gyroscope - ABSOLUTE POSITION WITH SMOOTHING
	// gyroX: rotation around X axis (pitch) - vertical position (-1 to 1)
	// gyroY: rotation around Y axis (roll) - horizontal position (-1 to 1)
	// gyroZ: rotation around Z axis (yaw) - rotation
	void MoveMouseGyro(float gyroX, float gyroY, float gyroZ)
	{
		// Apply smoothing filter
		TripleXYZ<float> filtered = m_gyroFilter.ApplyEMA({ gyroX, gyroY, gyroZ });

		// Apply dead zone for gyroscope
		if (abs(filtered.x) < m_gyroDeadzone) filtered.x = 0.0f;
		if (abs(filtered.y) < m_gyroDeadzone) filtered.y = 0.0f;

		// Map filtered values to screen with sensitivity
		// filtered.x (pitch) -> vertical movement (Y screen axis)
		// filtered.y (roll) -> horizontal movement (X screen axis)
		
		// Apply sensitivity multiplier for smooth movement
		// Negation corrects mirrored axes
		float scaledX = -filtered.y * m_gyroSensitivity;
		float scaledY = -filtered.x * m_gyroSensitivity;

		// Limit maximum range (prevent jump)
		scaledX = (scaledX > 1.0f) ? 1.0f : (scaledX < -1.0f) ? -1.0f : scaledX;
		scaledY = (scaledY > 1.0f) ? 1.0f : (scaledY < -1.0f) ? -1.0f : scaledY;

		// Normalize to [0, screenWidth] and [0, screenHeight] with offset
		// Convert from [-1, 1] to [screenOffsetX, screenOffsetX + screenWidth-1]
		int newX = static_cast<int>(m_screenOffsetX + (scaledX + 1.0f) * 0.5f * (m_screenWidth - 1));
		int newY = static_cast<int>(m_screenOffsetY + (scaledY + 1.0f) * 0.7f * (m_screenHeight - 1));

		// Get current cursor position for delta calculation
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		float deltaX = static_cast<float>(newX - cursorPos.x);
		float deltaY = static_cast<float>(newY - cursorPos.y);

		// Apply cursor motion smoothing
		if (m_smoothingMode == SMOOTHING_INERTIA)
		{
			m_motionFilter.FilterMotionWithInertia(deltaX, deltaY);
		}
		else if (m_smoothingMode == SMOOTHING_SPLINE)
		{
			m_motionFilter.FilterMotionSpline(deltaX, deltaY);
		}
		else if (m_smoothingMode == SMOOTHING_EMA)
		{
			m_motionFilter.FilterMotion(deltaX, deltaY);
		}

		// Calculate final position
		newX = cursorPos.x + static_cast<int>(deltaX);
		newY = cursorPos.y + static_cast<int>(deltaY);

		// Limit to screen boundaries (including offset for multiple monitors)
		int maxX = m_screenOffsetX + m_screenWidth - 1;
		int maxY = m_screenOffsetY + m_screenHeight - 1;
		newX = max(m_screenOffsetX, min(newX, maxX));
		newY = max(m_screenOffsetY, min(newY, maxY));

		// Set absolute cursor position
		SetCursorPos(newX, newY);
	}

	// Left mouse button click
	void LeftClick()
	{
		if (!m_leftButtonDown)
		{
			// Button press
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			SendInput(1, &input, sizeof(INPUT));

			m_leftButtonDown = true;
		}
	}

	// Uvolnení levého tlaèítka myši
	void LeftClickRelease()
	{
		if (m_leftButtonDown)
		{
			// Button release
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			SendInput(1, &input, sizeof(INPUT));

			m_leftButtonDown = false;
		}
	}

	// Right mouse button click
	void RightClick()
	{
		if (!m_rightButtonDown)
		{
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			SendInput(1, &input, sizeof(INPUT));

			m_rightButtonDown = true;
		}
	}

	// Right mouse button release
	void RightClickRelease()
	{
		if (m_rightButtonDown)
		{
			INPUT input = {};
			input.type = INPUT_MOUSE;
			input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			SendInput(1, &input, sizeof(INPUT));

			m_rightButtonDown = false;
		}
	}

	// Double-click left button
	void LeftDoubleClick()
	{
		INPUT inputs[4] = {};

		// First click
		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		// Second click
		inputs[2].type = INPUT_MOUSE;
		inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		inputs[3].type = INPUT_MOUSE;
		inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		SendInput(4, inputs, sizeof(INPUT));
	}

	// Mouse wheel scrolling (positive = up, negative = down)
	void Scroll(int delta)
	{
		if (delta == 0) return;

		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		input.mi.mouseData = delta * static_cast<int>(m_scrollSensitivity) * 10; // 120 = standard WHEEL_DELTA

		SendInput(1, &input, sizeof(INPUT));
	}

	// Scroll up
	void ScrollUp(int lines = 3)
	{
		Scroll(lines);
	}

	// Scroll down
	void ScrollDown(int lines = 3)
	{
		Scroll(-lines);
	}

	// Horizontal scroll (positive = right, negative = left)
	void HorizontalScroll(int delta)
	{
		if (delta == 0) return;

		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
		input.mi.mouseData = delta * static_cast<int>(m_scrollSensitivity) * 120;

		SendInput(1, &input, sizeof(INPUT));
	}

	// Return current cursor position
	void GetCursorPosition(int& x, int& y) const
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		x = cursorPos.x;
		y = cursorPos.y;
	}

	// Reset state
	void Reset()
	{
		LeftClickRelease();
		RightClickRelease();
		m_state.scrolling = false;
		m_motionFilter.Reset();
	}
};

// ============================================================================
// Global MouseController instance
// ============================================================================

extern MouseController* g_mouseController;

