void _ClaimDevice(int desiredDeviceNo, bool nice, _DeviceProps &props) {

    props.deviceNo        = 0;
    props.blockSizeAlign  = 1;
    props.blockYSizeAlign = 1;
    props.maxTexYSize     = 0xFFFFFFFF;
    props.maxTexXSize     = 0xFFFFFFFF;

}

/**********************************************************************************************************************/

void _ReleaseDevice() {

}

/**********************************************************************************************************************/

void _ClearTex(_TexPtrA &ta, _TexPtrB &tb) {

    ta.linear = false;
    ta.buf    = NULL;
    tb.buf    = NULL;

}

/**********************************************************************************************************************/

void _AllocTexLinear(_TexPtrA &ta, _TexPtrB &tb, const float *sBuf, unsigned int ySize, unsigned int xCount,
    const char *desc) {

    _ClearTex(ta, tb);

    if ((ySize == 0) || (xCount == 0)) return;

    ta.linear = true;

    tb.buf = sBuf;
    tb.h   = ySize;

}

/**********************************************************************************************************************/

void _AllocTexArray(_TexPtrA &ta, _TexPtrB &tb, const mxArray *array, unsigned int ySize, unsigned int xCount,
    const char *desc) {

    _ClearTex(ta, tb);

    if ((ySize == 0) || (xCount == 0)) return;

    ta.buf = (float *)mxMalloc(xCount * ySize * sizeof(float));
    if (ta.buf == NULL) {
        _Exit("unable to allocate %s buffer", desc);
    }
    mexMakeMemoryPersistent(ta.buf);

    if (array != NULL) {
        memcpy(ta.buf, (float *)mxGetData(array), xCount * ySize * sizeof(float));
    }

    tb.buf = ta.buf;
    tb.h   = ySize;

}

/**********************************************************************************************************************/

void _DeallocTex(_TexPtrA &ta, _TexPtrB &tb) {

    if (!ta.linear) {
        mxFree(ta.buf);
    }

    _ClearTex(ta, tb);

}

/**********************************************************************************************************************/

void _PublishTex(_TexPtrA &ta, _TexPtrB &tb, const float *sBuf, unsigned int ySize, unsigned int xCount,
    const char *desc) {

    if ((ySize == 0) || (xCount == 0)) return;

    if (!ta.linear) {
        memcpy(ta.buf, sBuf, xCount * ySize * sizeof(float));
    }

}

/**********************************************************************************************************************/

template<class T> void _ClearBuf(T *&buf) {

    _ClearHostBuf(buf);

}

/**********************************************************************************************************************/

template<class T> void _AllocBufA(T *&buf, const mxArray *array, unsigned int numComponents, const char *desc) {

    _AllocHostBufA(buf, array, numComponents, desc);

}

/**********************************************************************************************************************/

template<class T> void _AllocBufB(T *&buf, const T *sBuf, unsigned int numElements, const char *desc) {

    _AllocHostBufB(buf, sBuf, numElements, desc);

}

/**********************************************************************************************************************/

template<class T> void _DeallocBuf(T *&buf) {

    _DeallocHostBuf(buf);

}

/**********************************************************************************************************************/

template<class T> void _CopyBuf1D(T *dBuf, char dest, const T *buf, unsigned int count, const char *desc) {

    if (count == 0) return;

    memcpy(dBuf, buf, count * sizeof(T));

}

/**********************************************************************************************************************/

template<class T> void _CopyBuf2D(T *dBuf, char dest, const T *buf, unsigned int height, unsigned int yCount,
    unsigned int xCount, const char *desc) {

    if ((yCount == 0) || (xCount == 0)) return;

    for (unsigned int i = 0; i < xCount; i++, dBuf += yCount, buf += height) {
        memcpy(dBuf, buf, yCount * sizeof(T));
    }

}

/**********************************************************************************************************************/

template<class T> void _CopyBuf3D(T *dBuf, char dest, const T *buf, unsigned int height, unsigned int width,
    unsigned int yCount, unsigned int xCount, unsigned int zCount, const char *desc) {

    if ((yCount == 0) || (xCount == 0) || (zCount == 0)) return;

    for (unsigned int j = 0; j < zCount; j++, buf += height * (width - xCount)) {
        for (unsigned int i = 0; i < xCount; i++, dBuf += yCount, buf += height) {
            memcpy(dBuf, buf, yCount * sizeof(T));
        }
    }

}

/**********************************************************************************************************************/

template<class T> void _UpdateBuf1D(T *buf, const T *sBuf, char src, unsigned int count, const char *desc) {

    if (count == 0) return;

    memcpy(buf, sBuf, count * sizeof(T));

}

/**********************************************************************************************************************/

template<class T> void _UpdateBuf2D(T *buf, unsigned int height, const T *sBuf, char src, unsigned int yCount,
    unsigned int xCount, const char *desc) {

    if ((yCount == 0) || (xCount == 0)) return;

    for (unsigned int i = 0; i < xCount; i++, buf += height, sBuf += yCount) {
        memcpy(buf, sBuf, yCount * sizeof(T));
    }

}

/**********************************************************************************************************************/

template<class T> void _UpdateBuf3D(T *buf, unsigned int height, unsigned int width, const T *sBuf, char src,
    unsigned int yCount, unsigned int xCount, unsigned int zCount, const char *desc) {

    if ((yCount == 0) || (xCount == 0) || (zCount == 0)) return;

    for (unsigned int j = 0; j < zCount; j++, buf += height * (width - xCount)) {
        for (unsigned int i = 0; i < xCount; i++, buf += height, sBuf += yCount) {
            memcpy(buf, sBuf, yCount * sizeof(T));
        }
    }

}

/**********************************************************************************************************************/

template<class T> void _ClearConst(const char *constName, T *cPtr) {

}

/**********************************************************************************************************************/

template<class T> void _AllocConst(const char *constName, T *cPtr, const T *sPtr, unsigned int numElements) {

    if (numElements == 0) return;

    memcpy(cPtr, sPtr, numElements * sizeof(T));

}

/**********************************************************************************************************************/

template<class T> void _DeallocConst(const char *constName, T *cPtr) {

}

/**********************************************************************************************************************/

template<class T> void _CopyConst(T *dPtr, const char *constName, const T *cPtr, unsigned int off,
    unsigned int count) {

    if (count == 0) return;

    memcpy(dPtr, cPtr + off, count * sizeof(T));

}

/**********************************************************************************************************************/

template<class T> void _UpdateConst(const char *constName, T *cPtr, unsigned int off, const T *sPtr,
    unsigned int count) {

    if (count == 0) return;

    memcpy(cPtr + off, sPtr, count * sizeof(T));

}
