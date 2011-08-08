template<class T> void _ClearHostBuf(T *&buf) {

    buf = NULL;

}

/**********************************************************************************************************************/

template<class T> void _AllocHostBufA(T *&buf, const mxArray *array, unsigned int numComponents, const char *desc) {

    _AllocHostBufB(buf, (T *)mxGetData(array), mxGetNumberOfElements(array) / numComponents, desc);

}

/**********************************************************************************************************************/

template<class T> void _AllocHostBufB(T *&buf, const T *sBuf, unsigned int numElements, const char *desc) {

    if (numElements == 0) {
        buf = NULL;
        return;
    }

    buf = (T *)mxMalloc(numElements * sizeof(T));
    if (buf == NULL) {
        _Exit("unable to allocate %s buffer", desc);
    }
    mexMakeMemoryPersistent(buf);

    if (sBuf != NULL) {
        memcpy(buf, sBuf, numElements * sizeof(T));
    }

}

/**********************************************************************************************************************/

template<class T> void _DeallocHostBuf(T *&buf) {

    if (buf != NULL) {
        mxFree(buf);
        buf = NULL;
    }

}

/**********************************************************************************************************************/

#ifdef _GPU
    #include "util_def_cuda.h"
#else
    #include "util_def_cpu.h"
#endif
