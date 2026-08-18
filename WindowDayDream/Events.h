#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Triple.h"
#include "MathBase.h"

using namespace std;

enum enumRunMode;

template<class T, class... TT>
struct Startable
{
	static constexpr void Start()
	{
		T::Start();
		if constexpr (sizeof...(TT) > 0) Startable<TT...>::Start();
	}
};

// Simple event logging
static std::vector<string> split(const string& s1, const string& s, char delim) {
	std::vector<string> result;
	result.push_back(s1);
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		result.push_back(item);
	}
	return result;
};

#define DEFINE_ENUM(EnumType, first_value, first_name, ...) \
    enum EnumType { first_name = first_value, __VA_ARGS__ }; \
  const std::vector<std::string> EnumType##Name = split(#first_name, #__VA_ARGS__,',');

DEFINE_ENUM(scenarioN, 0, NO_ACTION, CALIBRATE_HORISONTAL, CALIBRATE_ROTATE, CALIBRATE_VERTICAL,
	SELECT_MODE, MOUSE_MODE, CATIA_MODE);

DEFINE_ENUM(calibrateN, 0, NO_CALIBRATE, CALIBRATE_BYFILE, CALIBRATE_INSESSION);

template <class T> struct getTDeviceN;

// Minimal event system for console application
template<class TData>
struct Events
{	
	static constexpr int N = getTDeviceN<TData>::N;
	
	struct scCalibrate
	{
		static void Start() { }
		static void pBegClick() { }
		static void pRightZClick() { }
		static void pRollYClick() { }
		static void pUpXClick() { }
	};
	   	  
	struct scModeManager
	{
		static void Start() { }
		static void pBegClick() { }
	};
	   	  	
	using Ss = Startable<scCalibrate, scModeManager>;
};

