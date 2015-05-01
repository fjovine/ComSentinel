#include "comsentinel.h"
//#define DEBUG_WINTRAY

/**
 * Initializes icon data structure.
 * @param info string to be shown as info field in baloon, if presetn.
 * @param title string to be used as title field in the baloon, if present
 * @param dwNotificationMessage message to be sent to Shell_NotifyIcon
 */
BOOL notifyShell(char * info, char * title, DWORD dwNotificationMessage)
{
	static NOTIFYICONDATA nid;
	HICON hicon = LoadIcon(appinfo.hMainInstance, MAKEINTRESOURCE(APPICON));

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = appinfo.hMainWnd;
	nid.uID = IDC_TRAYICON;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_INFO | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYNOTIFY;
	nid.hIcon = hicon;
	strcpy(nid.szTip, appinfo.szAppName);
	strcpy(nid.szInfo, info == NULL ? "" : info);
	strcpy(nid.szInfoTitle, title==NULL ? "": title);
	nid.dwInfoFlags = NIIF_INFO | NIIF_LARGE_ICON;
#ifdef DEBUG_WINTRAY
	log("notifyShell %s %s uFlags %08x dfInfoFlags %08x", 
		nid.szInfo, nid.szInfoTitle,
		nid.uFlags,
		nid.dwInfoFlags
	);
#endif	
	BOOL result = Shell_NotifyIcon(dwNotificationMessage, &nid);
	if (hicon)
	{
		DestroyIcon(hicon);
	}
	return result;
}

/**
 * Adds the process to the taskbar notification area.
 * @param hWnd handle of the window message loop.
 * @param hInst instance handle of the process.
 * @returns true if succeed.
 */
BOOL addTaskBarIcon()
{
	BOOL result = notifyShell(NULL, NULL, NIM_ADD);

#ifdef DEBUG_WINTRAY
	log("addTaskBarIcon result: %d", result);
#endif
	return result;
}

/**
 * Deletes the icon from the tray notification area.
 */
BOOL deleteTaskBarIcon()
{
	BOOL result;
	NOTIFYICONDATA tnid;

	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = appinfo.hMainWnd;
	tnid.uID = IDC_TRAYICON;

	result = Shell_NotifyIcon(NIM_DELETE, &tnid);
#ifdef DEBUG_WINTRAY
	log("deleteTaskBarIcon result:%d", result);
#endif
	return result;
}

/**
 * Shows a balloon tooltim on the tasbar icon.
 * @param into string to be shown as message.
 * @param infoTitle string to be shown as title.
 */
BOOL showTaskbarBalloon(char * info, char * infoTitle)
{
	BOOL result = notifyShell(info, infoTitle, NIM_MODIFY);

#ifdef DEBUG_WINTRAY
	log("showTaskbarBalloon result:%d", result);
#endif
	return result;
}
