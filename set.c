/**
 * Set of integers implemented as a bitmap.
 */
#include "comsentinel.h"
/**
 * Initializes the set as a void set.
 * @parameter  pcomset pointer to set.
 */
void setInit(PCOMSET pcomset)
{
	int i;
	for (i=0; i<8; i++)
	{
		pcomset->comSet[i] = 0;
	}
}

/**
 * Compares the dest and source words.
 * @param dest destination word.
 * @param source source word.
 * @return 0 if the words are equal, +/-n where n is the least significant 
 * different bit between the words.
 * If source[n]==HIGH i.e. the n-th bit of the source set is high, returns +n, else returns -n.
 */
int compareBitmaps(DWORD dest, DWORD source)
{
	if (dest == source)
	{
		return 0;
	}
	int result = 1;
	DWORD dwMask;
	for (dwMask = 1; dwMask != 0x00000000; dwMask <<= 1)
	{
		if ((dest & dwMask) != (source & dwMask))
		{
			if ((dest & dwMask) != 0)
			{
				return -result;
			}
			if ((source & dwMask) != 0)
			{
				return result;
			}
		}

		result ++;
	}
	// it will not hit this point, but let us use defensive programming
	return 0;
}

/**
 * Compares the passed sets then and copies source over dest.
 * @param dest destination set.
 * @param source source set.
 * @returns 0 if the sets are identical, +n if the n-th element has been added, -n if the n-th element as been deleted.
 */
int setCompareAndGetLeastChangedIfAny(PCOMSET dest, PCOMSET source)
{
	int result = 0;
	int i;
	for (i=0; i<8; i++)
	{
		if (result == 0)
		{
			// if still no differences have been found, the comparison
			// must be performed
			result = compareBitmaps(dest->comSet[i], source->comSet[i]);
			if (result > 0)
			{
				result += 32*i;
			}
			else if (result < 0)
			{
				result -= 32*i;
			}
		}
		dest->comSet[i] = source->comSet[i];
	}
	return result;
}

/**
 * Inserts the passed int value in the set.
 * @param pcomset pointer to the set
 * @param value integer to be added to the set. It must be > 0.
 * @return true if the value was already in the set.
 */
BOOL setAdd(PCOMSET pcomset, int value)
{
	BOOL result = FALSE;
	if (value < 1) return FALSE;
	if (value > 256) return FALSE;
	value --; // It is > 0. As the integers start from 1, bit 0 is COM1
	DWORD * pToChange = &(pcomset->comSet[value / 32]);
	DWORD dwMask = 1<< (value % 32);
	result = *pToChange & dwMask;
	*pToChange |= dwMask;
	return result;
}

/**
 * Writes the set in ascii form on file.
 * @param pcomset Pointer to the set to be written.
 * @param nBits number of bites to be shown
 * @param f file where to write the file.
 */
void setWrite(PCOMSET pcomset, int nBits, FILE * f)
{
	int i;
	int nWords = 7;
	if (nBits > 0 && (nBits / 32) < nWords)
	{
		nWords = nBits / 32;
		if (nBits %32 != 0)
		{
			nWords ++;
		}
	}
	for (i=nWords; i>=0; i--)
	{
		fprintf(f, "%08x ", pcomset->comSet[i]);
	}
	fprintf(f,"\n");
}

