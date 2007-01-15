#ifndef _WIIMOTE_LIB_H
#define _WIIMOTE_LIB_H

#if (WINVER < 0x0501)
#error Need to compile for Win XP
#endif

#include <windows.h>
#ifdef __GNUC__
#include <ddk\hidsdi.h>
/* mingw doesn't have this defined?? (as of w32api-3.8) */
WINHIDSDI BOOL WINAPI HidD_SetOutputReport(HANDLE, PVOID, ULONG);
#else
/* winddk build wants these included */
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <hidsdi.h> 
#endif // __GNUC__

/*
 * for button maps, assign each byte to a Virtual Key Code 
 * http://msdn2.microsoft.com/en-us/library/ms645540.aspx
 */
typedef struct _WIIMOTE_MAP
{
	BYTE up;
	BYTE left;
	BYTE right;
	BYTE down;
	BYTE a;
	BYTE b;
	BYTE minus;
	BYTE home;
	BYTE plus;
	BYTE one;
	BYTE two;
} WIIMOTE_MAP;

/* 
 * Process a report from the wiimote.
 * If IR is enabled, it'll control the mouse.
 * ProcessAndGetReport writes the HID report to a BYTE array.
 * Make sure the array has at least space for 22 characters.
 */
int WiiM_ProcessEvent();
int WiiM_ProcessAndGetReport(LPBYTE);
/* Get report gets an incoming report but doesn't process it */
int WiiM_GetReport(LPBYTE buffer);

void WiiM_TogRumble();
void WiiM_TogLED(int);
/* Toggles IR to control the mouse with absolute positioning (like the Wii) */
void WiiM_TogIR_Abs();
/* control mouse with relative positioning (for FPS) */
void WiiM_TogIR_Rel(); /* TODO */
void WiiM_SetWiimoteKeyMap(WIIMOTE_MAP*);
/* key status is only updated whenever a key report is processed by running
 * one of the ProcessEvent functions */
WIIMOTE_MAP WiiM_GetWiimoteKeyStatus();

int WiiM_ConnectWiimote(LPBYTE);
void WiiM_CloseConn();


#endif // _WIIMOTE_LIB_H

