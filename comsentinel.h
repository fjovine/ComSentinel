#ifndef COMSENTINEL_H
#define COMSENTINEL_H

//#define UNITTEST
//#define DEBUG

#include <commctrl.h>
#include <windows.h>
#include <shellapi.h>
#include <setupapi.h>
#include <stdio.h>
#include "set.h"
#include "wintray.h"

/**
 * General information about the app.
 */
typedef struct tagCOMSENTINELINFO
{
	HWND hMainWnd;
	HINSTANCE hMainInstance;
	char * szAppName;
} COMSENTINELINFO;

extern COMSENTINELINFO appinfo;

#define APPICON 101
#define BIGICON 102

/**
 * Definition of needed indentifier if they are not in the windows headers.
 */

#ifndef TTM_UPDATE
#define TTM_UPDATE (WM_USER+29)
#endif

#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP (0x80)
#endif

#ifndef NIIF_LARGE_ICON
#define NIIF_LARGE_ICON (0x20)
#endif

#ifndef NIIF_USER
#define NIIF_USER (0x4)
#endif

#ifdef DEBUG
extern FILE *fLog;
#endif

#endif
