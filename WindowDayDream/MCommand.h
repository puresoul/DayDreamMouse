#pragma once
#include <limits>
#include <tuple>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <setupapi.h>
#include <bluetoothapis.h>
#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothApis.lib")
#include "Triple.h"
#include "Timer.h"
#include "Events.h"
#include "ConstStack.h"
#include "MathBase.h"
#include "ParseByteArray.h"
#include "ProjectConsts.h"
#include "DeviceIteraction.h"
using namespace std;


struct DataDevice;
enum enumRunMode { nothing = 0, mouseRM, catiaRM };

struct DataButtons {
	bool bSoundPlus = false;/*11*/
	bool bSoundMinus = false;/*12*/
	bool bNearTouch = false;/*13*/
	bool bNearRun = false;/*14*/
	bool bTouch = false;/*15*/
};


template<class T, class... TT>
struct Checkable
{
	static constexpr void check()
	{
		T::check();
		if constexpr (sizeof...(TT) > 0) Checkable<TT...>::check();
	}
};

struct DataDevice
{
	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters;
	
	unsigned char(*bitData)[20];

	DataButtons b;

	TButtonCheck bTouch = TButtonCheck(&(b.bTouch), "bTouch");
	TButtonCheck bNRun = TButtonCheck(&(b.bNearRun), "bNRun");
	TButtonCheck bNTouch = TButtonCheck(&(b.bNearTouch), "bNTouch");
	TButtonCheck bSoundPlus = TButtonCheck(&(b.bSoundPlus), "bSoundPlus");
	TButtonCheck bSoundMinus = TButtonCheck(&(b.bSoundMinus), "bSoundMinus");

	std::tuple<TButtonCheck*, TButtonCheck*, TButtonCheck*, TButtonCheck*, TButtonCheck*> tp
	 { &bTouch, &bNRun, &bNTouch,&bSoundPlus,&bSoundMinus };	

	 DataDevice(int N)
	 {

	 }

	short xTouch = smallNumMax;
	short yTouch = smallNumMax;

   	TripleXYZ<float> ori;
	TripleXYZ<float> acc;
	TripleXYZ<float> gyr;


	void init()
	{
		constexpr float M_PI = 3.14159265358979323846f;
		constexpr float oriScale = 2.0f * M_PI / 4095.0f;      // Orientace [rad]
		constexpr float accScale = 8.0f * 9.8f / 4095.0f;       // Akcelerace [m/s²]
		constexpr float gyroScale = (2048.0f / 180.0f) * M_PI / 4095.0f; // Gyroskop [rad/s]

		// ===== ORIENTACE (ORI) - 13 bitů na souřadnici =====
		// xOri: Bajty 1-3, bity [1:0, 2:*, 3:7]
		int16_t xOriRaw = static_cast<int16_t>(
			(((*bitData)[1] & 0x03) << 11) |
			(((*bitData)[2] & 0xFF) << 3) |
			(((*bitData)[3] & 0x80) >> 5)
			);
		// Sign extension: (x << 19) >> 19 = (x << 3) >> 3
		ori.x = static_cast<float>((xOriRaw << 19) >> 19) * oriScale;

		// yOri: Bajty 3-4, bity [3:4-0, 4:*]
		int16_t yOriRaw = static_cast<int16_t>(
			(((*bitData)[3] & 0x1F) << 8) |
			((*bitData)[4] & 0xFF)
			);
		ori.y = static_cast<float>((yOriRaw << 19) >> 19) * oriScale;


		// zOri: Bajty 5-6, bity [5:*, 6:7-3]
		int16_t zOriRaw = static_cast<int16_t>(
			(((*bitData)[5] & 0xFF) << 5) |
			(((*bitData)[6] & 0xF8) >> 3)
			);
		ori.z = static_cast<float>((zOriRaw << 19) >> 19) * oriScale;

		// ===== AKCELERACE (ACC) - 13 bitů na souřadnici =====
		// xAcc: Bajty 6-8, bity [6:2-0, 7:*, 8:7-6]
		int16_t xAccRaw = static_cast<int16_t>(
			(((*bitData)[6] & 0x07) << 10) |
			(((*bitData)[7] & 0xFF) << 2) |
			(((*bitData)[8] & 0xC0) >> 6)
			);
		acc.x = static_cast<float>((xAccRaw << 19) >> 19) * accScale;

		// yAcc: Bajty 8-9, bity [8:5-0, 9:7-1]
		int16_t yAccRaw = static_cast<int16_t>(
			(((*bitData)[8] & 0x3F) << 7) |
			(((*bitData)[9] & 0xFE) >> 1)
			);
		acc.y = static_cast<float>((yAccRaw << 19) >> 19) * accScale;

		// zAcc: Bajty 9-11, bity [9:0, 10:*, 11:7-4]
		int16_t zAccRaw = static_cast<int16_t>(
			(((*bitData)[9] & 0x01) << 12) |
			(((*bitData)[10] & 0xFF) << 4) |
			(((*bitData)[11] & 0xF0) >> 4)
			);
		acc.z = static_cast<float>((zAccRaw << 19) >> 19) * accScale;

		// ===== GYROSKOP (GIR) - 13 bitů na souřadnici =====
		// xGyro: Bajty 11-13, bity [11:3-0, 12:*, 13:7]
		int16_t xGyroRaw = static_cast<int16_t>(
			(((*bitData)[11] & 0x0F) << 9) |
			(((*bitData)[12] & 0xFF) << 1) |
			(((*bitData)[13] & 0x80) >> 7)
			);
		gyr.x = static_cast<float>((xGyroRaw << 19) >> 19) * gyroScale;

		// yGyro: Bajty 13-14, bity [13:6-0, 14:7-2]
		int16_t yGyroRaw = static_cast<int16_t>(
			(((*bitData)[13] & 0x7F) << 6) |
			(((*bitData)[14] & 0xFC) >> 2)
			);
		gyr.y = static_cast<float>((yGyroRaw << 19) >> 19) * gyroScale;

		// zGyro: Bajty 14-16, bity [14:1-0, 15:*, 16:7-5]
		int16_t zGyroRaw = static_cast<int16_t>(
			(((*bitData)[14] & 0x03) << 11) |
			(((*bitData)[15] & 0xFF) << 3) |
			(((*bitData)[16] & 0xE0) >> 5)
			);

		gyr.z = static_cast<float>((zGyroRaw << 19) >> 19) * gyroScale;

		uint16_t xTouchRaw = static_cast<uint16_t>(
			(((*bitData)[16] & 0x1F) << 3) |
			(((*bitData)[17] & 0xE0) >> 5)
			);
		xTouch = static_cast<float>(xTouchRaw) / 255.0f;

		// yTouch: Bajty 17-18, bity [17:4-0, 18:7-5]
		uint16_t yTouchRaw = static_cast<uint16_t>(
			(((*bitData)[17] & 0x1F) << 3) |
			(((*bitData)[18] & 0xE0) >> 5)
			);
		yTouch = static_cast<float>(yTouchRaw) / 255.0f;

		b.bSoundPlus = getData<11>(bitData);
		b.bSoundMinus = getData<12>(bitData);
		b.bNearTouch = getData<13>(bitData);
		b.bNearRun = getData<14>(bitData);
		b.bTouch = getData<15>(bitData);
	}

	static constexpr auto checkAll = [](auto&... tests) {(tests->check(), ...); };	
	void init2()
	{
		bitData = (unsigned char(*)[20])(ValueChangedEventParameters->CharacteristicValue->Data);
		init();
		std::apply(checkAll, tp);
	};

};

struct DataDeviceList {
	ConstStack<DataButtons, 200> b;
	void add(const DataDevice& t) {	
		b.add(t.b);
	}
}static actionPack;


template<class D>
void SomethingHappenedFirst
(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter,	PVOID Context);//, DataDevice* r);

template<class D>
void SomethingHappenedSecond
(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter,	PVOID Context);//, DataDevice* r);


template <int i>
struct Device
{
	using EV = Events<Device<i>>;
	static DataDevice data;
	static  void (*SomethingHappened)
		(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context);
};

template <class T>struct getTDeviceN {
	static constexpr int N = -1;
};

template <int InN>struct getTDeviceN<Device<InN>>
{
	static constexpr int N = InN;
};

using TFirstDevice = Device<0>; DataDevice TFirstDevice::data(getTDeviceN<TFirstDevice>::N);
void (*TFirstDevice::SomethingHappened)(BTH_LE_GATT_EVENT_TYPE, PVOID, PVOID)
= [](BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context) {
	SomethingHappenedFirst<TFirstDevice>(EventType, EventOutParameter, Context);// , & TMouse::data);
};


using TSecondDevice = Device<1>; DataDevice TSecondDevice::data(getTDeviceN<TSecondDevice>::N);//fist time its just show data
void (*TSecondDevice::SomethingHappened)(BTH_LE_GATT_EVENT_TYPE, PVOID, PVOID)
= [](BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context) 
{
		SomethingHappenedSecond<TSecondDevice>(EventType, EventOutParameter, Context);// , & TShow::data);
};

