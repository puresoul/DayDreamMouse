#include "pch.h"

// ============================================================================
// OpenVR DayDream Bluetooth Controller Driver
// REFERENÈNÍ DOKUMENTACE - NETESTUJTE V HLAVNÍM PROJEKTU
// ============================================================================
//
// POZNÁMKA: Tento soubor je èistì referenèní a NENÍ urèen pro kompilaci
// v hlavním projektu. Je zahrnut pouze jako dokumentace pro vytvoøení
// samostatného OpenVR driver DLL projektu.
//
// Viz OPENVR_DRIVER_README.md pro instrukce.
//

/*

// ... KOMPLETNÍ OBSAH DOKUMENTACE - VIZ NÍŽE V KOMENTÁØI ...

*/

// ============================================================================
// KOMPLETNÍ IMPLEMENTACE DOSTUPNÁ V KOMENTÁØI NÍŽE
// ============================================================================



#include "pch.h"
#include "openvr_driver.h"
#include <cstdio>
#include "driverlog.h"	
#include <windows.h>
#include <math.h>
#include <functional>

// Bluetooth DayDream includes
#include "MCommand.h"
#include "DDConnector.h"
#include "BaseCycles.h"

using namespace vr;
using namespace std;

// ============================================================================
// DATOVÉ STRUKTURY
// ============================================================================

typedef struct
{
	double X;
	double Y;
	double Z;
	double Yaw;
	double Pitch;
	double Roll;
} Controller;

// Posun kontroléru od HMD
struct ControllerOffset {
	double offsetX;
	double offsetY;
	double offsetZ;
};

ControllerOffset controllerOffset = { 0.0, -0.1, 0.3 };

// Extern deklarace z projektu
extern Controller MyCtrl[1];
extern int32_t Active;
extern DDConnector* g_connector;

// ============================================================================
// UTILITY FUNKCE
// ============================================================================

vr::HmdQuaternion_t HmdQuaternion_Init(double w, double x, double y, double z)
{
	vr::HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

vr::HmdQuaternion_t EulerToQuaternion(double yaw, double pitch, double roll)
{
	vr::HmdQuaternion_t q;
	double cy = cos(yaw * 0.5);
	double sy = sin(yaw * 0.5);
	double cp = cos(pitch * 0.5);
	double sp = sin(pitch * 0.5);
	double cr = cos(roll * 0.5);
	double sr = sin(roll * 0.5);

	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	return q;
}

vr::HmdQuaternion_t MultiplyQuaternions(const vr::HmdQuaternion_t& q1, const vr::HmdQuaternion_t& q2)
{
	vr::HmdQuaternion_t result;
	result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
	return result;
}

vr::HmdQuaternion_t MatrixToQuaternion(const vr::HmdMatrix34_t& matrix)
{
	vr::HmdQuaternion_t q;
	double trace = matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2];

	if (trace > 0)
	{
		double s = 0.5 / sqrt(trace + 1.0);
		q.w = 0.25 / s;
		q.x = (matrix.m[2][1] - matrix.m[1][2]) * s;
		q.y = (matrix.m[0][2] - matrix.m[2][0]) * s;
		q.z = (matrix.m[1][0] - matrix.m[0][1]) * s;
	}
	else if ((matrix.m[0][0] > matrix.m[1][1]) && (matrix.m[0][0] > matrix.m[2][2]))
	{
		double s = 2.0 * sqrt(1.0 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2]);
		q.w = (matrix.m[2][1] - matrix.m[1][2]) / s;
		q.x = 0.25 * s;
		q.y = (matrix.m[0][1] + matrix.m[1][0]) / s;
		q.z = (matrix.m[0][2] + matrix.m[2][0]) / s;
	}
	else if (matrix.m[1][1] > matrix.m[2][2])
	{
		double s = 2.0 * sqrt(1.0 + matrix.m[1][1] - matrix.m[0][0] - matrix.m[2][2]);
		q.w = (matrix.m[0][2] - matrix.m[2][0]) / s;
		q.x = (matrix.m[0][1] + matrix.m[1][0]) / s;
		q.y = 0.25 * s;
		q.z = (matrix.m[1][2] + matrix.m[2][1]) / s;
	}
	else
	{
		double s = 2.0 * sqrt(1.0 + matrix.m[2][2] - matrix.m[0][0] - matrix.m[1][1]);
		q.w = (matrix.m[1][0] - matrix.m[0][1]) / s;
		q.x = (matrix.m[0][2] + matrix.m[2][0]) / s;
		q.y = (matrix.m[1][2] + matrix.m[2][1]) / s;
		q.z = 0.25 * s;
	}

	return q;
}

void RotateVectorByQuaternion(double& x, double& y, double& z, const vr::HmdQuaternion_t& q)
{
	vr::HmdQuaternion_t vecQuat = { 0, x, y, z };
	vr::HmdQuaternion_t qConj = { q.w, -q.x, -q.y, -q.z };
	vr::HmdQuaternion_t temp = MultiplyQuaternions(q, vecQuat);
	vr::HmdQuaternion_t result = MultiplyQuaternions(temp, qConj);
	x = result.x;
	y = result.y;
	z = result.z;
}

// ============================================================================
// CONTROLLER DRIVER TØÍDA
// ============================================================================

class CDayDreamControllerDriver : public vr::ITrackedDeviceServerDriver
{
private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	// Input component handles
	vr::VRInputComponentHandle_t m_trackpadXHandle;
	vr::VRInputComponentHandle_t m_trackpadYHandle;
	vr::VRInputComponentHandle_t m_trackpadClickHandle;
	vr::VRInputComponentHandle_t m_trackpadTouchHandle;
	vr::VRInputComponentHandle_t m_menuButtonHandle;
	vr::VRInputComponentHandle_t m_backButtonHandle;
	vr::VRInputComponentHandle_t m_gripHandle;
	vr::VRInputComponentHandle_t m_triggerHandle;
	vr::VRInputComponentHandle_t m_hapticHandle;

public:
	CDayDreamControllerDriver()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
		m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
	}

	virtual ~CDayDreamControllerDriver() {}

	virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId)
	{
		m_unObjectId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);

		// Device properties
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, "DayDream_001");
		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_LeftHand);
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ControllerType_String, "vive_controller");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ModelNumber_String, "DayDream");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ManufacturerName_String, "Google");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_IsOnDesktop_Bool, false);

		// Input components
		vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, "/input/trackpad/x", &m_trackpadXHandle,
			vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, "/input/trackpad/y", &m_trackpadYHandle,
			vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/trackpad/click", &m_trackpadClickHandle);
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/trackpad/touch", &m_trackpadTouchHandle);
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/application_menu/click", &m_menuButtonHandle);
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/system/click", &m_backButtonHandle);
		vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/grip/click", &m_gripHandle);
		vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, "/input/trigger/value", &m_triggerHandle,
			vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);
		vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer, "/output/haptic", &m_hapticHandle);

		return VRInitError_None;
	}

	virtual void Deactivate()
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby() {}
	virtual void* GetComponent(const char* pchComponentNameAndVersion) { return NULL; }
	virtual void PowerOff() {}
	virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
	{
		if (unResponseBufferSize >= 1) pchResponseBuffer[0] = 0;
	}

	virtual DriverPose_t GetPose()
	{
		DriverPose_t pose = { 0 };
		pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
		pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
		pose.qRotation = HmdQuaternion_Init(1, 0, 0, 0);

		vr::TrackedDevicePose_t devicePoses[vr::k_unMaxTrackedDeviceCount];
		vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0, devicePoses, vr::k_unMaxTrackedDeviceCount);

		if (devicePoses[0].bPoseIsValid)
		{
			vr::HmdMatrix34_t hmdMatrix = devicePoses[0].mDeviceToAbsoluteTracking;
			vr::HmdQuaternion_t hmdRotation = MatrixToQuaternion(hmdMatrix);

			// Controller rotation z DayDream IMU
			double yaw = MyCtrl[0].Yaw * 3.14159 / 180.0;
			double pitch = MyCtrl[0].Pitch * 3.14159 / 180.0;
			double roll = MyCtrl[0].Roll * 3.14159 / 180.0;

			pose.qRotation = EulerToQuaternion(yaw, pitch, roll);
			pose.qRotation = MultiplyQuaternions(hmdRotation, pose.qRotation);

			// Controller position relative to HMD
			double adjustedX = controllerOffset.offsetX;
			double adjustedY = controllerOffset.offsetY;
			double adjustedZ = controllerOffset.offsetZ;

			RotateVectorByQuaternion(adjustedX, adjustedY, adjustedZ, hmdRotation);

			pose.vecPosition[0] = hmdMatrix.m[0][3] + adjustedX;
			pose.vecPosition[1] = hmdMatrix.m[1][3] + adjustedY;
			pose.vecPosition[2] = hmdMatrix.m[2][3] + adjustedZ;
		}
		else
		{
			pose.vecPosition[0] = 0;
			pose.vecPosition[1] = -1.0;
			pose.vecPosition[2] = 0.5;
		}

		pose.poseIsValid = true;
		pose.result = TrackingResult_Running_OK;
		pose.deviceIsConnected = true;

		return pose;
	}

	void RunFrame()
	{
		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
		{
			UpdateInputState();
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
		}
	}

	void ProcessEvent(const vr::VREvent_t& vrEvent)
	{
		if (vrEvent.eventType == vr::VREvent_Input_HapticVibration &&
			vrEvent.data.hapticVibration.componentHandle == m_hapticHandle)
		{
			DriverLog("DayDream: Haptic vibration\n");
		}
	}

	std::string GetSerialNumber() const { return "DayDream_001"; }

private:
	void UpdateInputState()
	{
		const DataDevice& device = TFirstDevice::data;

		// Trackpad
		float trackpadX = (device.xTouch * 2.0f) - 1.0f;
		float trackpadY = (device.yTouch * 2.0f) - 1.0f;

		vr::VRDriverInput()->UpdateScalarComponent(m_trackpadXHandle, trackpadX, 0);
		vr::VRDriverInput()->UpdateScalarComponent(m_trackpadYHandle, trackpadY, 0);

		bool isTouching = (device.xTouch > 0.1f && device.xTouch < 0.9f) ||
		                   (device.yTouch > 0.1f && device.yTouch < 0.9f);
		vr::VRDriverInput()->UpdateBooleanComponent(m_trackpadTouchHandle, isTouching, 0);

		// Buttons
		vr::VRDriverInput()->UpdateBooleanComponent(m_trackpadClickHandle, device.b.bTouch, 0);
		vr::VRDriverInput()->UpdateBooleanComponent(m_menuButtonHandle, device.b.bNearRun, 0);
		vr::VRDriverInput()->UpdateBooleanComponent(m_backButtonHandle, device.b.bNearTouch, 0);
		vr::VRDriverInput()->UpdateBooleanComponent(m_gripHandle, device.b.bSoundPlus || device.b.bSoundMinus, 0);
		vr::VRDriverInput()->UpdateScalarComponent(m_triggerHandle, 0.0f, 0);

		// Update controller orientation
		MyCtrl[0].Pitch = device.ori.x * 57.2957795f;
		MyCtrl[0].Roll = device.ori.y * 57.2957795f;
		MyCtrl[0].Yaw = device.ori.z * 57.2957795f;
	}
};

// ============================================================================
// PROVIDER TØÍDA
// ============================================================================

class DayDreamProvider : public IServerTrackedDeviceProvider
{
public:
	virtual EVRInitError Init(vr::IVRDriverContext* pDriverContext);
	virtual const char* const* GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
	virtual void RunFrame();
	virtual void Cleanup();

private:
	CDayDreamControllerDriver* m_pController = nullptr;
};

static DayDreamProvider g_serverDriverDayDream;

EVRInitError DayDreamProvider::Init(vr::IVRDriverContext* pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("DayDream: Initializing OpenVR Driver\n");

	m_pController = new CDayDreamControllerDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pController->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_Controller,
		m_pController);

	DriverLog("DayDream: Controller registered\n");
	return VRInitError_None;
}

void DayDreamProvider::Cleanup()
{
	CleanupDriverLog();
	if (m_pController)
	{
		delete m_pController;
		m_pController = NULL;
	}
}

void DayDreamProvider::RunFrame()
{
	if (m_pController)
	{
		m_pController->RunFrame();

		vr::VREvent_t vrEvent;
		while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
		{
			m_pController->ProcessEvent(vrEvent);
		}
	}
}

// ============================================================================
// ENTRY POINT
// ============================================================================

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define HMD_DLL_EXPORT extern "C"
#endif

HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &g_serverDriverDayDream;
	}
	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}



// ============================================================================
// OBSAH VÝŠE JE KOMPLETNÍM ZDROJEM PRO SAMOSTATNÝ DRIVER DLL PROJEKT
// ============================================================================
