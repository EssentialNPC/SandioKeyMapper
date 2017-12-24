#pragma once

#include <stdint.h>
#include <string>

struct JoystickState {
	enum EButton { BTop, BRight, BLeft, numButtons };
	static const size_t    dirsPerButton = 4;

	typedef uint8_t        TButtonState;
	typedef uint32_t       TWholeState;

	union {
		struct {
			TButtonState   top;		// Top 3D Button:(Right=1 / Left=4 / Forward=2 / Backward=8)
			TButtonState   right;	// Right 3D Button:(Forward=4 / Backward=8 / Up=1 / Down=2)
			TButtonState   left;	// Left 3D Button:(Forward=1 / Backward=4 / Up=8 / Down=2)
		};
		TButtonState       buttons[numButtons];
		TWholeState        wholesale;
	};

	JoystickState()                                 { wholesale = 0; }
	JoystickState(uint32_t that)                    { wholesale = that; }
	const JoystickState &operator=(uint32_t that)   { wholesale = that; }
	operator bool() const                           { return !!wholesale; }
};

bool ParseConfig(const TCHAR *fileName, TCHAR *statusBuf, size_t nStatusChars);

void InitializeVirtualKeys();
bool AcceptMouseState(const JoystickState &newState);
