// WindowDayDream.cpp : Console application for Bluetooth device communication
//

#include "pch.h"
#include <string>
#include <vector>
#include <iostream>
#include "DDConnector.h"
#include "MCommand.h"
#include "BaseCycles.h"
#include "MouseController.h"
#include "DayDreamMouseMapper.h"

using namespace std;

#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothApis.lib")
#pragma comment(lib, "user32.lib")  // Linkování s user32.lib pro mouse input

// Globalni kontrolery pro OpenVR a ostatní moduly
struct _Controller
{
	double X = 0;
	double Y = 0;
	double Z = 0;
	double Yaw = 0;
	double Pitch = 0;
	double Roll = 0;
};

_Controller MyCtrl[1] = {};
int32_t Active = 0;

DDConnector connector;
// Globální connector pro OpenVR driver
DDConnector* g_connector = &connector;

// Globální mouse controller
MouseController* g_mouseController = nullptr;
DayDreamMouseMapper* g_mouseMapper = nullptr;

// Posledný stav trackpadu pro detekci zmÃ©n
struct TrackpadState
{
	float lastX = 0.5f;
	float lastY = 0.5f;
	bool lastTouching = false;
	bool lastClicking = false;
} g_trackpadState;

// Callback pro zpracování DayDream dat a myši
void ProcessDayDreamData()
{
	const DataDevice& device = TFirstDevice::data;

	if (g_mouseMapper)
	{
		// Trackpad: normalizace z [0, 1] na [-1, 1]
		float trackpadX = (device.xTouch > 0.01f && device.xTouch < 0.99f) ? device.xTouch : 0.5f;
		float trackpadY = (device.yTouch > 0.01f && device.yTouch < 0.99f) ? device.yTouch : 0.5f;

		// Detekce dotyku a kliku
		bool isTouching = (device.xTouch > 0.1f && device.xTouch < 0.9f) ||
		                   (device.yTouch > 0.1f && device.yTouch < 0.9f);
		bool isClicking = device.b.bTouch;

		// Zpracování trackpadu
		g_mouseMapper->ProcessTrackpad(trackpadX, trackpadY, isTouching, isClicking);

		// Zpracování tlačítek
		g_mouseMapper->ProcessButtons(device.b);

		// Zpracování IMU (gyroskop + orientace pro pohyb kurzoru)
		g_mouseMapper->ProcessIMU(device.ori, device.acc, device.gyr);
	}
}

int main(int argc, char* argv[]){
	std::cout << "=== Bluetooth Device Communication Console ===" << std::endl;
	std::cout << "=== DayDream Mouse Controller with Gyro ===" << std::endl;
	std::cout << "Starting device enumeration..." << std::endl << std::endl;
	
	try
	{
		// Inicializace Mouse Controlleru
		g_mouseController = new MouseController();
		g_mouseMapper = new DayDreamMouseMapper(g_mouseController);

		// ===== AXIS INVERSION CONFIGURATION =====
		// If axes are inverted after calibration, uncomment the lines below:
		// g_mouseMapper->SetInvertGyroX(true);   // Invert vertical axis (pitch)
		// g_mouseMapper->SetInvertGyroY(true);   // Invert horizontal axis (roll)
		// =========================================

		// Tisk napovedy
		DayDreamMouseMapper::PrintHelp();

		// ===== DIAGNOSTIC INFORMATION =====
		std::cout << "\n=== CURRENT CONFIGURATION ===" << std::endl;
		std::cout << "Gyro Sensitivity: " << MAPPER_GYRO_SENSITIVITY << std::endl;
		std::cout << "Gyro Filter Alpha: " << GYRO_FILTER_ALPHA << std::endl;
		std::cout << "Gyro Filter Threshold: " << GYRO_FILTER_THRESHOLD << std::endl;
		std::cout << "Motion Smoothing Alpha: " << MOTION_SMOOTHING_ALPHA << std::endl;
		std::cout << "Motion Velocity Damping: " << MOTION_VELOCITY_DAMPING << std::endl;
		std::cout << "Motion Min Threshold: " << MOTION_MIN_THRESHOLD << std::endl;
		std::cout << "Mouse Gyro Deadzone: " << MOUSE_GYRO_DEADZONE << std::endl;
		std::cout << "===============================\n" << std::endl;

		// Initialize and get device list
		auto deviceList = connector.getDeviceList();
		
		if (deviceList.empty())
		{
			std::cerr << "No compatible Bluetooth LE devices found." << std::endl;
			delete g_mouseMapper;
			delete g_mouseController;
			return 1;
		}
		
		std::cout << "Found " << deviceList.size() << " device(s)." << std::endl;
		std::cout << "Initializing devices..." << std::endl << std::endl;
		
		// Initialize first device
		TFirstDevice::EV::Ss::Start();
		connector.init(deviceList[0], (DDConnector::PFunc)(TFirstDevice::SomethingHappened));
		
		std::cout << "Device 0 initialized successfully." << std::endl;
		
		// Initialize second device if available
		if (deviceList.size() > 1)
		{
			TSecondDevice::EV::Ss::Start();
			connector.init(deviceList[1], (DDConnector::PFunc)(TSecondDevice::SomethingHappened));
			std::cout << "Device 1 initialized successfully." << std::endl;
		}
		
		std::cout << std::endl << "Listening for device data. Press Ctrl+C to exit..." << std::endl;
		std::cout << "========================================" << std::endl << std::endl;
		
		
		// Tisk napovedy pro kalibraci
		std::cout << "To start gyro calibration: HOLD MENU button for 2 seconds!" << std::endl << std::endl;
		
		// Keep the application running and listen for events
		while (true)
		{
			// Zpracování DayDream dat a ovládání myši
			ProcessDayDreamData();

			Sleep(10);  // Aktualizace 20x za sekundu (50ms interval)
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		if (g_mouseMapper) delete g_mouseMapper;
		if (g_mouseController) delete g_mouseController;
		return 1;
	}
	catch (...)
	{
		std::cerr << "An unknown error occurred." << std::endl;
		if (g_mouseMapper) delete g_mouseMapper;
		if (g_mouseController) delete g_mouseController;
		return 1;
	}
	
	return 0;
}

