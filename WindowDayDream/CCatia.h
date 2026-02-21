#pragma once
#include "MCatia.h"
#include <map>
#include <vector>
#include <iostream>

// Minimal CATIA wrapper for console application
class CCatia
{
	bool isInit = false;

public:
	CCatia() { }
	~CCatia() { }

	void initZero() { isInit = false; }

	void runZoomIn() { }
	void runZoomOut() { }
	void iniAll(double PitchX, double RollY, double YawZ) { }
	void iniAll(int procKey, double PitchX, double RollY, double YawZ) { }

} static Catia;