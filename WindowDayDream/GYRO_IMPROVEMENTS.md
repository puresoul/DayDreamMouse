# Vylepšené Gyroszkopické Øízení Kurzoru - Dokumentace

## Pøehled

Toto vylepšení implementuje hladký a pøesný pohyb kurzoru myši øízený gyroszkopem ovladaèe DayDream s dvojfázovou kalibrací.

## Hlavní Vylepšení

### 1. Vyhlazený Pohyb Gyroskopu

**MouseController.h - MoveMouseGyro()**
- Implementován pokroèilý filtr exponenciálního klouzavého prùmìru (EMA)
- Pøidáno velocity damping pro prediktivní filtraci
- Dead zone pro gyroskop (0.05 jednotek) eliminuje malý šum
- Sensitivity multiplikátor pro kontrolu intenzity pohybu
- Omezení maximálního rozsahu pohybu (prevent jump)

**Konfigurace v WindowDayDream.cpp:**
```cpp
g_mouseController->SetGyroSensitivity(2.0f);      // Nižší = citlivìjší (rozsah 0.5-5.0)
g_mouseController->SetGyroFilterAlpha(0.15f);     // Nižší = hladší (rozsah 0.1-0.3)
g_mouseController->SetGyroDeadzone(0.05f);        // Práh pro šum
```

### 2. Pokroèilý Filtr Gyroskopu

**GyroFilter.h - ApplyEMA()**
- Exponenciální klouzavý prùmìr (EMA) s velocity tracking
- Velocity damping koeficient (0.92) pro fyzikalnì vìrohodný pohyb
- Threshold na minimální pohyb pro eliminaci driftu
- Pøipravený Kalman filtr pro budoucí vylepšení

Parametry filtru:
- `m_alpha`: 0.15 (nižší = hladší, vyšší = responsivnìjší)
- `m_threshold`: 0.02 (minimální pohyb pro registraci)
- `m_velocityDamping`: 0.92 (prediktivní filtrování)

### 3. Dvoustupòová Kalibrace

**GyroCalibrator.h - Nové Funkce:**

#### Stupeò 1: Kalibrace Gyro Driftu
```cpp
void StartCalibration()        // Spustí sbìr 100 vzorkù v klidu
bool AddSample(...)           // Pøidá vzorek
void CompleteCalibration()    // Vypoèítá offset (støední hodnota)
```

Poèet vzorkù: 100 (cca 5 sekund pøi 20 Hz)
Cíl: Eliminovat konstantní offset gyroskopu

#### Stupeò 2: Kalibrace Orientace (Vpøed)
```cpp
void StartOrientationCalibration()        // Spustí sbìr orientaèních dat
bool AddOrientationSample(...)            // Pøidá vzorek orientace
void CompleteOrientationCalibration()     // Vypoèítá neutralní polohu
```

Poèet vzorkù: 150 (cca 7-8 sekund)
Cíl: Zapamatovat si neutralní polohu (oblièej hledí vpøed)

**Výhody:**
- Automatická kalibrace pøi spuštìní
- Pøesná referencí pro pohyb vpøed
- Kompenzace individuálních rozdílù v orientaci senzorù

### 4. Vylepšené Mapování Vstupù

**DayDreamMouseMapper.h**

Nový stav kalibrace:
```cpp
enum CalibrationState {
    CALIB_NONE,           // Normální operace
    CALIB_GYRO_DRIFT,     // Fáze 1: Kalibrace driftu
    CALIB_ORIENTATION,    // Fáze 2: Kalibrace orientace
};
```

Nové funkce:
```cpp
bool IsCalibrating()                           // Zjistí, zda probíhá kalibrace
void StartOrientationCalibrationOnly()         // Manuální opìtnovaná kalibrace
void SetGyroFilterAlpha(float alpha)           // Dynamická úprava vyhlazení
```

## Diagram Toku Dat

```
DayDream Hardware (Gyro)
    ?
ProcessIMU() ? GyroCalibrator::GetCalibratedOrientation()
    ?
MouseController::MoveMouseGyro()
    ?
GyroFilter::ApplyEMA() [Vyhlazení + Velocity Damping]
    ?
Dead Zone + Sensitivity + Range Clamping
    ?
SetCursorPos() ? Windows Kurzor
```

## Parametry pro Jemné Ladìní

### Citlivost Pohybu
```cpp
// V WindowDayDream.cpp
g_mouseController->SetGyroSensitivity(2.0f);
// Nižší = citlivìjší (mìní na 0.5-5.0)
// 1.0 = standardní, 2.0 = ménì citlivý
```

### Vyhlazení Pohybu
```cpp
g_mouseController->SetGyroFilterAlpha(0.15f);
// 0.1-0.15 = velmi hladké (zpoždìní)
// 0.2-0.3 = støednì hladké (doporuèeno)
// 0.4-0.5 = responsivní (málo vyhlazení)
```

### Dead Zone
```cpp
g_mouseController->SetGyroDeadzone(0.05f);
// Rozsah 0.01-0.1
// Vyšší = ignoruje více šumu, ale mùže "pøeskakovat"
```

## Problémy a Øešení

### Problém: Kurzor pøeskakuje
**Øešení:** Snižte SetGyroSensitivity (napø. 1.0 místo 2.0)

### Problém: Pohyb je pøíliš zpoždìlý
**Øešení:** Zvyšte SetGyroFilterAlpha (napø. 0.25 místo 0.15)

### Problém: Kurzor se pohybuje bez dotyku
**Øešení:** Zvyšte SetGyroDeadzone (napø. 0.08 místo 0.05)

### Problém: Orientace není správná
**Øešení:** Spuste orientaèní kalibraci znovu s ovladaèem smìøujícím vpøed

## Budoucí Vylepšení

1. **Kalman Filtr** - Více sofistikovaný filtr pro ještì lepší stabilitu
2. **Adaptive Sensitivity** - Automatická úprava citlivosti podle rychlosti pohybu
3. **Motion Prediction** - Predikce pohybu na základì pøedchozích trendù
4. **Gesture Recognition** - Detekce gestù pro speciální akce
5. **Persistent Calibration** - Uložení kalibrace do souboru

## Konfigurace pro Rùzné Scenario

### Hraní Her (Maximální Responsivnost)
```cpp
SetGyroSensitivity(1.5f);      // Vyšší citlivost
SetGyroFilterAlpha(0.20f);     // Ménì vyhlazení
SetGyroDeadzone(0.03f);        // Nižší práh
```

### Psaní (Maximální Pøesnost)
```cpp
SetGyroSensitivity(3.0f);      // Nižší citlivost
SetGyroFilterAlpha(0.10f);     // Více vyhlazení
SetGyroDeadzone(0.08f);        // Vyšší práh
```

### Standardní Použití
```cpp
SetGyroSensitivity(2.0f);      // Výchozí
SetGyroFilterAlpha(0.15f);     // Standardní
SetGyroDeadzone(0.05f);        // Standardní
```

## Kódové Pøíklady Integrace

### Zapnutí/Vypnutí Gyro Módu
```cpp
g_mouseMapper->SetGyroMode(true);   // Gyro øízení
g_mouseMapper->SetGyroMode(false);  // Trackpad øízení
```

### Manuální Orientaèní Kalibrace
```cpp
// Když uživatel chce znovu nakalibrovat smìr vpøed
g_mouseMapper->StartOrientationCalibrationOnly();
```

### Úprava Vyhlazení Za Bìhu
```cpp
if (smoothingTooHigh) {
    g_mouseMapper->SetGyroFilterAlpha(0.20f);  // Zvýšit responsivnost
}
```

## Technické Detaily

### Osové Mapování
```
Gyro osí ? Kurzor osí
X (Pitch) ? Y (Vertikální)
Y (Roll)  ? X (Horizontální)
Z (Yaw)   ? Nevyužíváno
```

### Normalizace Hodnot
```
Raw Gyro [-1.0, 1.0] ? Filtr EMA ? Dead Zone ? Sensitivity
? Range Clamp [-1.0, 1.0] ? Screen Position [0, screenWidth]
```

---

**Autor:** DayDream Gyro Controller Project
**Aktualizováno:** 2024
**Verze:** 2.0 - Smooth Gyro with Calibration
