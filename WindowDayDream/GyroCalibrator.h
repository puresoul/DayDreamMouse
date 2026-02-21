#pragma once

#include "ProjectConsts.h"
#include "MCommand.h"
#include <vector>
#include <numeric>
#include <iostream>
#include <cmath>

class GyroCalibrator
{
private:
	struct Calibration
	{
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		float offsetZ = 0.0f;
		bool isCalibrated = false;
	} m_calibration;

	// Kalibrace pro orientaci (neutralnÌ poloha)
	struct OrientationCalibration
	{
		float neutralX = 0.0f;  // NeutralnÌ pitch (0 stupnu vp¯ed)
		float neutralY = 0.0f;  // NeutralnÌ roll (0 stupnu vlevo/vpravo)
		float neutralZ = 0.0f;  // NeutralnÌ yaw
		bool isCalibratedOrientation = false;
	} m_orientationCalibration;

	std::vector<TripleXYZ<float>> m_gyroSamples;
	std::vector<TripleXYZ<float>> m_orientationSamples;
	const int SAMPLES_TO_COLLECT = CALIB_GYRO_SAMPLES_TO_COLLECT;
	const int ORIENTATION_SAMPLES = CALIB_ORIENTATION_SAMPLES;

public:
	GyroCalibrator() {}

	~GyroCalibrator() {}

	// Start calibration - collect data from stationary gyroscope (offset drift)
	void StartCalibration()
	{
		m_gyroSamples.clear();
		std::cout << "\n========== GYRO DRIFT CALIBRATION ==========" << std::endl;
		std::cout << "Measuring gyro offset noise..." << std::endl;
		std::cout << "Keep the controller COMPLETELY STILL!" << std::endl;
	}

	// Add sample during calibration
	bool AddSample(const TripleXYZ<float>& gyroData)
	{
		if (m_gyroSamples.size() < SAMPLES_TO_COLLECT)
		{
			m_gyroSamples.push_back(gyroData);
			
			// Tisk prub
			int percent = (m_gyroSamples.size() * 100) / SAMPLES_TO_COLLECT;
			std::cout << "\rDrift Calibration: " << percent << "% (" << m_gyroSamples.size() << "/" << SAMPLES_TO_COLLECT << ")";
			std::cout.flush();

			return false;  // Kalibrace nenÌ hotova
		}
		else
		{
			// Kalibrace je hotova
			CompleteCalibration();
			return true;
		}
	}

	// SpustÌ kalibraci orientace - uûivatel smÏ¯uje ovladaË p¯Ìmo veda (forward)
	void StartOrientationCalibration()
	{
		m_orientationSamples.clear();
		std::cout << "\n========== ORIENTATION CALIBRATION ==========" << std::endl;
		std::cout << "Hold controller FORWARD with screen facing you" << std::endl;
		std::cout << "Collecting orientation baseline..." << std::endl;
	}

	// Add orientation sample during calibration
	bool AddOrientationSample(const TripleXYZ<float>& orientation)
	{
		if (m_orientationSamples.size() < ORIENTATION_SAMPLES)
		{
			m_orientationSamples.push_back(orientation);
			
			// Tisk prub
			int percent = (m_orientationSamples.size() * 100) / ORIENTATION_SAMPLES;
			std::cout << "\rOrientation Calibration: " << percent << "% (" << m_orientationSamples.size() << "/" << ORIENTATION_SAMPLES << ")";
			std::cout.flush();

			return false;  // Kalibrace nenÌ hotova
		}
		else
		{
			// Kalibrace je hotova
			CompleteOrientationCalibration();
			return true;
		}
	}

	// Dokonci kalibraci a vypocita offsety
	void CompleteCalibration()
	{
		if (m_gyroSamples.size() == 0) return;

		// Vypocet prumeru
		float sumX = 0, sumY = 0, sumZ = 0;
		for (const auto& sample : m_gyroSamples)
		{
			sumX += sample.x;
			sumY += sample.y;
			sumZ += sample.z;
		}

		m_calibration.offsetX = sumX / m_gyroSamples.size();
		m_calibration.offsetY = sumY / m_gyroSamples.size();
		m_calibration.offsetZ = sumZ / m_gyroSamples.size();
		m_calibration.isCalibrated = true;

		std::cout << "\n\nGYRO CALIBRATION COMPLETE!" << std::endl;
		std::cout << "Gyro Offset X: " << m_calibration.offsetX << std::endl;
		std::cout << "Gyro Offset Y: " << m_calibration.offsetY << std::endl;
		std::cout << "Gyro Offset Z: " << m_calibration.offsetZ << std::endl;
		std::cout << "=========================================\n" << std::endl;
	}

	// Dokonci orientacni kalibraci
	void CompleteOrientationCalibration()
	{
		if (m_orientationSamples.size() == 0) return;

		// Vypocet prumeru orientace
		float sumX = 0, sumY = 0, sumZ = 0;
		for (const auto& sample : m_orientationSamples)
		{
			sumX += sample.x;
			sumY += sample.y;
			sumZ += sample.z;
		}

		m_orientationCalibration.neutralX = sumX / m_orientationSamples.size();
		m_orientationCalibration.neutralY = sumY / m_orientationSamples.size();
		m_orientationCalibration.neutralZ = sumZ / m_orientationSamples.size();
		m_orientationCalibration.isCalibratedOrientation = true;

		std::cout << "\n\nORIENTATION CALIBRATION COMPLETE!" << std::endl;
		std::cout << "Neutral X (Pitch): " << m_orientationCalibration.neutralX << " degrees" << std::endl;
		std::cout << "Neutral Y (Roll): " << m_orientationCalibration.neutralY << " degrees" << std::endl;
		std::cout << "Neutral Z (Yaw): " << m_orientationCalibration.neutralZ << " degrees" << std::endl;
		std::cout << "========================================\n" << std::endl;
	}

	// Vraù kalibrovane gyro data
	TripleXYZ<float> GetCalibratedGyro(const TripleXYZ<float>& rawGyro) const
	{
		if (!m_calibration.isCalibrated)
		{
			return rawGyro;
		}

		return {
			rawGyro.x - m_calibration.offsetX,
			rawGyro.y - m_calibration.offsetY,
			rawGyro.z - m_calibration.offsetZ
		};
	}

	// Vraù kalibrovanou orientaci (relativnÌ v˘Ëi vp¯ed)
	TripleXYZ<float> GetCalibratedOrientation(const TripleXYZ<float>& rawOri) const
	{
		if (!m_orientationCalibration.isCalibratedOrientation)
		{
			return rawOri;
		}

		// VypoËti relativnÌ orientaci
		return {
			rawOri.x - m_orientationCalibration.neutralX,
			rawOri.y - m_orientationCalibration.neutralY,
			rawOri.z - m_orientationCalibration.neutralZ
		};
	}

	// Kontrluje, zda je gyroskop zkalibrov·n
	bool IsGyroCalibrated() const
	{
		return m_calibration.isCalibrated;
	}

	// Kontroluje, zda je orientace zkalibrov·na
	bool IsOrientationCalibrated() const
	{
		return m_orientationCalibration.isCalibratedOrientation;
	}

	// Reset vsech kalibrac
	void ResetAll()
	{
		m_calibration.isCalibrated = false;
		m_orientationCalibration.isCalibratedOrientation = false;
		m_calibration.offsetX = m_calibration.offsetY = m_calibration.offsetZ = 0.0f;
		m_orientationCalibration.neutralX = m_orientationCalibration.neutralY = m_orientationCalibration.neutralZ = 0.0f;
		m_gyroSamples.clear();
		m_orientationSamples.clear();
		std::cout << "All calibrations reset!" << std::endl;
	}

	// Vrati statistiku kalibrace
	void PrintCalibrationStatus() const
	{
		std::cout << "\n========== CALIBRATION STATUS ==========" << std::endl;
		std::cout << "Gyro Calibrated: " << (m_calibration.isCalibrated ? "YES" : "NO") << std::endl;
		if (m_calibration.isCalibrated)
		{
			std::cout << "  Offset X: " << m_calibration.offsetX << std::endl;
			std::cout << "  Offset Y: " << m_calibration.offsetY << std::endl;
			std::cout << "  Offset Z: " << m_calibration.offsetZ << std::endl;
		}
		std::cout << "Orientation Calibrated: " << (m_orientationCalibration.isCalibratedOrientation ? "YES" : "NO") << std::endl;
		if (m_orientationCalibration.isCalibratedOrientation)
		{
			std::cout << "  Neutral X (Pitch): " << m_orientationCalibration.neutralX << std::endl;
			std::cout << "  Neutral Y (Roll): " << m_orientationCalibration.neutralY << std::endl;
			std::cout << "  Neutral Z (Yaw): " << m_orientationCalibration.neutralZ << std::endl;
			
		}
		std::cout << "========================================\n" << std::endl;
	}
};

