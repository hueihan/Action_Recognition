#include "common_dec.h"

#include <math.h>

/**********************************************************************************************************************/

const unsigned int RAND_BATCH_SIZE = 1000000;

char Msg[_ERRMSG_LEN];

#define Exit(...) do { sprintf(Msg, __VA_ARGS__); mexErrMsgTxt(Msg); } while(false)

/**********************************************************************************************************************/

static void MakeSpikes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void FindSpikes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void BinSpikes (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

/*--------------------------------------------------------------------------------------------------------------------*/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    unsigned int mode = (unsigned int)mxGetScalar(prhs[0]);

    switch (mode) {
    case 0: MakeSpikes(nlhs, plhs, nrhs - 1, prhs + 1); break;
    case 1: FindSpikes(nlhs, plhs, nrhs - 1, prhs + 1); break;
    case 2: BinSpikes (nlhs, plhs, nrhs - 1, prhs + 1); break;
    }

}

/**********************************************************************************************************************/

class RandGen {
public:

    void Open();
    void Close();
    double Rand();

private:

    mxArray      *Array;
    double       *Buf;
    unsigned int  Index;

};

/*--------------------------------------------------------------------------------------------------------------------*/

void RandGen::Open() {

    Array = NULL;
    Buf   = NULL;
    Index = RAND_BATCH_SIZE;

}

/*--------------------------------------------------------------------------------------------------------------------*/

void RandGen::Close() {

    if (Array != NULL) mxDestroyArray(Array);

}

/*--------------------------------------------------------------------------------------------------------------------*/

double RandGen::Rand() {

    if (Index >= RAND_BATCH_SIZE) {

        if (Array != NULL) mxDestroyArray(Array);

        mxArray *rhs[2];
        rhs[0] = mxCreateDoubleScalar(RAND_BATCH_SIZE);
        rhs[1] = mxCreateDoubleScalar(1);

        mexCallMATLAB(1, &Array, 2, rhs, "rand");

        mxDestroyArray(rhs[0]);
        mxDestroyArray(rhs[1]);

        Buf   = mxGetPr(Array);
        Index = 0;

    }

    return Buf[Index++];

}

/**********************************************************************************************************************/

template<class T> void ResizeCols(T *&buf, unsigned int &yCount, unsigned int xCount, unsigned int yCountNew, T fill) {

    if (yCount == yCountNew) return;

    T *bufNew = (T *)mxMalloc(yCountNew * xCount * sizeof(T));
    if (bufNew == NULL) Exit("out of memory");

    unsigned int yCopy = (yCount < yCountNew) ? yCount : yCountNew;

    T *ptrNew = bufNew;
    for (unsigned int x = 0; x < xCount; x++) {
        T *ptr = buf + x * yCount;
        unsigned int y = 0;
        for (; y < yCopy    ; y++) *ptrNew++ = *ptr++;
        for (; y < yCountNew; y++) *ptrNew++ = fill;
    }

    if (buf != NULL) mxFree(buf);

    buf    = bufNew;
    yCount = yCountNew;

}

/**********************************************************************************************************************/

void MakeSpikes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    double *rates = mxGetPr(prhs[0]);
    unsigned int rCount = mxGetM(prhs[0]);
    unsigned int xCount = mxGetN(prhs[0]);
    double dt = mxGetScalar(prhs[1]);
    unsigned int factor = (unsigned int)mxGetScalar(prhs[2]);

    plhs[0] = mxCreateNumericMatrix(0, xCount, mxSINGLE_CLASS, mxREAL);
    float *buf = (float *)mxGetData(plhs[0]);
    unsigned int yAlloc = 0;
    unsigned int ySize = 0;

    RandGen gen;
    gen.Open();

    double *rPtr = rates;
    for (unsigned int x = 0; x < xCount; x++) {
        unsigned int y = 0;
        for (unsigned int r = 0; r < rCount; r++) {
            double prob = *rPtr++ * dt;
            for (unsigned int f = 0; f < factor; f++) {
                if (gen.Rand() <= prob) {
                    if (ySize <= y) {
                        ySize = y + 1;
                        if (yAlloc < ySize) ResizeCols(buf, yAlloc, xCount, 2 * ySize, CNS_INTMAX);
                    }
                    buf[x * yAlloc + y++] = (float)(r * factor + f);
                }
            }
        }
    }

    gen.Close();

    ResizeCols(buf, yAlloc, xCount, ySize + 1, CNS_INTMAX);
    mxSetData(plhs[0], buf);
    mxSetM(plhs[0], yAlloc);

}

/**********************************************************************************************************************/

void FindSpikes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    float *volts = (float *)mxGetData(prhs[0]);
    unsigned int vCount = mxGetM(prhs[0]);
    unsigned int xCount = mxGetN(prhs[0]);
    float thres = (float)mxGetScalar(prhs[1]);
    unsigned int factor = (unsigned int)mxGetScalar(prhs[2]);

    plhs[0] = mxCreateNumericMatrix(0, xCount, mxSINGLE_CLASS, mxREAL);
    float *buf = (float *)mxGetData(plhs[0]);
    unsigned int yAlloc = 0;
    unsigned int ySize = 0;

    float *vPtr = volts;
    for (unsigned int x = 0; x < xCount; x++) {
        float volt1 = *vPtr++;
        unsigned int y = 0;
        for (unsigned int v = 1; v < vCount; v++) {
            float volt2 = *vPtr++;
            if ((volt1 < thres) && (thres <= volt2)) {
                if (ySize <= y) {
                    ySize = y + 1;
                    if (yAlloc < ySize) ResizeCols(buf, yAlloc, xCount, 2 * ySize, CNS_INTMAX);
                }
                buf[x * yAlloc + y++] = floorf(((float)v - 0.4999f) * (float)factor + 0.5f);
            }
            volt1 = volt2;
        }
    }

    ResizeCols(buf, yAlloc, xCount, ySize + 1, CNS_INTMAX);
    mxSetData(plhs[0], buf);
    mxSetM(plhs[0], yAlloc);

}

/**********************************************************************************************************************/

void BinSpikes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    float *times = (float *)mxGetData(prhs[0]);
    unsigned int tCount = mxGetM(prhs[0]);
    unsigned int xCount = mxGetN(prhs[0]);
    unsigned int bCount = (unsigned int)mxGetScalar(prhs[1]);
    unsigned int factor = (unsigned int)mxGetScalar(prhs[2]);

    plhs[0] = mxCreateNumericMatrix(bCount, xCount, mxUINT32_CLASS, mxREAL);
    unsigned int *buf = (unsigned int *)mxGetData(plhs[0]);

    float *tPtr = times;
    for (unsigned int x = 0; x < xCount; x++) {
        for (unsigned int t = 0; t < tCount; t++) {
            float time = *tPtr++;
            if (time < CNS_INTMAX) {
                unsigned int b = (unsigned int)time / factor;
                if (b < bCount) buf[x * bCount + b]++;
            }
        }
    }

}

/**********************************************************************************************************************/

#include "common_def.h"
