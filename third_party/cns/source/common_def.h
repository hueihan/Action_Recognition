void _Info(const char *format, ...) {

    // Keep this function for printing debugging output.

    char msg[_ERRMSG_LEN];

    va_list argList;
    va_start(argList, format);
    vsprintf(msg, format, argList);
    va_end(argList);

    mexPrintf("%s\n", msg);
    mexEvalString("drawnow");

}

/**********************************************************************************************************************/

unsigned int _GetDimSize(const mxArray *array, unsigned int d1, unsigned int nDims) {

    unsigned int d2 = d1 + nDims - 1;

    unsigned int size = 1;

    for (unsigned int d = d1; (d <= d2) && (d < (unsigned int)mxGetNumberOfDimensions(array)); d++) {
        size *= mxGetDimensions(array)[d];
    }

    return size;

}

/**********************************************************************************************************************/

unsigned int _CBYX2E(unsigned int z, unsigned int y, unsigned int x) {

    mxArray *rhs[5];
    mxArray *lhs[1];

    rhs[0] = (mxArray *)_g_CB;
    rhs[1] = mxCreateString("CBYX2E");
    rhs[2] = mxCreateDoubleScalar(z + 1);
    rhs[3] = mxCreateDoubleScalar(y + 1);
    rhs[4] = mxCreateDoubleScalar(x + 1);

    mexCallMATLAB(1, lhs, 5, rhs, "feval");

    unsigned int i = (unsigned int)mxGetScalar(lhs[0]) - 1;

    mxDestroyArray(rhs[1]);
    mxDestroyArray(rhs[2]);
    mxDestroyArray(rhs[3]);
    mxDestroyArray(rhs[4]);
    mxDestroyArray(lhs[0]);

    return i;

}

/**********************************************************************************************************************/

void *operator new(size_t size) {

    void *ptr = mxMalloc(size);

    mexMakeMemoryPersistent(ptr);

    return ptr;

}

/**********************************************************************************************************************/

void *operator new[](size_t size) {

    void *ptr = mxMalloc(size);

    mexMakeMemoryPersistent(ptr);

    return ptr;

}

/**********************************************************************************************************************/

void operator delete(void *ptr) {

    mxFree(ptr);

}

/**********************************************************************************************************************/

void operator delete[](void *ptr) {

    mxFree(ptr);

}
