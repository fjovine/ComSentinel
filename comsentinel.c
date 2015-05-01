/**
 * The algorithm checks new events dealing with COM ports: plug, unplug, conflicts.
 * To describe the set of com ports available a bitmap is used: the maximum
 * number of COM ports is 255 so 8 DWORDS will be used.
 * The application receives the WM_DEVICECHANGE message and updates its internal
 * data structures accoordintly, triggering notifications when one of the following events
 * happens;
 * 1. New COM is recognized as an USB/Serial interface is plugged
 * 2. A COM disappears as the interface is unplugged
 * 3. Two conflicting ports are plugged, having the same COM number.
 */
#include "comsentinel.h"


/** Container for global WinApp related info */
COMSENTINELINFO appinfo;

/** General purpose string buffer. */
char szBuffer[256];

/** Current set (in the sense of mathematics) containing the recognized com ports. */
COMSET comStatus;

/** New set (in the sense of mathematics) containing the recognized com ports. */
COMSET newComStatus;

/** Variable for the windows-managed critical section struct. */
CRITICAL_SECTION critical_section =  {0};

/** Handle of the function to modify the behaviour of the standard message box. */
HHOOK hMsgBoxHook;


#ifdef DEBUG
FILE * fLog = NULL;
char * logFilename = "Log.txt";

/**
 * Macro to store a log message in the Log.txt file.
 *
 * Must be used as a printf message, e.g. **LOG("Message n.%d [%s]", msgNo, msg);**
 */
#define LOG(...) if (openLog() != NULL) { fprintf(fLog, __VA_ARGS__); fprintf(fLog, "\n"); fclose(fLog); }

/**
 * Macro to show the content of a COMSET.
 * @param caption Human readable description of the log message.
 * @param set COMSET to be shown. Only the first 16 bits are shown.
 */
#define LOGSET(caption, set) logSet(caption, set)

/**
 * Opens the log file and writes the time.
 *
 * The first time it is called after launch, a new log file is created.
 * The following times it is opened in append mode.
 */
FILE * openLog(void)
{
	SYSTEMTIME stime;

	if (fLog == NULL)
	{
		// The first time deletes the file
		fLog = fopen(logFilename, "w");
	}
	else
	{
		// from the second onwards time appends the message to it
		fLog = fopen(logFilename, "a");
	}
	if (fLog != NULL)
	{
		GetLocalTime(&stime);
		fprintf(fLog, "%02d:%02d:%02d.%03d ",
			stime.wHour, stime.wMinute, stime.wSecond,
			stime.wMilliseconds
		);
	}
	return fLog;
}

/**
 * Log the contents of an integer set.
 *
 * @param szCaption Human readable descriptive text.
 * @param pcomset Pointer to a COMSET, set of integers implemented as bitmap.
 */
void logSet(char * szCaption, PCOMSET pcomset)
{
	if (openLog() != NULL)
	{
		fprintf(fLog, "set [%s] ", szCaption);
		setWrite(pcomset, 16, fLog);
	}
	fclose(fLog);
}
#else
#define LOG(...) 
#define LOGSET(caption, set)
#endif

/// Callback to be passed to getRegistrzLocalMachine
typedef void (*KEYVALUECALLBACK)(LONG index, char * key);

/**
 * Selects the path from the selected hive and calls for each direct value the passed callback, if not null.
 *
 * This function was used in the first implementation. Now it is not used anymore as a better
 * strategy accessing the Windows device subsystem has been developed. It is left for possible
 * future use.
 * 
 * @param hive Registy hive to start the path from e.g. **HKEY_LOCAL_MACHINE.**
 * @param path backslash delimited pathname under the passed hive defining the key.
 *        for instance **"HARDWARE\DEVICEMAP\SERIALCOM".**
 * @param callback pointer to function to be called for each key-value pair of the selected path.
 */
void inspectRegistry(HKEY hive, char * path, KEYVALUECALLBACK callback)
{
	HKEY hKey;
	DWORD nValues;

	if (RegOpenKeyEx(hive, path, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryInfoKey(hKey,
		  NULL, NULL, NULL, NULL, NULL, NULL,
		  &nValues,
		  NULL, NULL, NULL, NULL
		) == ERROR_SUCCESS)
		{
			char key[32768];
			DWORD dwKeySize = sizeof(key);
			char value[16384];
			DWORD dwValueSize = sizeof(value);

			int i;
			for (i=0; i< nValues; i++)
			{
				LONG result = RegEnumValue(hKey, i, (char *)&key, &dwKeySize, NULL, NULL, (char*)&value, &dwValueSize);
				LOG("i:%d nValues %d result %d value %s", i, nValues, result, value);
				if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA)
				{
					if (callback != NULL)
					{
						(*callback)(i, value);
					}
				}
			}
		}

	}
}

/** number of conflicting port. If more than two ports are conflicting, only one is shown */
static int conflicting;

/** counter of recongnized ports */
static int portCount;

/** vector containing the recognized ports */
static int portList[256];

/**
 * String containing the list of com ports in human readable format and additional info.
 * As Windows supports up to 256 com ports numbered COM1 ... COM256, as after the string 
 * there is a blank, we need 256 * 7 characters in the worst case per com and additional 30 characters
 * for the additional info.
 */
static char msgBoxString[256 * 7 +30];

/**
 * Callback to be used during the enumeration of the COM ports.
 *
 * If the static variable **conflicting** is zero, the conflicting com port is stored in it.
 * @param i index of the key.
 * @param value string containing the descriptor of the port e.g. "COM3".
 */
void detectComCallback(LONG i, char * value)
{
	if ((strncmp(value, "COM", 3) == 0) && (strlen(value) <= 6))
	{
		// The comstring starts for COM and has less than 6 chars (COM1..COM256)
		int nCom = atoi(&value[3]);
		portList[portCount++] = nCom;
		if (setAdd(&newComStatus, nCom) && conflicting==0)
		{
			conflicting = nCom;
		}
	}
}

/**
 * Compares two integers. Needed by qsort.
 * @param a pointer to the first integer
 * @param b pointer to the second integer
 * @return a value >=< 0 following the ordering of a and b
 */ 
int compint(const void *a, const void *b)
{
	return (*(int*) a - *(int*)b);
}

/**
 * Appends two c-like strings.
 * @param dest string at the end of which the other is appended.
 * @param src string to append.
 * @return pointer to the enf of the destination string.
 */
char * strappend(char * dest, const char * src)
{
	char * result = dest+strlen(dest);
	strcpy(result, src);
	result += strlen(src);
	*result = '\0';
	return result;
}

/**
 * Enumerate the COM ports peeking into the system description of devices.
 * The callback is called for every recognized port.
 * 
 * @param callback Every time a port is recognized, the callback is called (if not null) with
 * two parameters: the sequence number and the correspondent com number.
 */
int enumerateSerialPorts(KEYVALUECALLBACK callback)
{
	HDEVINFO hDevInfo = 0L;
	SP_DEVINFO_DATA spDevInfoData = {0};
	short wIndex = 0;
	int nCom = 0;

	hDevInfo = SetupDiGetClassDevs(0L, 0L, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_PROFILE);
	if (hDevInfo == (void*)-1)
	{
		return 0;
	};

	wIndex = 0;
	spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	while (1)
	{
		if (SetupDiEnumDeviceInfo(hDevInfo, wIndex, &spDevInfoData))
		{
			char szBuf[MAX_PATH] = {0};
			char szID[LINE_LEN] = {0};
			char szPath[MAX_PATH] = {0};
			short wImageIdx = 0;
			short wItem = 0;
			if (!SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&spDevInfoData,
				SPDRP_CLASS, //SPDRP_DEVICEDESC,
				0L,
				(PBYTE)szBuf,
				2048,
				0))
			{
				wIndex++;
				continue;
			};
			if (strcmp(szBuf,"Ports") == 0)
			{
				char szName[64] = {0};
				char szID[LINE_LEN] = {0};
				char szPath[MAX_PATH] = {0};
				DWORD dwRequireSize;
				if (!SetupDiGetClassDescription(
					&spDevInfoData.ClassGuid,
					szBuf,
					MAX_PATH,
					&dwRequireSize))
				{
					wIndex++;
					continue;
				};
				if (!SetupDiGetDeviceInstanceId(hDevInfo, &spDevInfoData, szID, LINE_LEN,0)) 
				{
					continue;
				}
				if (SetupDiGetDeviceRegistryProperty(
					hDevInfo,
					&spDevInfoData,
					SPDRP_FRIENDLYNAME,
					0L,
					(PBYTE)szName,
					63,
					0))
				{
					char szComName[7] = {"COM"};
					char * comInName = strstr(szName, szComName);
					if (comInName != NULL) 
					{
						int i;
						comInName += 3;
						for (i=3; i<6; i++)
						{
							if (isdigit(*comInName))
							{
								szComName[i] = *comInName++;
							}
							else
							{
								szComName[i] = '\0';
							}
						}							
						if (callback)
						{
							callback(nCom, szComName);
						}
						nCom++;
					}
				}
			}
		}
		else 
		{
			break;
		}
		wIndex++;
	};
	return 1;
};

/**
 * Updates the set of com ports and fills the global msgBoxString with a human readable
 * list of available ports and conflicts.
 * @return if 0, no ports are conflicting, otherwise the number of the lowest conflicting port.
 *
 *         For instance, if there are 2x COM1's and 2x COM3's, 1 will be returned.
 */
char * createMsgBoxString(void)
{
	int i;
	msgBoxString[0] = '\0';
	char * lastMsg = &msgBoxString[0];
	for (i=0; i<portCount; i++)
	{	
		char szMsg[8];
		sprintf(szMsg, "COM%d ", portList[i]); 
		lastMsg = strappend(lastMsg, szMsg);
	}
	return msgBoxString;
}

/**
 * Updates the port list and conflicting port number if any.
 * The newComStatus set is updated.
 */ 
int updateComSet(void)
{
	conflicting = 0;
	portCount = 0;
	setInit(&newComStatus);
	#if 0
	inspectRegistry(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", detectComCallback);
	#else
	enumerateSerialPorts(detectComCallback);
	#endif
	qsort(portList, portCount, sizeof(int), compint);
	createMsgBoxString();
	LOG("---- %s",msgBoxString);
	return conflicting;
}

/**
 * Callback function managing the hooks to modify the behaviour of the MessageBox.
 * @param nCode code function when active.
 * @param wParam of the message to be processed.
 * @param lParam of the message to be processed.
 * 
 */
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	if (nCode <0)
	{
		CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
	}
	if (nCode == HCBT_ACTIVATE)
	{
		hwnd = (HWND) wParam;
		HICON hicon = LoadIcon(appinfo.hMainInstance, MAKEINTRESOURCE(BIGICON));

		if (GetDlgItem(hwnd, IDOK) != NULL)
		{
			SetDlgItemText(hwnd, IDOK, "Minimize");
		}
		if (GetDlgItem(hwnd, IDCANCEL) != NULL)
		{
			SetDlgItemText(hwnd, IDCANCEL, "Exit");
		}
		
		HWND wi = GetDlgItem(hwnd, 20);
		SendMessage(wi, STM_SETICON, (WPARAM)hicon, 0);
		if (hicon)
		{
			DestroyIcon(hicon);
		}
		return 0;
	}
	return CallNextHookEx(hMsgBoxHook, nCode, wParam, lParam);
}

/**
 * Builds and shows the MessageBox containing
 * - The list of COM ports found
 * - If there is a conflict
 * - The Minimize button to start monitoring
 * - The Exit button to exit
 * - The ? (Help button) to open the system dialog for device maintenence
 * @param hwnd handle to the main window function needed to manage the help functionality.
 */
void actMessageBox(HWND hwnd)
{
	if (conflicting)
	{
		strappend(msgBoxString,"\nConflicting ports");
	}
	if (portCount == 0)
	{
		strcpy(szBuffer,"No available COM ports");
	}
	else {
		sprintf(szBuffer,  "%s\n%s", "Available COM ports:", msgBoxString);
	}
	hMsgBoxHook = SetWindowsHookEx(
		WH_CBT,
		CBTProc,
		NULL,
		GetCurrentThreadId()
	);
	int rVal = MessageBox (hwnd, szBuffer, "ComSentinel", MB_ICONEXCLAMATION | MB_OKCANCEL | MB_HELP | MB_SYSTEMMODAL);
	UnhookWindowsHookEx(hMsgBoxHook);
	if (rVal == IDCANCEL)
	{
		PostMessage(hwnd, WM_DESTROY, 0, 0L);
	}
}

#ifndef UNITTEST
////
#pragma argsused
/**
 * Message loop of the application.
 *
 * It catches WM_DEVICECHANGE to verify what device has been plugged or unplugged.
 * It catches WM_HELP launched when the ? button is pressed on the main dialog. 
 */
LRESULT FAR PASCAL WndProc (HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
	static BOOL isMessageBox = FALSE;
	int nConflict;
	int nCom;

	switch (message) {

	case WM_DEVICECHANGE:
		EnterCriticalSection(&critical_section);
		nConflict = updateComSet();
		nCom = setCompareAndGetLeastChangedIfAny(&comStatus, &newComStatus);
		if ((nConflict != 0) || (nCom != 0))
		{
			if (nConflict != 0)
			{
				sprintf(szBuffer, "Conflicting COM%d", nConflict);
			}
			else
			{
				// If something has changed, open the taskbar balloon and notify it.
				if (nCom < 0)
				{
					sprintf(szBuffer, "Unplugged COM%d", -nCom);
				}
				else
				{
					sprintf(szBuffer, "Plugged COM%d", nCom);
				}
			}
			LOG(szBuffer);
			showTaskbarBalloon(szBuffer, "Comsentinel");
		}
		LeaveCriticalSection(&critical_section);
		return 0;
		
	case WM_HELP:
		LOG("Launch device manager");
		ShellExecute(NULL, "open", "cmd.exe", "/c start control /name Microsoft.DeviceManager", NULL, SW_HIDE);
		LOG("WinHelpError %d\n", GetLastError());		
		return 0;
	case WM_TRAYNOTIFY:
		if (wParam == IDC_TRAYICON) {
			if (lParam == WM_LBUTTONDOWN) {
				if (isMessageBox) {
					break;
				}
				isMessageBox = TRUE;
				actMessageBox(hwnd);
				isMessageBox = FALSE;
			}
		}
		break;

	case WM_DESTROY:
		DeleteCriticalSection(&critical_section);
		deleteTaskBarIcon();
		PostQuitMessage	(0);
		return  0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

/**
 * Entry point for the windows function.
 *
 * The application is minimal: it uses only a MessageBox as the main user interface
 * and saves an icon on the taskbar to notify events dealing with the USB/serial COM ports.
 */
int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow )
{
	appinfo.szAppName = "comsentinel";
	HWND hWnd;
	MSG msg;
	WNDCLASS wndclass;

	if (FindWindow(appinfo.szAppName, appinfo.szAppName))
	{
		// If there is another window of the same class, exit
		return(1);
	}

	LOG("%s starts", appinfo.szAppName);
	if (!hPrevInstance) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = (WNDPROC) WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = appinfo.szAppName;
		RegisterClass (&wndclass);
	}

	hWnd = CreateWindow (
		appinfo.szAppName, appinfo.szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,
		CW_USEDEFAULT,CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);

	appinfo.hMainWnd = hWnd;
	appinfo.hMainInstance = hInstance;


	// The initial value set of recognized com ports is loaded.
	setInit(&comStatus);
	updateComSet();
	
	// This call is only to copy the new status on the old as initial state.
	setCompareAndGetLeastChangedIfAny(&comStatus, &newComStatus);

	LOGSET("comstatus", &comStatus);
	LOGSET("newcomstatus", &newComStatus);
	// Adds this application in the taskbar notification area.
	addTaskBarIcon();

	InitializeCriticalSection(&critical_section);

	// Open the message box after the harnessing has been finished.
	actMessageBox(hWnd);
	// Message loop.
	while (GetMessage(&msg, NULL, 0,0)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}
#else
/**
 * Unittests for the code. The results are in the Log.txt file in the folder
 * from which the program is launched.
 */
#include <stdio.h>
#include <setupapi.h>
int main ()
{
	
	// In UNITTEST mode, no console is available so all the results will be saved in f

	FILE * f = fopen("Log.txt","w");
	HDEVINFO hdevinfo = SetupDiGetClassDevs(NULL, "COM", NULL, DIGCF_ALLCLASSES);
	DWORD error = GetLastError();
	fprintf(f,"error %08d\n", error);

	COMSET set1, set2;
	setInit(&set1);
	setInit(&set2);

	fprintf(f, "Should be 1 %d \n", compareBitmaps(0xAAAE, 0xAAAF));
	fprintf(f, "Should be -1 %d \n", compareBitmaps(0xAAAF, 0xAAAE));

	fprintf(f, "Should be -32 %d \n", compareBitmaps(0x8AAAAAAA, 0x0AAAAAAA));
	fprintf(f, "Should be 32 %d \n",  compareBitmaps(0x0AAAAAAA, 0x8AAAAAAA));

	// Sets 1,3,5,69 and 250 on set1
	setAdd(&set1, 1);
	setAdd(&set1, 3);
	setAdd(&set1, 5);
	setAdd(&set1, 69);
	setAdd(&set1, 250);
	fprintf(f, "Set1 "); setWrite(&set1, 16, f);

	// Sets 1,3,5, and 250 on set1
	setAdd(&set2, 1);
	setAdd(&set2, 3);
	setAdd(&set2, 5);
	setAdd(&set2, 250);
	fprintf(f, "Set2 "); setWrite(&set2, 16, f);

	// Compares both, should return -69
	int diff1 = setCompareAndGetLeastChangedIfAny(&set1, &set2);

	// A second comparison should return 0
	int diff2 = setCompareAndGetLeastChangedIfAny(&set1, &set2);

	fprintf(f, "Should be -69 : [%d]\n", diff1);
	fprintf(f, "Should be 0   : [%d]\n", diff2);

	for (portCount=0; portCount<10; portCount++)
	{
		portList[portCount] = 2*portCount+1;
	}
	fprintf(f, ">>>%s\n", createMsgBoxString());
	fclose(f);
}
#endif
