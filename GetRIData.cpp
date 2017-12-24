//****************************************************************
// Example Program for acquisition of 3D button data using RAWINPUT API
// Sandio 3D Mouse SDK function.
//
// Coptright sandio Technology Corp. 2006
//*****************************************************************
//
// GetRIData.cpp : Defines the entry point for the application.
//
//----------------------------------------------------------------

#include "stdafx.h"
#include "GetRIData.h"
#include <string>
#include "strsafe.h"
#include "EmitKeystrokes.h"
#define MAX_LOADSTRING 100

// Behavioral settings (#define or #undef as needed)
#undef  SHOW_DIAGNOSTICS               // Show verbose mouse stats besides 3D buttons in the main window
#undef  WATCH_MOUSE_MOVEMENT           // Listen to mouse movement events and include them in diagnostics above (if enabled)
#define DETECT_ONLY_SANDIO_2DMOUSE     // If listening to mouse movement events, ignore mice other than Sandio
#undef  DEBUG_SPEW                     // Send debug output to MSVC debug output window (which is a notoriously slow interface, or has been in prior MSVC versions)

#ifdef  _DEBUG
#define DEBUG_SPEW
#endif

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	TextBoxWndProc(HWND, UINT, WPARAM, LPARAM);

int RegRawInput(HWND hWnd);
int HID_Device(HWND hWnd, RAWINPUT* raw);

// Device state

#define S2DMNAME "\\\\?\\HID#Vid_19ca&Pid_0001&MI_00#"  // Sandio 3D mouse 
#define S3DMNAME "\\\\?\\HID#Vid_19ca&Pid_0001&MI_01#" // Sandio 3D mouse 

#ifdef SHOW_DIAGNOSTICS
DWORD dwVendorID, dwProductID1, dwProductID2, dwVersionNO;
#endif

#ifdef WATCH_MOUSE_MOVEMENT
HANDLE hSandio2DM;
DWORD dwDeltaX, dwDeltaY;
unsigned char cFirstByte, cSecondByte;
int Mouse_Device(HWND hWnd, RAWINPUT* raw);
SHORT sWheel;
#endif

HANDLE hSandio3DM;
ULONG rawButtons;
DWORD ByteSize3DRawData;
DWORD NumberOf3DInput;
BYTE  RawData3DButton[10];
BYTE  DPI = BYTE(-1);

DWORD keyDownWParam = 0, keyDownLParam = 0;

// Program state display
TCHAR szTempOutput[4096];
TCHAR szProgramState[256];
HWND hDisplayBox = NULL;
WNDPROC fOldDisplayWndProc = NULL;

//================================================================================
//
//
bool CompareDevName(TCHAR* pszStr1, TCHAR* pszStr2)
{
	// To filter the Sandio 2D mouse by compare device name
	//if (strcmp(S2DNAME, pszStr2) == 0)
	bool bCompare = false;
	size_t nLen;
	StringCchLength(pszStr1, 1024, &nLen);
	for (unsigned int j=4; j<nLen; j++)
	{
		char a,b;
		a = (pszStr1[j] >= 'a' && pszStr1[j] <='z') ? pszStr1[j]-32 : pszStr1[j];
		b = (pszStr2[j] >= 'a' && pszStr2[j] <='z') ? pszStr2[j]-32 : pszStr2[j];
		if (a == b)
		{
			bCompare = true;
			continue;
		}
		else
		{
			bCompare = false;
			break;
		}
	}
	return bCompare;
}

void UpdateWindowText() {
	StringCchPrintf(szTempOutput, sizeof(szTempOutput)/sizeof(szTempOutput[0]),
		TEXT("Config status: %s\r\n\r\n")
#ifdef SHOW_DIAGNOSTICS
		TEXT("USB/HID data\r\nVendor ID = 0x%04X\r\n")
		TEXT("Product ID = 0x%02x (for 3D mouse: Encrypted Ver.)\r\n")
		TEXT("Version number = %d\r\n")
		TEXT("\r\n")
		TEXT("2D Mouse Data Display\r\n")				
		TEXT("Delta X= % d , Delta Y= % d , Wheel= % i\r\n")
		TEXT("First word (binary notation)= %1u%1u%1u%1u%1u%1u%1u%1u %1u%1u%1u%1u%1u%1u%1u%1u\r\n")
		TEXT("Raw Buttons= %x")
		TEXT("\r\n")
		TEXT("3D Button Data Display\r\n")
		TEXT("Hid: dwSizeHid=%u dwCount=%u bRawData[0]=%u\r\n")
		TEXT("bRawData[1]=%u\r\n")
#endif
		TEXT("bRawData[2] DPI = %u\r\n")
		TEXT("bRawData[3] Top 3D Button: (Right=1 / Left=4 / Forward=2 / Backward=8) = %u\r\n")
		TEXT("bRawData[4] Right 3D Button: (Forward=4 / Backward=8 / Up=1 / Down=2) = %u\r\n")
		TEXT("bRawData[5] Left 3D Button: (Forward=1 / Backward=4 / Up=8 / Down=2) = %u\r\n")
		TEXT("\r\n")
		TEXT("Last Keypress:  VirtKey %d (0x%04x), ScanCode %d (0x%02x), flags 0x%02x")
		TEXT("\r\n"),
			szProgramState,
#ifdef SHOW_DIAGNOSTICS
			dwVendorID, dwProductID1, dwVersionNO, 
			dwDeltaX, dwDeltaY, sWheel, 
			(cSecondByte&0x80)>>7, (cSecondByte&0x40)>>6, (cSecondByte&0x20)>>5, (cSecondByte&0x10)>>4, 
			(cSecondByte&0x8)>>3, (cSecondByte&0x4)>>2, (cSecondByte&0x2)>>1, cSecondByte&0x1,
			(cFirstByte&0x80)>>7, (cFirstByte&0x40)>>6, (cFirstByte&0x20)>>5, (cFirstByte&0x10)>>4, 
			(cFirstByte&0x8)>>3, (cFirstByte&0x4)>>2, (cFirstByte&0x2)>>1, cFirstByte&0x1,
			rawButtons,
			ByteSize3DRawData, 
			NumberOf3DInput, 
			RawData3DButton[0], RawData3DButton[1], 
#endif
			RawData3DButton[2], RawData3DButton[3], RawData3DButton[4],
			RawData3DButton[5],
			keyDownWParam, keyDownWParam, (keyDownLParam >> 16) & 0xFF, (keyDownLParam >> 16) & 0xFF, (keyDownLParam >> 24) & 0xFF
		);
	SetWindowText(hDisplayBox, szTempOutput);
}

//========================================================================
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	// LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_A1, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// inintialize config
	const char *configFileName = "config.txt";
	if(strlen(lpCmdLine)) {
		configFileName = lpCmdLine;
	}
	ParseConfig(configFileName, szProgramState, sizeof(szProgramState)/sizeof(szProgramState[0]));

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_A1);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_A1);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_A1;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		200, 100, 700, 300, NULL, NULL, hInstance, NULL);
	//      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	hDisplayBox = CreateWindow(_T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY | ES_MULTILINE, 0, 0, 700, 300, hWnd, NULL, hInst, 0);
	UpdateWindowText();
	fOldDisplayWndProc = (WNDPROC)SetWindowLong(hDisplayBox, GWL_WNDPROC, (LONG)TextBoxWndProc);
	ShowWindow(hWnd, nCmdShow);
	RegRawInput(hWnd);

	return TRUE;
}

//========================================================================
int RegRawInput(HWND hWnd)
{
	int nRet = 0;

	UINT nDevices;
	PRAWINPUTDEVICELIST pRawInputDeviceList;
	if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
	{
		return -1;
	}
	pRawInputDeviceList = (RAWINPUTDEVICELIST*) malloc(sizeof(RAWINPUTDEVICELIST) * nDevices);
	GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
#ifdef DEBUG_SPEW
	StringCchPrintf(szTempOutput, 4096, TEXT("Number of raw input devices: %i\r\n"), nDevices);
	//MessageBox(NULL, szTempOutput, TEXT("raw input devices"), MB_OK);
	OutputDebugString(szTempOutput);
#endif

	TCHAR* chDevName;
	UINT cbInfoSize;

	for (unsigned int i=0; i<nDevices; i++)
	{
		if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, NULL, &cbInfoSize) < 0)
		{
			nRet = -2;
			break;
		}
		chDevName = (TCHAR*) malloc(cbInfoSize * sizeof(TCHAR));
		if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, chDevName, &cbInfoSize) < 0)
		{
			nRet = -3;
			break;
		}

#ifdef DETECT_ONLY_SANDIO_2DMOUSE
#ifdef WATCH_MOUSE_MOVEMENT
		if (CompareDevName(S2DMNAME, chDevName))
			hSandio2DM = pRawInputDeviceList[i].hDevice;
#endif
		if (CompareDevName(S3DMNAME, chDevName))
			hSandio3DM = pRawInputDeviceList[i].hDevice;
#endif
		free(chDevName);

		RID_DEVICE_INFO Dev_Info;
		Dev_Info.cbSize = sizeof(Dev_Info);
		cbInfoSize = Dev_Info.cbSize;
		if ( GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice,
			RIDI_DEVICEINFO, &Dev_Info, &cbInfoSize) < 0 )
		{
			nRet = -4;
			break;
		}

		switch (Dev_Info.dwType)
		{
			case RIM_TYPEMOUSE:
			{
				RID_DEVICE_INFO_MOUSE Dev_Mouse = Dev_Info.mouse;
			
				break;
			}

			case RIM_TYPEKEYBOARD:
			{
				RID_DEVICE_INFO_KEYBOARD Dev_Keyboard = Dev_Info.keyboard;
			
				break;
			}

			case RIM_TYPEHID:
			{
				if (pRawInputDeviceList[i].hDevice == hSandio3DM)
				{				
					RID_DEVICE_INFO_HID Dev_Hid = Dev_Info.hid;
#ifdef SHOW_DIAGNOSTICS
					dwProductID1 = Dev_Hid.dwProductId;
					dwVendorID = Dev_Hid.dwVendorId;
					dwVersionNO = Dev_Hid.dwVersionNumber;
#endif
					break;
				}
			}
		}
		
	}

	RAWINPUTDEVICE Rid[3];
	nDevices = 0;

	Rid[nDevices].usUsagePage = 0x01; 
	Rid[nDevices].usUsage = 0x08; // 08=Multiaxis, 05=Game Pad, 04=Joystick 
	Rid[nDevices].dwFlags = RIDEV_INPUTSINK;// | RIDEV_PAGEONLY; // 0;
	Rid[nDevices].hwndTarget = hWnd; //NULL;
	nDevices++;

#ifdef WATCH_MOUSE_MOVEMENT
	Rid[nDevices].usUsagePage = 0x01; 
	Rid[nDevices].usUsage = 0x02; // Mouse
	Rid[nDevices].dwFlags = RIDEV_INPUTSINK; // 0;
	Rid[nDevices].hwndTarget = hWnd;
	nDevices++;
#endif

	if (RegisterRawInputDevices(Rid, nDevices, sizeof(Rid[0])) == FALSE)
	{
		//registration failed. 
		MessageBox(NULL, TEXT("RawInput init failed."), TEXT("Register"), MB_OK);
	}



	// after the job, free the RAWINPUTDEVICELIST
	free(pRawInputDeviceList);

	// After successful registration, I am watching for the WM_INPUT messages in the WndProc:
	return nRet;
}
//

//==================================================================
/*
* Define the mouse button state indicators.
#define RI_MOUSE_LEFT_BUTTON_DOWN   0x0001  // Left Button changed to down.
#define RI_MOUSE_LEFT_BUTTON_UP     0x0002  // Left Button changed to up.
#define RI_MOUSE_RIGHT_BUTTON_DOWN  0x0004  // Right Button changed to down.
#define RI_MOUSE_RIGHT_BUTTON_UP    0x0008  // Right Button changed to up.
#define RI_MOUSE_MIDDLE_BUTTON_DOWN 0x0010  // Middle Button changed to down.
#define RI_MOUSE_MIDDLE_BUTTON_UP   0x0020  // Middle Button changed to up.

#define RI_MOUSE_BUTTON_1_DOWN      RI_MOUSE_LEFT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_1_UP        RI_MOUSE_LEFT_BUTTON_UP
#define RI_MOUSE_BUTTON_2_DOWN      RI_MOUSE_RIGHT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_2_UP        RI_MOUSE_RIGHT_BUTTON_UP
#define RI_MOUSE_BUTTON_3_DOWN      RI_MOUSE_MIDDLE_BUTTON_DOWN
#define RI_MOUSE_BUTTON_3_UP        RI_MOUSE_MIDDLE_BUTTON_UP

#define RI_MOUSE_BUTTON_4_DOWN      0x0040
#define RI_MOUSE_BUTTON_4_UP        0x0080
#define RI_MOUSE_BUTTON_5_DOWN      0x0100
#define RI_MOUSE_BUTTON_5_UP        0x0200

* If usButtonFlags has RI_MOUSE_WHEEL, the wheel delta is stored in usButtonData.
* Take it as a signed value.
#define RI_MOUSE_WHEEL              0x0400

* Define the mouse indicator flags.
#define MOUSE_MOVE_RELATIVE         0
#define MOUSE_MOVE_ABSOLUTE         1
#define MOUSE_VIRTUAL_DESKTOP    0x02  // the coordinates are mapped to the virtual desktop
#define MOUSE_ATTRIBUTES_CHANGED 0x04  // requery for mouse attributes
*/
//==================================================================
#ifdef WATCH_MOUSE_MOVEMENT
int Mouse_Device(HWND hWnd, RAWINPUT* raw)
{
#ifdef DEBUG_SPEW
	HRESULT hResult;

	if ( raw->data.mouse.usButtonFlags != 0 
		|| raw->data.mouse.usButtonData != 0
		|| raw->data.mouse.lLastX != 0
		|| raw->data.mouse.lLastY != 0 )
	{
		hResult = StringCchPrintf(szTempOutput, 4096/*STRSAFE_MAX_CCH*/, TEXT(
			"Mouse: usFlags  ulButtons  usFlags  usData  ulRawButtons  lLastX         lLastY        ulExtraInformation"));

		hResult = StringCchPrintf(szTempOutput, 4096/*STRSAFE_MAX_CCH*/, TEXT(
			//"Mouse: usFlags=     ulButtons=     usButtonFlags=     usButtonData=     ulRawButtons=     lLastX=     lLastY=     ulExtraInformation=    \r\n"
			"Mouse: %04X      %08X   %04X        %04X     %08X       %08X    %08X   %08X    \r\n"),
			raw->data.mouse.usFlags, 
			raw->data.mouse.ulButtons, 
			raw->data.mouse.usButtonFlags, 
			raw->data.mouse.usButtonData, 
			raw->data.mouse.ulRawButtons, 
			raw->data.mouse.lLastX, 
			raw->data.mouse.lLastY, 
			raw->data.mouse.ulExtraInformation);

			cSecondByte = (raw->data.mouse.usButtonFlags & 0xFF00) >> 8;
			cFirstByte = raw->data.mouse.usButtonFlags & 0x0FF;
			dwDeltaX = raw->data.mouse.lLastX;
			dwDeltaY = raw->data.mouse.lLastY;
			sWheel = (raw->data.mouse.usButtonData);
			rawButtons = (raw->data.mouse.ulRawButtons);
		
		OutputDebugString(szTempOutput);
	}
#endif
	return 0;
}
#endif

void __cdecl Decode3DSignal_mine(const unsigned __int8 *const a1, unsigned __int8 *const a2)
{
	uint16_t v2 = ((unsigned __int16)a1[6]) << 8 | a1[7];
	uint8_t v3 = uint8_t(240 * ((21 * uint32_t(v2) + 7) % 2048) / 2048);

	uint8_t val;
	val = a1[3] - v3;
	if(val & 0xF0) {
		a2[3] = 0;
	} else {
		a2[3] = val;
	}
	val = a1[4] - v3;
	if(val & 0xF0) {
		a2[4] = 0;
	} else {
		a2[4] = val;
	}
	val = a1[5] - v3;
	if(val & 0xF0) {
		a2[5] = 0;
	} else {
		a2[5] = val;
	}
}


//==================================================================
int Hid_Device(HWND hWnd, RAWINPUT* raw)
{
	ByteSize3DRawData = raw->data.hid.dwSizeHid;
	NumberOf3DInput = raw->data.hid.dwCount;
	memcpy(RawData3DButton, raw->data.hid.bRawData, sizeof(RawData3DButton));
	// ----  decryption ------
	Decode3DSignal_mine(RawData3DButton, RawData3DButton); // decryption of 3D button data
	// ---- end of decryption ---
	JoystickState state;
	for(int i = 0; i < state.numButtons; i++) {
		state.buttons[i] = RawData3DButton[3 + i];
	}
	bool changes = AcceptMouseState(state);

#ifndef SHOW_DIAGNOSTICS
	if(changes || DPI != RawData3DButton[2])
#endif
		UpdateWindowText();

	DPI = RawData3DButton[2];

#ifdef DEBUG_SPEW
	HRESULT hResult;

	hResult = StringCbPrintf(szTempOutput, 4096 /*STRSAFE_MAX_CCH*/, 
		TEXT("Hid: dwSizeHid=%4x dwCount=%4x bRawData=%08x \r\n"),
		raw->data.hid.dwSizeHid, 
		raw->data.hid.dwCount, 
		raw->data.hid.bRawData[0]);

	if (SUCCEEDED(hResult))
	{
		// whole message
		BYTE bRawData[4096];
	
		int nDataLen = (int)(raw->data.hid.dwSizeHid*raw->data.hid.dwCount);
		memcpy(bRawData, &raw->data.hid.bRawData[0], nDataLen);

		TCHAR *pszDebugTemp = szTempOutput;
		StringCbPrintf(pszDebugTemp, 4096-(pszDebugTemp-pszDebugTemp), "Hid: Data = ");
		pszDebugTemp += 12;
		for (int i=0; i<nDataLen; i++)
		{
			hResult = StringCbPrintf(pszDebugTemp, 4096-(pszDebugTemp-pszDebugTemp), "%.2X ", bRawData[i]);
			pszDebugTemp += 3;
		}

		StringCbPrintf(pszDebugTemp, 4096-(pszDebugTemp-pszDebugTemp), "\r\n");
		pszDebugTemp += 2;
		
		OutputDebugString(pszDebugTemp);
	}
	else
	{
	// TODO: write error handler
	}
#endif
	return 0;
}

//==================================================================
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_KEYDOWN:
		keyDownWParam = wParam;
		keyDownLParam = lParam;
		UpdateWindowText();
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_SIZE:
		SetWindowPos(hDisplayBox, NULL, 0, 0, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, SWP_NOZORDER);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_INPUT: 
	{
		UINT dwSize;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, 
						sizeof(RAWINPUTHEADER));
		BYTE localBuf[64];
		LPBYTE lpb = localBuf;
		if(sizeof(localBuf) < dwSize)
		{
			lpb = new BYTE[dwSize];
			if (lpb == NULL) 
			{
				return 0;
			}
		}

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, 
			sizeof(RAWINPUTHEADER)) != dwSize )
			MessageBox(NULL, TEXT("GetRawInputData doesn't return correct size!\n"), TEXT(""), MB_OK);

		RAWINPUT* raw = (RAWINPUT*)lpb;

/*
* Type of the raw input
#define RIM_TYPEMOUSE       0
#define RIM_TYPEKEYBOARD    1
#define RIM_TYPEHID         2
*/
#ifdef WATCH_MOUSE_MOVEMENT
		if (raw->header.dwType == RIM_TYPEMOUSE) 
		{
#ifdef DETECT_ONLY_SANDIO_2DMOUSE
			if (raw->header.hDevice == hSandio2DM)
			{
#endif
				Mouse_Device(hWnd, raw);
				UpdateWindowText();
#ifdef DETECT_ONLY_SANDIO_2DMOUSE
			}
#endif
		} 
		else 
#endif
		if (raw->header.dwType == RIM_TYPEHID) 
		{
			Hid_Device(hWnd, raw);
		} 
		if(lpb != localBuf) {
			delete[] lpb;
		}
		break; // return 0;
	} 
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK TextBoxWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_KEYDOWN) {
		keyDownWParam = wParam;
		keyDownLParam = lParam;
		UpdateWindowText();
	}
	return CallWindowProc(fOldDisplayWndProc, hWnd, message, wParam, lParam);
}
