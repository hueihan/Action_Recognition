#ifdef _GPU

    typedef struct {
        bool       linear;
        cudaArray *arr;
    } _TexPtrA;

    #define _TEXTUREA _TexPtrA
    #define _TEXTUREB texture<float, 2>

    static void _CudaExit(const char *format, ...);

    cudaError_t _g_cudaErr;

    #define _INLINE __device__

#else

    #include <math.h>

    typedef struct {unsigned short x, y, z, w;} ushort4;

    typedef struct {
        bool   linear;
        float *buf;
    } _TexPtrA;

    typedef struct {
        const float  *buf;
        unsigned int  h;
    } _TexPtrB;

    #define _TEXTUREA _TexPtrA
    #define _TEXTUREB _TexPtrB

    #define _INLINE inline

#endif

/**********************************************************************************************************************/

typedef struct {
    unsigned int deviceNo;
    unsigned int blockSizeAlign;
    unsigned int blockYSizeAlign;
    unsigned int maxTexYSize;
    unsigned int maxTexXSize;
} _DeviceProps;

/**********************************************************************************************************************/

template<class T> void _ClearHostBuf(T *&buf);

template<class T> void _AllocHostBufA(T *&buf, const mxArray *array, unsigned int numComponents, const char *desc);
template<class T> void _AllocHostBufB(T *&buf, const T *sBuf, unsigned int numElements, const char *desc);

template<class T> void _DeallocHostBuf(T *&buf);

/**********************************************************************************************************************/

static void _ClaimDevice(int desiredDeviceNo, bool nice, _DeviceProps &props);

static void _ReleaseDevice();

/**********************************************************************************************************************/

static void _ClearTex(_TEXTUREA &ta, _TEXTUREB &tb);

static void _AllocTexLinear(_TEXTUREA &ta, _TEXTUREB &tb, const float *sBuf, unsigned int ySize, unsigned int xCount,
    const char *desc);

static void _AllocTexArray(_TEXTUREA &ta, _TEXTUREB &tb, const mxArray *array, unsigned int ySize, unsigned int xCount,
    const char *desc);

static void _DeallocTex(_TEXTUREA &ta, _TEXTUREB &tb);

static void _PublishTex(_TEXTUREA &ta, _TEXTUREB &tb, const float *sBuf, unsigned int ySize, unsigned int xCount,
    const char *desc);

/**********************************************************************************************************************/

template<class T> void _ClearBuf(T *&buf);

template<class T> void _AllocBufA(T *&buf, const mxArray *array, unsigned int numComponents, const char *desc);
template<class T> void _AllocBufB(T *&buf, const T *sBuf, unsigned int numElements, const char *desc);

template<class T> void _DeallocBuf(T *&buf);

template<class T> void _CopyBuf1D(T *dBuf, char dest, const T *buf, unsigned int count, const char *desc);

template<class T> void _CopyBuf2D(T *dBuf, char dest, const T *buf, unsigned int height, unsigned int yCount,
    unsigned int xCount, const char *desc);

template<class T> void _CopyBuf3D(T *dBuf, char dest, const T *buf, unsigned int height, unsigned int width,
    unsigned int yCount, unsigned int xCount, unsigned int zCount, const char *desc);

template<class T> void _UpdateBuf1D(T *buf, const T *sBuf, char src, unsigned int count, const char *desc);

template<class T> void _UpdateBuf2D(T *buf, unsigned int height, const T *sBuf, char src, unsigned int yCount,
    unsigned int xCount, const char *desc);

template<class T> void _UpdateBuf3D(T *buf, unsigned int height, unsigned int width, const T *sBuf, char src,
    unsigned int yCount, unsigned int xCount, unsigned int zCount, const char *desc);

/**********************************************************************************************************************/

template<class T> void _ClearConst(const char *constName, T *cPtr);

template<class T> void _AllocConst(const char *constName, T *cPtr, const T *sPtr, unsigned int numElements);

template<class T> void _DeallocConst(const char *constName, T *cPtr);

template<class T> void _CopyConst(T *dPtr, const char *constName, const T *cPtr, unsigned int off,
    unsigned int count);

template<class T> void _UpdateConst(const char *constName, T *cPtr, unsigned int off, const T *sPtr,
    unsigned int count);
