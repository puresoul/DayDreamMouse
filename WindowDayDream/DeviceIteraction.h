#pragma once

#include "ProjectConsts.h"
#include <string>
#include "Delegate.h"

static constexpr int MaxPack = 60;
static constexpr int SmallPack = 35;
static constexpr int ZN = 3;
static constexpr int CountTicktoZero = 40;

#define checkLongClick
#define checkHolddown
#define checkMultiTab

constexpr bool _checkMultiTab_ = true;
constexpr bool _checkHolddown_ = true;
constexpr bool _checkLongClick_ = true;

// Simplified button check structure
struct TButtonCheck
{
	TButtonCheck(const bool* initiator, const char* name)
		: _initiator(initiator), _name(name) {}

	const char* _name = "test";
	const bool* _initiator = nullptr;
	int countClick = 0;
	int countZero = MaxPack;
	
	void (*beginClick)() = nullptr;
	void (*runHolddownClick)() = nullptr;
	void (*beginBigClick)() = nullptr;
	void (*beginHolddownClick)() = nullptr;
	void (*endBigClick)() = nullptr;
	void (*endHolddownClick)() = nullptr;
	void (*beginZero)() = nullptr;
	void (*endBigZero)() = nullptr;
	void (*endZero)() = nullptr;

#ifdef DELEGATE_CHECK
	DelegateV<void> endClick;
#else
	void (*endClick)() = nullptr;
#endif

	void _beginClick() {
		outEventMessage(_name, "beginClick");
		if (beginClick) beginClick();
	}

	void _runHolddownClick() {
		if (runHolddownClick) runHolddownClick();
	}

	void _endClick() {
		outEventMessage(_name, "endClick");
		if (endClick) endClick();
	}

	void check() {
		if ((*_initiator) || (countZero < ZN)) {
			if (!countClick) {
				_beginClick();
			} else if (countClick == SmallPack) {
				if (beginBigClick) beginBigClick();
			} else if (countClick == MaxPack) {
				if (beginHolddownClick) beginHolddownClick();
			} else if (countClick > MaxPack) {
				_runHolddownClick();
			}
			++countClick;
			if (!(*_initiator)) ++countZero; else countZero = 0;
		} else {
			if (countZero < MaxPack) {
				if (countZero == ZN) {
					if (countClick < SmallPack) _endClick();
					else if (countClick < MaxPack) {
						if (endBigClick) endBigClick();
					} else {
						if (endHolddownClick) endHolddownClick();
					}
				}
				countClick = 0;
				++countZero;
			}
		}
	}
};

// Simplified touch check structure
struct TTouchCheck
{
	TTouchCheck(const smallNum* X, const smallNum* Y, const char* name)
		: _X(X), _Y(Y), _name(name) {}

	const char* _name = "test";
	const smallNum* _X = nullptr;
	const smallNum* _Y = nullptr;
	smallNum Xbeg = smallNumMax;
	smallNum Ybeg = smallNumMax;
	int countTick = 0;
	
	void (*beginMove)() = nullptr;
	void (*runMove)() = nullptr;
	void (*endMove)() = nullptr;

	void _beginMove() {
		if (beginMove) beginMove();
	}

	void _runMove() {
		if (runMove) runMove();
	}

	void _endMove() {
		if (endMove) endMove();
	}

	void check() {
		if ((*_X) | (*_Y)) {
			if (Xbeg == smallNumMax) {
				Xbeg = *_X;
				Ybeg = *_Y;
				_beginMove();
			} else {
				++countTick;
				_runMove();
			}
		} else {
			if (Xbeg != smallNumMax) {
				_endMove();
				Xbeg = smallNumMax;
				Ybeg = smallNumMax;
			}
		}
	}
};

// Simplified AGMT check structure
static int w = 0;
struct TAGMCheck
{
	TAGMCheck(const decimal& xAcross, const decimal& yAlong, const decimal& zFlat, const char* name)
		: _xAcross(xAcross), _yAlong(yAlong), _zFlat(zFlat), _name(name) {}

	const char* _name = "test";
	const decimal& _xAcross;
	const decimal& _yAlong;
	const decimal& _zFlat;

	decimal xAcrossPrev = decimalMax;
	decimal yAlongPrev = decimalMax;
	decimal zFlatPrev = decimalMax;
	bool beg = true;
	int countTick = 0;
	int countSameTickEnd = 0;
	int countDiffTickBegin = 0;
	
	void (*beginMove)() = nullptr;
	void (*runMove)() = nullptr;
	void (*endMove)() = nullptr;

	void _beginMove() {
		if (beginMove) beginMove();
	}

	void _runMove() {
		if (runMove) runMove();
	}

	void _endMove() {
		if (endMove) endMove();
	}

	void check() {
		if ((xAcrossPrev != _xAcross) || (yAlongPrev != _yAlong) || (zFlatPrev != _zFlat)) {
			if (beg) {
				++countDiffTickBegin;
				if (countDiffTickBegin == 5) {
					_beginMove();
					beg = false;
					countSameTickEnd = 0;
					countTick = 0;
				}
			} else {
				++countTick;
				countSameTickEnd = 0;
				_runMove();
			}
			xAcrossPrev = _xAcross;
			yAlongPrev = _yAlong;
			zFlatPrev = _zFlat;
		} else {
			if (!beg) {
				if (countSameTickEnd == 5) {
					_endMove();
					beg = true;
					countSameTickEnd = 0;
				} else {
					++countSameTickEnd;
					++countTick;
					_runMove();
				}
			} else {
				++countSameTickEnd;
				if (countSameTickEnd == 5) {
					countDiffTickBegin = 0;
				}
			}
		}
	}
};
