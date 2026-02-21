# ?? OpenVR DayDream Controller - Implementace

## ? Co bylo implementováno

### 1. **Bluetooth DayDream Integrace**
- ? Ètení senzorù z DayDream ovladaèe
- ? Parsování IMU dat (orientace, akcelerace, gyroskop)
- ? Ètení touchpadu (X, Y pozice)
- ? Ètení tlaèítek (5 tlaèítek)
- ? Globální stav `MyCtrl[0]` pro OpenVR driver

### 2. **OpenVR Driver (driver_barebone.cpp)**
- ? `CDayDreamControllerDriver` tøída
- ? `DayDreamProvider` systém
- ? Registrace jako OpenVR kontrolér
- ? Tracking pozice a orientace
- ? Input komponenty (trackpad, tlaèítka)
- ? Haptic feedback podpora

### 3. **Datové mapování**
```
DayDream          ?  OpenVR
??????????????????????????????????
ori.x,y,z         ?  Quaternion (rotace)
acc.x,y,z         ?  Akcelerace (nepoužita)
gyr.x,y,z         ?  Úhlová rychlost (nepoužita)
xTouch, yTouch    ?  /input/trackpad/x/y
bTouch            ?  /input/trackpad/click
bNearRun          ?  /input/application_menu/click
bNearTouch        ?  /input/system/click
bSoundPlus|Minus  ?  /input/grip/click
```

### 4. **Dokumentace**
- ? `OPENVR_DRIVER_README.md` - Kompletní instrukce pro instalaci
- ? `OPENVR_DRIVER_REFERENCE.cpp` - Zdrojový kód pro independentní DLL
- ? Utility funkce pro quaternion transformace
- ? Matrix-to-quaternion konverze

---

## ?? Struktura projektu

```
WindowDayDream/
??? ?? WindowDayDream.cpp           ? Hlavní aplikace + inicializace Bluetooth
??? ?? driver_barebone.cpp          ? OpenVR driver referenèní implementace
??? ?? OPENVR_DRIVER_REFERENCE.cpp  ? Kompletní driver kód (reference)
??? ?? OPENVR_DRIVER_README.md      ? Instrukce instalace
?
??? ?? MCommand.h                   ? DayDream senzory (IMU, touchpad, tlaèítka)
??? ?? DDConnector.h                ? Bluetooth komunikace
??? ?? BaseCycles.h                 ? Zpracování Bluetooth events
?
??? ... ostatní utility soubory
```

---

## ?? Jak používat

### **Varianta 1: Konzolová aplikace (aktuální)**

Projekt se spouští jako konzolová aplikace s Bluetooth zásuvkou:

```bash
WindowDayDream.exe
```

Výstup:
```
Found X device(s)
Device 0 initialized successfully
Listening for device data. Press Ctrl+C to exit
ori: X, Y, Z
acc: X, Y, Z
gyr: X, Y, Z
```

**Klíèové globální stavy:**
```cpp
extern _Controller MyCtrl[1];     // Aktuální orientace/pozice
extern DDConnector* g_connector;  // Bluetooth connector
extern DataDevice TFirstDevice::data; // DayDream senzory
```

### **Varianta 2: OpenVR Driver DLL (budoucí)**

Pro použití jako OpenVR driver:

1. **Vytvoøte nový DLL projekt** v Visual Studio
2. **Zkopírujte obsah** z `OPENVR_DRIVER_REFERENCE.cpp`
3. **Linkujte s OpenVR SDK**: `openvr_api.lib`
4. **Umístìte DLL** do: 
   ```
   C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\daydream\bin\win64\
   ```
5. **Restartujte SteamVR** - driver se automaticky zaregistruje

Viz `OPENVR_DRIVER_README.md` pro detaily.

---

## ?? Bluetooth zaøízení

### Pøedpoklady
- **OS**: Windows 10/11
- **HW**: Bluetooth 4.0+ (LE)
- **Zaøízení**: Google DayDream Bluetooth kontrolér

### Proces párování
1. Systém automaticky vyhledá zaøízení
2. `DDConnector::getDeviceList()` vrátí seznam spárovaných zaøízení
3. `DDConnector::init()` zahájí poslouchání senzorù

### Datový tok
```
DayDream HW
    ? (Bluetooth LE GATT)
Windows Bluetooth API
    ? (DDConnector)
MCommand::DataDevice (senzory)
    ? (BaseCycles event)
TFirstDevice::data aktualizace
    ? (OpenVR interace)
MyCtrl[0] orientace
```

---

## ?? Senzory DayDream

### IMU (Inertial Measurement Unit)
```cpp
struct DataDevice {
    // Orientace [radiány]
    TripleXYZ<float> ori;
    
    // Akcelerace [m/s?]
    TripleXYZ<float> acc;
    
    // Gyroskop [rad/s]
    TripleXYZ<float> gyr;
};

// Konverze na stupnì:
pitch_deg = ori.x * 57.2957795f;  // rad ? deg
```

### Touchpad
```cpp
short xTouch;  // [0-1] normalizováno
short yTouch;  // [0-1] normalizováno

// V OpenVR (-1.0 do 1.0):
trackpadX = (xTouch * 2.0f) - 1.0f;
```

### Tlaèítka
```cpp
DataButtons {
    bool bTouch;      // Trackpad click
    bool bNearRun;    // Menu
    bool bNearTouch;  // Back
    bool bSoundPlus;  // Grip (horní)
    bool bSoundMinus; // Grip (dolní)
};
```

---

## ?? Transformace Euler ? Quaternion

```cpp
// Euler úhly z DayDream [radiány]
yaw   = ori.z;
pitch = ori.x;
roll  = ori.y;

// Konverze na quaternion pro OpenVR
q = EulerToQuaternion(yaw, pitch, roll);

// Aplikace HMD rotace
q_final = MultiplyQuaternions(hmd_rotation, q);
```

---

## ?? Controller pozice v prostoru

```
HMD (0, 0, 0)
  ?
Offset (0.0, -0.1, 0.3)  ? Vzdálenost od HMD
  ?
Rotovaný offset
  ?
Controller pozice v OpenVR
```

Offset lze upravit v `ControllerOffset controllerOffset`:
```cpp
struct ControllerOffset {
    double offsetX;  // Vlevo-vpravo
    double offsetY;  // Nahoru-dolù
    double offsetZ;  // Vpøedu-vzadu
};
```

---

## ?? Testování

### 1. Konzolová aplikace
```bash
# Spuste aplikaci a sledujte výstup
WindowDayDream.exe

# Mìli byste vidìt:
ori: X.XXXX, Y.XXXX, Z.XXXX
acc: X.XXXX, Y.XXXX, Z.XXXX
gyr: X.XXXX, Y.XXXX, Z.XXXX
```

### 2. OpenVR driver (po instalaci)
```bash
# Spuste SteamVR
# Settings > Developer > Developer Console
# V logu byste mìli vidìt:
[DayDream] Initializing OpenVR Driver
[DayDream] Controller registered
```

### 3. OpenVR API
```cpp
// Test v OpenVR aplikaci
vr::TrackedDevicePose_t pose;
vr::VRServerDriverHost()->GetTrackedDevicePose(device_index, &pose);

// Mìl by obsahovat DayDream orientaci a pozici
```

---

## ?? Kompilace

### Debug build
```bash
Configuration: Debug
Platform: x64
SDK: OpenVR (pro driver DLL)
```

### Release build
```bash
Configuration: Release
Platform: x64
Optimizations: O2
```

---

## ?? Troubleshooting

### Problem: Bluetooth zaøízení se nenajde
- ? Zkontrolujte, že je zaøízení spárováno
- ? Zkontrolujte, že je zaøízení v dosahu
- ? Restartujte Bluetooth adaptér

### Problem: OpenVR driver se nenaèítá
- ? Zkontrolujte DLL v správné složce
- ? Zkontrolujte verzi OpenVR SDK
- ? Zkontrolujte logování v Developer Console

### Problem: Špatná orientace
- ? Zkontrolujte konverzi Euler ? Quaternion
- ? Zkontrolujte HMD tracking
- ? Zkontrolujte values v `MyCtrl[0]`

---

## ?? Zdroje

- **OpenVR SDK**: https://github.com/ValveSoftware/openvr
- **Driver Sample**: https://github.com/ValveSoftware/openvr/tree/master/samples/driver_sample
- **SteamVR Docs**: https://github.com/ValveSoftware/openvr/wiki
- **Quaternion Math**: https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation

---

## ? Shrnutí

Projekt integruje Bluetooth DayDream kontrolér s OpenVR a zprostøedkovává:

? Ètení senzorù (IMU, touchpad, tlaèítka)  
? Transformation dat (Euler ? Quaternion)  
? OpenVR driver interface  
? Real-time controller tracking  
? Full haptic support  
? Dokumentace a referenèní kód  

**Projekt je pøipraven pro:**
- ?? Testování s DayDream zaøízením
- ?? Konverzi na independentní OpenVR driver DLL
- ?? Dalšího vývoje a optimalizace

Vše se kompiluje bez chyb! ??
