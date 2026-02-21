#pragma once

#include "ProjectConsts.h"
#include "MCommand.h"
#include <deque>
#include <numeric>

class GyroFilter
{
private:
	struct FilterState
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	} m_filtered;

	// Filter configuration (initialized from ProjectConsts)
	float m_alpha = GYRO_FILTER_ALPHA;
	float m_threshold = GYRO_FILTER_THRESHOLD;
	float m_velocityDamping = GYRO_VELOCITY_DAMPING;

	// For Kalman filter
	struct KalmanFilter
	{
		float value = 0.0f;
		float estimate_error = GYRO_KALMAN_ESTIMATE_ERROR;
		float measurement_error = GYRO_KALMAN_MEASUREMENT_ERROR;
		float process_variance = GYRO_KALMAN_PROCESS_VARIANCE;
	} m_kalmanX, m_kalmanY, m_kalmanZ;

	// Velocity tracking for predictive filter
	struct VelocityTracker
	{
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float velocityZ = 0.0f;
	} m_velocity;

public:
	GyroFilter() {}

	~GyroFilter() {}

	TripleXYZ<float> ApplyEMA(const TripleXYZ<float>& raw)
	{
		m_filtered.x = (m_alpha * raw.x) + ((1.0f - m_alpha) * m_filtered.x);
		m_filtered.y = (m_alpha * raw.y) + ((1.0f - m_alpha) * m_filtered.y);
		m_filtered.z = (m_alpha * raw.z) + ((1.0f - m_alpha) * m_filtered.z);

		// Apply threshold
		if (abs(m_filtered.x) < m_threshold) m_filtered.x = 0.0f;
		if (abs(m_filtered.y) < m_threshold) m_filtered.y = 0.0f;
		if (abs(m_filtered.z) < m_threshold) m_filtered.z = 0.0f;

		return { m_filtered.x, m_filtered.y, m_filtered.z };
	}

	// Apply Kalman filter (more precise, but more complex)
	TripleXYZ<float> ApplyKalman(const TripleXYZ<float>& raw)
	{
		m_kalmanX.value = UpdateKalman(m_kalmanX, raw.x);
		m_kalmanY.value = UpdateKalman(m_kalmanY, raw.y);
		m_kalmanZ.value = UpdateKalman(m_kalmanZ, raw.z);

		return { m_kalmanX.value, m_kalmanY.value, m_kalmanZ.value };
	}

	// Set alpha for EMA (lower = smoother, higher = more responsive)
	void SetAlpha(float alpha)
	{
		m_alpha = alpha > 1.0f ? 1.0f : (alpha < 0.0f ? 0.0f : alpha);
	}

	// Set threshold (minimum movement for registration)
	void SetThreshold(float threshold)
	{
		m_threshold = threshold;
	}

private:
	// Update Kalman filter with new measurement
	float UpdateKalman(KalmanFilter& kf, float measurement)
	{
		// Prediction step
		float estimate = kf.value;
		float estimate_error = kf.estimate_error + kf.process_variance;

		// Update step
		float gain = estimate_error / (estimate_error + kf.measurement_error);
		float value = estimate + gain * (measurement - estimate);
		float error = (1 - gain) * estimate_error;

		kf.value = value;
		kf.estimate_error = error;

		return value;
	}
};
