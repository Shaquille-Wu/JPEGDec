#ifndef __KY_MEM_H__
#define __KY_MEM_H__

#define   MEM_ALIGNED                 32

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

	void*  KyAllocMem(size_t uMemSize) ;

	void   KyFreeMem(void* pMem) ;

#ifdef __cplusplus
}
#endif

#endif