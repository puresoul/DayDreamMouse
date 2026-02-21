#pragma once

#include "ProjectConsts.h"
#include <cmath>
#include <deque>

class MouseMotionFilter
{
private:
	// Smoothing state
	struct MotionState
	{
		float currentX = 0.0f;
		float currentY = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
	} m_state;

	// Filter configuration (initialized from ProjectConsts)
	float m_smoothingAlpha = MOTION_SMOOTHING_ALPHA;
	float m_velocityDamping = MOTION_VELOCITY_DAMPING;
	float m_minMotion = MOTION_MIN_THRESHOLD;
	
	// Motion history for trend detection
	struct MotionHistory
	{
		float deltaX = 0.0f;
		float deltaY = 0.0f;
	};
	std::deque<MotionHistory> m_history;
	const int HISTORY_SIZE = MOTION_HISTORY_SIZE;

	// Stop detection
	struct StopDetection
	{
		int framesWithoutMotion = 0;
		const int STOP_THRESHOLD = MOTION_STOP_THRESHOLD;
	} m_stopDetection;

public:
	MouseMotionFilter() {}

	~MouseMotionFilter() {}

	void FilterMotion(float& deltaX, float& deltaY)
	{
		// Apply EMA filter
		m_state.currentX = (m_smoothingAlpha * deltaX) + ((1.0f - m_smoothingAlpha) * m_state.currentX);
		m_state.currentY = (m_smoothingAlpha * deltaY) + ((1.0f - m_smoothingAlpha) * m_state.currentY);


		if (std::fabs(m_state.currentX) < m_minMotion)
			m_state.currentX = 0.0f;
		if (std::fabs(m_state.currentY) < m_minMotion)
			m_state.currentY = 0.0f;

		// Assign smoothed values
		deltaX = m_state.currentX;
		deltaY = m_state.currentY;

		// Track motion history
		AddToHistory(deltaX, deltaY);


		DetectStop(deltaX, deltaY);
	}

	// Apply smoothing with inertia (momentum)
	// Better for gyroscope movement
	void FilterMotionWithInertia(float& deltaX, float& deltaY)
	{
		// Calculate difference from last update
		float accelX = deltaX - m_state.velocityX;
		float accelY = deltaY - m_state.velocityY;

		// Apply smoothing to acceleration
		accelX *= m_smoothingAlpha;
		accelY *= m_smoothingAlpha;

		// Update velocity
		m_state.velocityX += accelX;
		m_state.velocityY += accelY;

		// Apply friction (damping)
		m_state.velocityX *= m_velocityDamping;
		m_state.velocityY *= m_velocityDamping;

		// Stop at very small values
		if (std::fabs(m_state.velocityX) < m_minMotion)
			m_state.velocityX = 0.0f;
		if (std::fabs(m_state.velocityY) < m_minMotion)
			m_state.velocityY = 0.0f;


		deltaX = m_state.velocityX;
		deltaY = m_state.velocityY;

		AddToHistory(deltaX, deltaY);
	}

	// Apply Catmull-Rom spline interpolation for ultra-smooth motion
	void FilterMotionSpline(float& deltaX, float& deltaY)
	{
		// Add new point to history
		MotionHistory current = { deltaX, deltaY };
		m_history.push_back(current);
		if (m_history.size() > HISTORY_SIZE)
			m_history.pop_front();

		// If we don't have enough points, use EMA
		if (m_history.size() < 3)
		{
			FilterMotion(deltaX, deltaY);
			return;
		}

		// Apply Catmull-Rom interpolation to the last points
		// Weighted average of the last points
		float smoothedX = 0.0f;
		float smoothedY = 0.0f;
		float weightSum = 0.0f;

		const float weights[] = { 0.1f, 0.2f, 0.4f, 0.3f, 0.2f };
		int historySize = static_cast<int>(m_history.size());

		for (int i = 0; i < historySize && i < 5; ++i)
		{
			float weight = weights[5 - historySize + i];
			smoothedX += m_history[i].deltaX * weight;
			smoothedY += m_history[i].deltaY * weight;
			weightSum += weight;
		}

		if (weightSum > 0.0f)
		{
			smoothedX /= weightSum;
			smoothedY /= weightSum;
		}

		deltaX = smoothedX;
		deltaY = smoothedY;
	}


	void SetSmoothingAlpha(float alpha)
	{
		m_smoothingAlpha = alpha > 1.0f ? 1.0f : (alpha < 0.0f ? 0.0f : alpha);
	}

	void SetVelocityDamping(float damping)
	{
		m_velocityDamping = damping > 1.0f ? 1.0f : (damping < 0.0f ? 0.0f : damping);
	}


	void SetMinMotion(float minMotion)
	{
		m_minMotion = minMotion;
	}

	void Reset()
	{
		m_state.currentX = 0.0f;
		m_state.currentY = 0.0f;
		m_state.velocityX = 0.0f;
		m_state.velocityY = 0.0f;
		m_history.clear();
		m_stopDetection.framesWithoutMotion = 0;
	}


	void GetVelocity(float& velX, float& velY) const
	{
		velX = m_state.velocityX;
		velY = m_state.velocityY;
	}

private:

	void AddToHistory(float deltaX, float deltaY)
	{
		m_history.push_back({ deltaX, deltaY });
		if (m_history.size() > HISTORY_SIZE)
			m_history.pop_front();
	}


	void DetectStop(float deltaX, float deltaY)
	{
		float magnitude = std::sqrt(deltaX * deltaX + deltaY * deltaY);
		
		if (magnitude < m_minMotion)
		{
			m_stopDetection.framesWithoutMotion++;
		}
		else
		{
			m_stopDetection.framesWithoutMotion = 0;
		}


		if (m_stopDetection.framesWithoutMotion >= m_stopDetection.STOP_THRESHOLD)
		{
			m_state.velocityX = 0.0f;
			m_state.velocityY = 0.0f;
		}
	}
};
