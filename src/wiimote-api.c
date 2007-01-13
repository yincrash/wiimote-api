/* wiimote-api C library */

#ifdef _WIN64 /* winddk's build didn't like not having it defined */
#define _SIZE_T_DEFINED
#endif

#include "wiimote-api.h"

/* not public functions */
void WShowError2(LPCTSTR, LPCTSTR);
void WShowError(LPCTSTR);
void ParseReport(unsigned char*);
int LoopAction(LPBYTE, BOOL);

/* current controller status */
HANDLE wiimotehandle;
BYTE led[4] = {0, 0, 0, 0};
BYTE rumble = 0;
BYTE ir     = 0;
WIIMOTE_MAP wiimote_keys;
WIIMOTE_MAP wiimote_status;


void WShowError(LPCTSTR title)
{
	WShowError2(title, NULL);
}

void WShowError2(LPCTSTR title, LPCTSTR e_msg)
{
	char buffer[256];
	if(e_msg != NULL)
	{
		MessageBox(NULL, e_msg, title, MB_OK | MB_ICONERROR);
	}
	else
	{
		sprintf(buffer, "wiimote-api:\nError Code: %d\nCheck http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes.asp", GetLastError());
		MessageBox(NULL, buffer, title, MB_OK | MB_ICONERROR);
	}
}


/* writes at most 16 bytes to a given location */
void WriteData(DWORD location, BYTE* array, BYTE numbytes)
{
	int index;
	BYTE report[22];
	if(numbytes > 16)
		return;
	memset(report, 0, 22);
	report[0] = 0x16;
	report[1] = (BYTE) (location >> 24 & 0xFF);
	report[2] = (BYTE) (location >> 16 & 0xFF);
	report[3] = (BYTE) (location >> 8  & 0xFF);
	report[4] = (BYTE) (location & 0xFF);
	report[5] = numbytes;
	memcpy(report + 6, array, numbytes);
	if(HidD_SetOutputReport(wiimotehandle, report, 22) == 0)
		WShowError("While writing data");
	printf("Data written to 0x%2x: ", location);
	for(index = 0; index < 22; index++)
	{
		printf("%2X ", report[index]);
	}
	printf("\n");
	/* sleep after writing data */
	Sleep(300);
}

/*
sample report...
 0  1  2  3  4  5  6  7  8  9 10 11 
30  0  0 88 78 9b  4 e6 11 8f e0 11 ff ff ff ff ff ff ff 7f d8 d6 
*/
void ParseReport(unsigned char* report)
{
	INPUT mouse_input[1];
	INPUT remotebuttons[11];
	MOUSEINPUT* mouse;
	KEYBDINPUT* keybd;
	static rbutton = 0;
	static lbutton = 0;
	unsigned char x_mult = 0;
	unsigned char y_mult = 0;
	int index;
	UINT num_buttons = 0;
	LPBYTE status;
	BYTE button_mask;
	int byte;
	/* status information */
	if(report[0] == 0x20)
		return;
	/* read memory data */
	if(report[0] == 0x21)
		return;
	/* write memory data status */
	if(report[0] == 0x22)
		return;
	/* extension data only */
	if(report[0] == 0x3d)
		return;
	 /* IR/Motion/Button report, plus in range */
	if(report[0] == 0x33 && report[8] != 0xFF)
	{
		mouse_input->type = INPUT_MOUSE;
		mouse = &(mouse_input->mi);
		mouse->dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		/* use one dot */
		x_mult = (report[8] & 0x30) >> 4;
		y_mult = (report[8] & 0xC0) >> 6;
		mouse->dx = (0xFF*4 - ((LONG) report[6] + x_mult*0xFF)) 
			* 65535 / (0xFF*4);
		mouse->dy = ((LONG) report[7] + y_mult*0xFF) * 65535 / (0xFF*3);
		mouse->time = 0;
		/* what an odd thing apps have to do */
		mouse->dwExtraInfo = GetMessageExtraInfo();
		/* parse A & B as Right and Left buttons */
		if(report[2] & 0x04)
		{
			if(lbutton == 0)
				mouse->dwFlags |= MOUSEEVENTF_LEFTDOWN;
			lbutton = 1;
		}
		else if(lbutton == 1)
		{
			mouse->dwFlags |= MOUSEEVENTF_LEFTUP;	
			lbutton = 0;
		}
		if(report[2] & 0x08)
		{
			if(rbutton == 0)
				mouse->dwFlags |= MOUSEEVENTF_RIGHTDOWN;
			rbutton = 1;
		}
		else if(rbutton == 1)
		{
			mouse->dwFlags |= MOUSEEVENTF_RIGHTUP;	
			rbutton = 0;
		}
		if(SendInput(1, mouse_input, sizeof(INPUT)) == 0 )
			WShowError("Pointer to Mouse");

	}
	/* parse buttons */
	/* a button mapped to 0 means no mapping */
	/* TODO */
	for(index = 0; index < 11; index++)
	{
		remotebuttons[num_buttons].type = INPUT_KEYBOARD;
		keybd = &(remotebuttons[num_buttons].ki);
		switch(index)
		{
			/* case numbering is irrelevant, just unique */
			case 0: /* up */
				button_mask = 0x08;
				byte = 1;
				keybd->wVk = (WORD) wiimote_keys.up;
				status = &(wiimote_status.up);
				break;
			case 1: /* left */
				button_mask = 0x01;
				byte = 1;
				keybd->wVk = (WORD) wiimote_keys.left;
				status = &(wiimote_status.left);
				break;
			case 2: /* right */
				button_mask = 0x02;
				byte = 1;
				keybd->wVk = (WORD) wiimote_keys.right;
				status = &(wiimote_status.right);
				break;
			case 3: /* down */
				button_mask = 0x04;
				byte = 1;
				keybd->wVk = (WORD) wiimote_keys.down;
				status = &(wiimote_status.down);
				break;
			case 4: /* a */
				button_mask = 0x08;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.a;
				status = &(wiimote_status.a);
				break;
			case 5: /* b */
				button_mask = 0x04;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.b;
				status = &(wiimote_status.b);
				break;
			case 6: /* minus */
				button_mask = 0x10;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.minus;
				status = &(wiimote_status.minus);
				break;
			case 7: /* home */
				button_mask = 0x80;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.home;
				status = &(wiimote_status.home);
				break;
			case 8: /* plus */
				button_mask = 0x10;
				byte = 1;
				keybd->wVk = (WORD) wiimote_keys.plus;
				status = &(wiimote_status.plus);
				break;
			case 9: /* one */
				button_mask = 0x02;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.one;
				status = &(wiimote_status.one);
				break;
			case 10: /* two */
				button_mask = 0x01;
				byte = 2;
				keybd->wVk = (WORD) wiimote_keys.two;
				status = &(wiimote_status.two);
				break;
			default: /* shouldn't happen, but just in case */
				keybd->wVk = 0;
		}
		if(keybd->wVk == 0) /* no key mapping */
			continue;
		keybd->time = 0;
		keybd->wScan = 0;
		if(report[byte] & button_mask) /* key down */
		{
			*status = 1;
			keybd->dwFlags = 0;
		}
		else /* key up */
		{
			if(*status == 0) /* key already up */
				continue;
			*status = 0;
			keybd->dwFlags = KEYEVENTF_KEYUP;
		}
		keybd->dwExtraInfo = GetMessageExtraInfo();
		num_buttons++;
	}
	if(SendInput(num_buttons, remotebuttons, sizeof(INPUT)) == 0 )
		WShowError("Wii Remote Buttons");
}

/*
 * Connects to a Wiimote given the Device Path
 * returns 0 if no wiimote found.
 */
int WiiM_ConnectWiimote(LPBYTE devicepath)
{
	/* handle for just reading, currently */
	wiimotehandle = CreateFile(devicepath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if(wiimotehandle == INVALID_HANDLE_VALUE)
	{
		WShowError("While Connecting to Wiimote");
	}
	memset(&wiimote_keys, 0, sizeof(WIIMOTE_MAP));
	memset(&wiimote_status, 0, sizeof(WIIMOTE_MAP));
	return 1;
}

void WiiM_CloseConn()
{
	CloseHandle(wiimotehandle);
}

/* sets a new key map for the wiimote, if NULL is passed, 
 * previous map is cleared */
void WiiM_SetWiimoteKeyMap(WIIMOTE_MAP* keys)
{
	if(keys == NULL)
		memset(&wiimote_keys, 0, sizeof(WIIMOTE_MAP));
	else
		memcpy(&wiimote_keys, keys, sizeof(WIIMOTE_MAP));
}

WIIMOTE_MAP WiiM_GetWiimoteKeyStatus()
{
	return wiimote_status;
}

void WiiM_TogRumble()
{
	BYTE report[2];
	rumble = !rumble;
	report[0] = 0x13;
	report[1] = rumble;
	if(HidD_SetOutputReport(wiimotehandle, report, 2) == 0)
		WShowError("While setting output report.");
	/* id and first byte */
	printf("Sent: %x %x\n", report[0], report[1]); 
}

/* from 0 to 3 */
void WiiM_TogLED(int index)
{
	BYTE report[2];
	led[index] = !led[index];
	report[0] = 0x11;
	report[1] = led[0] | led[1]<<1 | led[2]<<2 | led[3]<<3;
	report[1] = report[1]<<4 | rumble;
	if(HidD_SetOutputReport(wiimotehandle, report, 2) == 0)
		WShowError("While setting output report.");
	/* id and first byte */
	printf("Sent: %x %x\n", report[0], report[1]); 
}


void WiiM_TogIR_Abs()
{
	BYTE report[3];
	BYTE data[2];

	/* enabled using marcan's info */
	ir = !ir;
	if(ir) {
		/* set mode to motion + (basic) IR */
		report[0] = 0x12;
		report[1] = 0x00;
		report[2] = 0x33;
		if(HidD_SetOutputReport(wiimotehandle, report, 3) == 0)
			WShowError("While setting output report.");
		printf("Sent: %x %x\n", report[0], report[1]);
		/* enable IR (2 parts) */
		report[0] = 0x13;
		report[1] = 0x04;
		if(HidD_SetOutputReport(wiimotehandle, report, 2) == 0)
			WShowError("While setting output report.");
		printf("Sent: %x %x\n", report[0], report[1]); 
		report[0] = 0x1A;
		report[1] = 0x04;
		if(HidD_SetOutputReport(wiimotehandle, report, 2) == 0)
			WShowError("While setting output report.");
		printf("Sent: %x %x\n", report[0], report[1]); 
		/* marcan's stuff */
		data[0] = 0x08;
		WriteData(0x04B00030, data, 1);
		data[0] = 0x90;
		WriteData(0x04B00006, data, 1);
		data[0] = 0xC0;
		WriteData(0x04B00008, data, 1);
		data[0] = 0x40;
		WriteData(0x04B0001A, data, 1);
		data[0] = 0x33;
		WriteData(0x04B00033, data, 1);
	}
	else
	{
		/* set mode to buttons only */
		report[0] = 0x12;
		report[1] = 0x00;
		report[2] = 0x30;
		if(HidD_SetOutputReport(wiimotehandle, report, 3)==0)
			WShowError("While setting output report.");
		printf("Sent: %x %x\n", report[0], report[1]);
	}
}

/* buffer needs to hold at least 22 characters */
int WiiM_GetReport(LPBYTE buffer)
{
	return LoopAction(buffer, FALSE);
}

int WiiM_ProcessEvent()
{
	return LoopAction(NULL, TRUE);
}

/* buffer needs to hold at least 22 characters */
int WiiM_ProcessAndGetReport(LPBYTE buffer)
{
	return LoopAction(buffer, TRUE);	
}

int LoopAction(LPBYTE buffer, BOOL process)
{
	OVERLAPPED hidoverlap;
	DWORD result;
	PDWORD size = malloc(sizeof(DWORD));
	BOOL nobuffer = FALSE;

	/* event that can be signaled */
	hidoverlap.hEvent = CreateEvent(NULL, TRUE, TRUE, ""); 
	hidoverlap.Offset = 0;
	hidoverlap.OffsetHigh = 0;

	if(buffer == NULL)
	{
		buffer = malloc(22 * sizeof(BYTE));
		nobuffer = TRUE;
	}

	if(!ReadFile(wiimotehandle, buffer, 22, size, &hidoverlap))
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			result = WaitForSingleObject(hidoverlap.hEvent, 100);
			if(result == WAIT_TIMEOUT)
			{
				CancelIo(wiimotehandle);
				return 0;
			}
			else if(result == WAIT_FAILED)
			{
				WShowError("Read Wait Error");
				return 0;
			}
			GetOverlappedResult(wiimotehandle, &hidoverlap, 
					size, FALSE);
		}
		else
		{
			WShowError("Read Error\n");
		}
	}
	ResetEvent(hidoverlap.hEvent);
	if(process)
		ParseReport(buffer);
	if(nobuffer)
		free(buffer);

	free(size);
	return 1;
}

