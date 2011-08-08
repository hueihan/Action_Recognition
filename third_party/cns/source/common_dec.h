#include "mex.h"
#include <stdarg.h>
#include <string.h>
#include <float.h>

/**********************************************************************************************************************/

#if (SHRT_MAX != 32767) || (INT_MAX != 2147483647)
    #error unsupported size for short or int
#endif

/**********************************************************************************************************************/

#define CNS_INTMIN (-16777215.0f)
#define CNS_INTMAX (16777215.0f)
#define CNS_FLTMIN (-FLT_MAX)
#define CNS_FLTMAX (FLT_MAX)

// These are fairly arbitrary limits governing the size of statically allocated arrays in host memory.  There would not
// be much penalty for increasing them, and they could be removed altogether if we were willing to make the code a bit
// more complicated (i.e. use dynamic allocation and deallocation).

const unsigned int _MAX_LAYERS = 100;
const unsigned int _MAX_DIMS   = 10;
const unsigned int _ERRMSG_LEN = 512;

/**********************************************************************************************************************/

static void _Info(const char *format, ...);

/**********************************************************************************************************************/

static unsigned int _GetDimSize(const mxArray *array, unsigned int d1, unsigned int nDims = 1);

/**********************************************************************************************************************/

const mxArray *_g_CB;

static unsigned int _CBYX2E(unsigned int z, unsigned int y, unsigned int x);

/**********************************************************************************************************************/

void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *ptr);
void operator delete[](void *ptr);
