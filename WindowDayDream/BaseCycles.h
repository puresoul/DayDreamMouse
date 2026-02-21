#pragma once
#include <iostream>

template<class D>
void SomethingHappenedFirst
(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	static constexpr int N = getTDeviceN<D>::N;
	D::data.ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;
	D::data.init2();
	actionPack.add(D::data);
	std::cout << "ori: " << D::data.ori.x << " , " << D::data.ori.y << " , " << D::data.ori.z << std::endl;
	std::cout << "acc: " << D::data.acc.x << " , " << D::data.acc.y << " , " << D::data.acc.z << std::endl;
	std::cout << "gyr: " << D::data.gyr.x << " , " << D::data.gyr.y << " , " << D::data.gyr.z << std::endl;
}

template<class D> static void SomethingHappenedSecond
(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	static constexpr int N = getTDeviceN<D>::N;
	D::data.ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;
	D::data.init();
	actionPack.add(D::data);
}
