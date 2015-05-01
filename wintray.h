#ifndef WINTRAY_H
#define WINTRAY_H

#include <windows.h>

#ifndef IDC_TRAYICON
#define IDC_TRAYICON 1000
#endif
#ifndef WM_TRAYNOTIFY
#define WM_TRAYNOTIFY (WM_USER+100)
#endif

BOOL addTaskBarIcon(void);
BOOL deleteTaskBarIcon(void);
BOOL showTaskbarBalloon(char * info, char * infoTitle);
#endif

