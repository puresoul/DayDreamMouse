# OpenVR DayDream Bluetooth Controller Driver

## Pøehled

Tento projekt integruje OpenVR virtuální ovladaè s Bluetooth DayDream øadièem. Aplikace ète senzorová data (IMU, touchpad, tlaèítka) z DayDream ovladaèe a zpøístupòuje je jako standardní OpenVR kontrolér.

## Struktura projektu

```
WindowDayDream/
??? WindowDayDream.cpp         # Hlavní aplikace - inicializace Bluetooth
??? MCommand.h                 # Parsování DayDream senzorù
??? driver_barebone.cpp        # OpenVR driver reference (zakomentován)
??? OPENVR_DRIVER_REFERENCE.cpp # Kompletní OpenVR driver implementace
??? ... ostatní utility soubory
```

## Architektura

```
Bluetooth HW (DayDream)
    ?
DDConnector (Windows Bluetooth API)
    ?
MCommand::DataDevice (parsování dat)
    ?
MyCtrl[0] globální stav
    ?
OpenVR Driver (CDayDreamControllerDriver)
    ?
SteamVR / OpenVR aplikace
```

## Mapování dat

### Senzory DayDream ? OpenVR

| DayDream | OpenVR | Typ | Popis |
|----------|--------|-----|-------|
| `ori.x, ori.y, ori.z` | Quaternion | float[4] | Orientace (Euler ? Quaternion) |
| `acc.x, acc.y, acc.z` | - | float[3] | Akcelerace (m/s?) |
| `gyr.x, gyr.y, gyr.z` | - | float[3] | Úhlová rychlost (rad/s) |
| `xTouch, yTouch` | `/input/trackpad/x/y` | float | Trackpad pozice (0-1) |
| `bTouch` | `/input/trackpad/click` | bool | Stisk trackpadu |
| `bNearRun` | `/input/application_menu/click` | bool | Menu tlaèítko |
| `bNearTouch` | `/input/system/click` | bool | Back tlaèítko |
| `bSoundPlus \| bSoundMinus` | `/input/grip/click` | bool | Grip tlaèítko |

## Instalace OpenVR Driver

### Krok 1: Vytvoøení DLL projektu

```bash
# Vytvoøte novı Visual Studio projekt
# File > New > Project > Visual C++ > DLL (Dynamic Library)
# Pojmenujte: DayDreamDriver
```

### Krok 2: Konfigurace projektu

1. **Include paths:**
   ```
   Properties > VC++ Directories > Include Directories
   Add: $(OpenVR_SDK)\headers
   ```

2. **Linking:**
   ```
   Properties > Linker > Input > Additional Dependencies
   Add: openvr_api.lib
   ```

3. **Library path:**
   ```
   Properties > Linker > General > Additional Library Directories
   Add: $(OpenVR_SDK)\lib\win64
   ```

### Krok 3: Implementace driveru

Zkopírujte obsah z `OPENVR_DRIVER_REFERENCE.cpp` do `driver_barebone.cpp` v novém projektu:

```cpp
#include "pch.h"
#include "openvr_driver.h"
// ... (zbytek OPENVR_DRIVER_REFERENCE.cpp)
```

### Krok 4: Build a instalace

1. **Zkompilujte projekt:**
   ```bash
   Build > Build Solution
   ```

2. **Umístìte DLL do SteamVR drivers:**
   ```
   C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\daydream\bin\win64\
   ```

3. **Vytvoøte driver.vrdrivermanifest soubor:**
   ```
   C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\daydream\
   ```

   Obsah:
   ```json
   {
     "version" : "1.14.15",
     "name" : "daydream",
     "layout" : "default",
     "directory" : "",
     "resources" : [
     ],
     "driver_paths" : [
       "bin/win64"
     ]
   }
   ```

### Krok 5: Restartujte SteamVR

```bash
# Zavøete a znovu otevøete SteamVR
# Driver by mìl bıt automaticky zaregistrován
```

## Ovìøení instalace

### Kontrola v SteamVR

1. Otevøete SteamVR
2. Kliknìte na menu (hamburger ikona)
3. Settings > Developer > Enable Developer Mode
4. V Developer Console zkontrolujte:
   ```
   [DriverManager] Loading driver: C:\...daydream\bin\win64\driver_barebone.dll
   [DayDream] Initializing OpenVR Driver
   [DayDream] Controller registered
   ```

### Testování v HMD

```bash
# Spuste SteamVR Desktop Theater Mode
# Mìl by vám zobrazit DayDream ovladaè jako Controller
```

## Problematika a øešení

### Problem 1: Driver se nenaèítá

**Øešení:**
- Zkontrolujte, e DLL je v správné sloce
- Zkontrolujte verzi OpenVR SDK
- Zkontrolujte, e je DLL správnì linkován s openvr_api.lib

### Problem 2: DayDream data se neètou

**Øešení:**
- Zkontrolujte, e Bluetooth zaøízení je spárováno
- Zkontrolujte logování v `DDConnector`
- Zkontrolujte, e `MyCtrl[0]` se aktualizuje v `UpdateInputState()`

### Problem 3: Pozice/Orientace je špatnì

**Øešení:**
- Zkontrolujte konverzi Euler úhlù v `EulerToQuaternion()`
- Zkontrolujte `ControllerOffset` hodnoty
- Zkontrolujte, e HMD je správnì trackován

## Datová struktura DayDream

```cpp
struct DataDevice {
    // Orientace [rad]
    TripleXYZ<float> ori;  // ori.x = pitch, ori.y = roll, ori.z = yaw
    
    // Akcelerace [m/s?]
    TripleXYZ<float> acc;  // acc.x, acc.y, acc.z
    
    // Gyroskop [rad/s]
    TripleXYZ<float> gyr;  // gyr.x, gyr.y, gyr.z
    
    // Touchpad [0-1]
    short xTouch;
    short yTouch;
    
    // Tlaèítka
    struct {
        bool bTouch;       // Stisk trackpadu
        bool bNearRun;     // Menu
        bool bNearTouch;   // Back
        bool bSoundPlus;   // Grip (horní)
        bool bSoundMinus;  // Grip (dolní)
    } b;
};
```

## Globální stavy

```cpp
// WindowDayDream.cpp
extern struct _Controller {
    double X, Y, Z;           // Pozice
    double Yaw, Pitch, Roll;  // Orientace [stupnì]
} MyCtrl[1];

extern int32_t Active;        // Status
extern DDConnector* g_connector; // Bluetooth connector
```

## Další zdroje

- **OpenVR SDK:** https://github.com/ValveSoftware/openvr
- **Driver Sample:** https://github.com/ValveSoftware/openvr/tree/master/samples/driver_sample
- **SteamVR Driver Docs:** https://github.com/ValveSoftware/openvr/wiki/IVRServerDriverHost

## Poznámky

- Driver vyaduje Windows 10 nebo novìjší
- DayDream ovladaè musí bıt spárován pøes Bluetooth
- SteamVR musí bıt spuštìn v "Desktop Theater Mode" nebo s HMD
- Orientace je aktualizována v reálném èase z IMU senzorù

## Kontakt

Pokud máte otázky nebo chyby, zkontrolujte:
1. Bluetooth spojení
2. OpenVR SDK verzi
3. Binární kompatibilitu (Debug/Release, x86/x64)
