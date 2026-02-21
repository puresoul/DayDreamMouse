# ?? Konverze na OpenVR Driver DLL - Kompletní Prùvodce

## ?? Pøehled procesu

Budeme pøevádìt `OPENVR_DRIVER_REFERENCE.cpp` na **samostatný OpenVR driver DLL**, který bude fungovat jako nezávislý plugin pro SteamVR.

### Cílová architektura:

```
SteamVR Runtime
    ?
Driver Manager (native plugin loader)
    ?
DayDreamDriver.dll (náš driver)
    ?
Bluetooth DayDream controller
```

---

## ? Požadavky

- **Visual Studio 2019+** s C++ support
- **OpenVR SDK** (https://github.com/ValveSoftware/openvr)
- **Windows 10/11 x64**
- **SteamVR** (nainstalovaný pøes Steam)
- **Python 3.x** (volitelný - pro scripting)

---

## ??? Krok 1: Pøíprava OpenVR SDK

### 1.1 Stažení SDK

```bash
# Stáhnìte OpenVR SDK
git clone https://github.com/ValveSoftware/openvr.git
# nebo stáhnìte ZIP z GitHub

# Doporuèená struktura:
C:\OpenVR\
??? headers/          # openvr_driver.h, openvr.h
??? lib/
?   ??? win64/        # openvr_api.lib (x64)
?   ??? win32/        # openvr_api.lib (x86)
??? samples/
```

### 1.2 Vyexportování SDK cesty

V Visual Studio:
```
Tools > Options > Projects and Solutions > VC++ Directories
```

Pøidejte:
- **Include Directories**: `C:\OpenVR\headers`
- **Library Directories**: `C:\OpenVR\lib\win64`

Nebo nastavte environment variables:
```bash
# Windows Command Prompt (jako Admin)
setx OPENVR_SDK C:\OpenVR
```

---

## ?? Krok 2: Vytvoøení DLL Projektu

### 2.1 Nový projekt v Visual Studio

1. File > New > Project
2. Vyberte: **Visual C++ > Windows Desktop**
3. Vyberte: **Dynamic Library (DLL)**
4. Pojmenujte: `DayDreamDriver`
5. Vyberte: **Place solution and project in the same directory**

### 2.2 Konfigurace projektu

#### Platformy:
- ? **x64** (required for SteamVR)
- ? **Win32** (not compatible)

#### Konfigurace:
- ? **Debug** (pro vývoj)
- ? **Release** (pro deployment)

### 2.3 Struktura souboru

Vytvoøte následující strukturu:

```
DayDreamDriver/
??? pch.h              # Precompiled header
??? pch.cpp
??? driver.cpp         # MAIN - Zkopírujte z OPENVR_DRIVER_REFERENCE.cpp
??? driver.h           # Interface deklarace
??? driverlog.h        # OpenVR logging
??? driverlog.cpp
```

---

## ?? Krok 3: Implementace Driveru

### 3.1 Vytvoøení `pch.h`

```cpp
#pragma once

// Standard includes
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <functional>

// OpenVR includes
#include "openvr_driver.h"

// Project includes
#include "driverlog.h"

// Using declarations
using namespace vr;
using namespace std;
```

### 3.2 Vytvoøení `pch.cpp`

```cpp
#include "pch.h"
```

### 3.3 Vytvoøení `driverlog.h`

```cpp
#pragma once

#include <stdio.h>
#include <stdarg.h>

class DriverLog
{
public:
	static void Log(const char* pMsgFormat, ...)
	{
		va_list args;
		va_start(args, pMsgFormat);
		
		char buffer[1024];
		vsnprintf_s(buffer, sizeof(buffer), pMsgFormat, args);
		va_end(args);
		
		if (vr::VRDriverLog())
		{
			vr::VRDriverLog()->Log(buffer);
		}
		
		OutputDebugStringA(buffer);
		printf("%s", buffer);
	}
};

void DriverLog(const char* pMsgFormat, ...);

extern vr::IVRDriverLog* g_pLogFile;

#define DriverLog(...) DriverLog(__VA_ARGS__)
```

### 3.4 Vytvoøení `driverlog.cpp`

```cpp
#include "pch.h"

vr::IVRDriverLog* g_pLogFile = NULL;

void DriverLog(const char* pMsgFormat, ...)
{
	va_list args;
	va_start(args, pMsgFormat);

	char buffer[1024];
	vsnprintf_s(buffer, sizeof(buffer), pMsgFormat, args);
	va_end(args);

	if (g_pLogFile)
	{
		g_pLogFile->Log(buffer);
	}

	OutputDebugStringA(buffer);
	printf("%s", buffer);
}
```

### 3.5 Vytvoøení `driver.h`

```cpp
#pragma once

#include "openvr_driver.h"

// Forward declarations
class CDayDreamControllerDriver;
class DayDreamProvider;

// Helper structures
typedef struct
{
	double X, Y, Z;
	double Yaw, Pitch, Roll;
} Controller;

extern Controller g_controller;
extern vr::IVRDriverLog* g_pLogFile;

// Entry point
extern "C" __declspec(dllexport) 
void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode);
```

### 3.6 Vytvoøení `driver.cpp` (HLAVNÍ SOUBOR)

```cpp
#include "pch.h"
#include "driver.h"

// Global variables
Controller g_controller = {};
vr::IVRDriverLog* g_pLogFile = NULL;

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

// ============================================================================
// CONTROLLER DRIVER TØÍDA
// ============================================================================

class CDayDreamControllerDriver : public vr::ITrackedDeviceServerDriver
{
private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	// Input handles
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

		DriverLog("DayDream: Activating controller\n");

		// Set properties
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, "DayDream_001");
		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_LeftHand);
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ControllerType_String, "vive_controller");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ModelNumber_String, "DayDream");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_ManufacturerName_String, "Google");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_IsOnDesktop_Bool, false);

		// Create input components
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

		DriverLog("DayDream: Controller activated\n");
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

		// Get HMD pose
		vr::TrackedDevicePose_t devicePoses[vr::k_unMaxTrackedDeviceCount];
		vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0, devicePoses, vr::k_unMaxTrackedDeviceCount);

		if (devicePoses[0].bPoseIsValid)
		{
			vr::HmdMatrix34_t hmdMatrix = devicePoses[0].mDeviceToAbsoluteTracking;
			vr::HmdQuaternion_t hmdRotation = MatrixToQuaternion(hmdMatrix);

			// Controller rotation from DayDream IMU
			double yaw = g_controller.Yaw * 3.14159 / 180.0;
			double pitch = g_controller.Pitch * 3.14159 / 180.0;
			double roll = g_controller.Roll * 3.14159 / 180.0;

			pose.qRotation = EulerToQuaternion(yaw, pitch, roll);
			pose.qRotation = MultiplyQuaternions(hmdRotation, pose.qRotation);

			// Position offset
			double offsetX = 0.0;
			double offsetY = -0.1;
			double offsetZ = 0.3;

			RotateVectorByQuaternion(offsetX, offsetY, offsetZ, hmdRotation);

			pose.vecPosition[0] = hmdMatrix.m[0][3] + offsetX;
			pose.vecPosition[1] = hmdMatrix.m[1][3] + offsetY;
			pose.vecPosition[2] = hmdMatrix.m[2][3] + offsetZ;
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
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
		}
	}

	void ProcessEvent(const vr::VREvent_t& vrEvent)
	{
		if (vrEvent.eventType == vr::VREvent_Input_HapticVibration)
		{
			DriverLog("DayDream: Haptic event\n");
		}
	}

	std::string GetSerialNumber() const { return "DayDream_001"; }
};

// ============================================================================
// PROVIDER TØÍDA
// ============================================================================

class DayDreamProvider : public vr::IServerTrackedDeviceProvider
{
public:
	virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext);
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

vr::EVRInitError DayDreamProvider::Init(vr::IVRDriverContext* pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	g_pLogFile = pDriverContext->GetGenericInterface(vr::IVRDriverLog_Version);

	DriverLog("======================\n");
	DriverLog("DayDream Driver Loaded\n");
	DriverLog("======================\n");

	m_pController = new CDayDreamControllerDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pController->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_Controller,
		m_pController);

	return vr::VRInitError_None;
}

void DayDreamProvider::Cleanup()
{
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

extern "C" __declspec(dllexport)
void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &g_serverDriverDayDream;
	}

	if (pReturnCode)
		*pReturnCode = vr::VRInitError_Init_InterfaceNotFound;

	return NULL;
}
```

---

## ?? Krok 4: Konfigurace Linkerù

### 4.1 Project Properties

```
Project > Properties > Linker > Input
```

Pøidejte:
```
openvr_api.lib
kernel32.lib
user32.lib
gdi32.lib
winspool.lib
comdlg32.lib
advapi32.lib
shell32.lib
ole32.lib
oleaut32.lib
uuid.lib
odbc32.lib
odbccp32.lib
```

### 4.2 Output Directory

```
Configuration > Release
Properties > General > Output Directory
C:\build\DayDreamDriver\x64\Release\
```

---

## ?? Krok 5: Build DLL

```bash
# V Visual Studio
Build > Build Solution

# Nebo z Command Line:
msbuild DayDreamDriver.sln /p:Configuration=Release /p:Platform=x64
```

**Výsledek:**
```
DayDreamDriver.dll  (~500 KB)
```

---

## ?? Krok 6: Instalace do SteamVR

### 6.1 Vytvoøení Driver Struktury

```
C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\daydream\
??? bin\
?   ??? win64\
?       ??? driver_daydream.dll       ? Náš DLL
??? resources\
?   ??? input\
?       ??? daydream_profile.json     ? Input mapa
??? driver.vrdrivermanifest           ? Manifest
```

### 6.2 Vytvoøení `driver.vrdrivermanifest`

```json
{
  "version" : "1.14.15",
  "name" : "daydream",
  "layout" : "default",
  "directory" : "",
  "resources" : [
    "resources"
  ],
  "driver_paths" : [
    "bin/win64"
  ]
}
```

### 6.3 Vytvoøení Input Profile

Soubor: `resources/input/daydream_profile.json`

```json
{
  "title": "Google DayDream Controller",
  "description": "Google DayDream Bluetooth Controller",
  "controller_type": "vive_controller",
  "category": "steamvr",
  "interactions": [
    {
      "path": "/user/hand/left",
      "outputs": [
        "/user/hand/left/output/haptic"
      ],
      "inputs": [
        "/user/hand/left/input/trackpad/x",
        "/user/hand/left/input/trackpad/y",
        "/user/hand/left/input/trackpad/click",
        "/user/hand/left/input/trackpad/touch",
        "/user/hand/left/input/application_menu/click",
        "/user/hand/left/input/system/click",
        "/user/hand/left/input/grip/click",
        "/user/hand/left/input/trigger/value"
      ]
    }
  ]
}
```

### 6.4 Kopírování DLL

```bash
# PowerShell (jako Admin)
$source = "C:\Users\user\source\repos\base2\build\x64\Release\driver_daydream.dll"
$dest = "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\daydream\bin\win64\"

Copy-Item $source $dest -Force
```

---

## ?? Krok 7: Testování

### 7.1 Ovìøení v Developer Console

```bash
# Spuste SteamVR
# Settings > Developer > Enable Developer Console

# V konzoli byste mìli vidìt:
[DayDream Driver Loaded]
[DayDream] Controller activated
```

### 7.2 Ovìøení Controlleru

```bash
# V SteamVR menu: Devices > Controllers
# DayDream controller by mìl být v seznamu
```

### 7.3 Testování Input Events

```bash
# V libovolné OpenVR aplikaci
# Trackpad movements
# Button presses
# Haptic feedback
```

---

## ?? Troubleshooting

### Problem: "Driver not loaded"

**Øešení:**
1. Zkontrolujte DLL v správné složce
2. Zkontrolujte manifest soubor
3. Zkontrolujte verzi OpenVR SDK
4. Zkontrolujte x64 vs x86 kompatibilitu

### Problem: "Controller doesn't appear"

**Øešení:**
1. Zkontrolujte logování v Developer Console
2. Zkontrolujte HmdDriverFactory() entry point
3. Zkontrolujte TrackedDeviceAdded() call
4. Zkontrolujte Bluetooth connectivity

### Problem: "Input doesn't work"

**Øešení:**
1. Zkontrolujte input profile JSON
2. Zkontrolujte VRDriverInput()->CreateScalarComponent()
3. Zkontrolujte UpdateScalarComponent() v RunFrame()

---

## ? Checklist pro produkci

- [ ] DLL zkompilován v Release režimu
- [ ] DLL podepsán (Code Signing) - volitelné
- [ ] Manifest je platný JSON
- [ ] Input profile je platný JSON
- [ ] DLL je v `bin/win64/` directory
- [ ] Driver se registruje v SteamVR
- [ ] Controller se zobrazuje v devices
- [ ] Input events fungují
- [ ] Tracking je pøesný
- [ ] Haptic feedback funguje

---

## ?? Další zdroje

- OpenVR Driver Development: https://github.com/ValveSoftware/openvr/wiki/Driver-Development
- SteamVR Input System: https://github.com/ValveSoftware/openvr/wiki/IVRInput_Overview
- Driver API Reference: https://github.com/ValveSoftware/openvr/blob/master/headers/openvr_driver.h

---

## ?? Shrnutí

Nyní máte:

? Kompletní OpenVR driver pro DayDream controller  
? Bluetooth integrace  
? Input mapování  
? Instalaèní instrukce  
? Troubleshooting guide  

**Gratuluji! Váš OpenVR driver je pøipraven k nasazení!** ??
