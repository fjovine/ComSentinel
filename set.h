#ifndef SET_H
#define SET_H
#include <windows.h>
#include <stdio.h>
/**
 * Set of integers implemented as a bitmap.
 */

 typedef struct tagComSet
{
	DWORD comSet[8];
} COMSET, * PCOMSET;
void setInit(PCOMSET pcomset);
int compareBitmaps(DWORD dest, DWORD source);
int setCompareAndGetLeastChangedIfAny(PCOMSET dest, PCOMSET source);
BOOL setAdd(PCOMSET pcomset, int value);
void setWrite(PCOMSET pcomset, int nBits, FILE * f);

#endif
