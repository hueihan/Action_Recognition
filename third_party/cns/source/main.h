#include "common_dec.h"

/**********************************************************************************************************************/

#include "util_dec.h"

/**********************************************************************************************************************/

class _T_BASE {
public:
    int z;
};

_INLINE _T_BASE _P_BASE(int z) {
    _T_BASE p;
    p.z = z;
    return p;
}

_INLINE int _Z_BASE(_T_BASE p) {
    return p.z;
}

/**********************************************************************************************************************/

#include _USER_GLOBAL

/**********************************************************************************************************************/

// Use all available constant memory, currently limited to 64K bytes.  It would be nice to query the device for the
// amount of constant memory available.

const unsigned int _MAX_CDATA = 8192;
const unsigned int _MAX_CMETA = 16384;

// These are fairly arbitrary limits governing the size of statically allocated arrays in host memory.  There would not
// be much penalty for increasing them, and they could be removed altogether if we were willing to make the code a bit
// more complicated (i.e. use dynamic allocation and deallocation).

const unsigned int _MAX_KERNELS = 100;
const unsigned int _MAX_RESULTS = 100;
const unsigned int _NAME_LEN    = 32;

/**********************************************************************************************************************/

class _OutTable {
public:

    float        *m_ptr[_NUM_CV_NZ];
    unsigned int  m_h  [_NUM_CV_NZ];
    unsigned int  m_w  [_NUM_CV_NZ];

};

/**********************************************************************************************************************/

// We put this structure in union with an array of uints so that the structure can be read in by multiple threads in
// parallel using a single coalesced read.  The structure must be exactly 128 bytes so that each structure in an array
// of structures will be correctly aligned for coalesced reading.

const unsigned int _LAYERDATA_UINTS = 32;

class _LayerData {
public:

    union {

        struct {

            unsigned int        m_yCount0;
            unsigned int        m_ySize0;
            unsigned int        m_yCount;
            unsigned int        m_ySize;
            unsigned int        m_xCount;
            unsigned int        m_sSize;

            unsigned int        m_gmvOff;
            unsigned int        m_mvOff;
            unsigned int        m_gcOff;
            unsigned int        m_cOff;

            unsigned int        m_tOff;

            unsigned int        m_blockYSize;

            float              *m_ndPtr;
            const unsigned int *m_nmPtr;
            float              *m_sdPtr;
            const ushort4      *m_smPtr;

            unsigned short      m_entry[_LT_LEN];

        };

        unsigned int m_array[_LAYERDATA_UINTS];

    };

};

/**********************************************************************************************************************/

class _Layer : private _LayerData {
public:

    void LInit(const mxArray *layers, unsigned int z);

    float *GetNDPtr();
    float *GetSDPtr();

    #ifndef _GPU
        void LRun();
    #endif

private:

    // We rely on these members being at the end of the structure.

    unsigned int m_z;
    unsigned int m_typeNo;

};

/**********************************************************************************************************************/

class _Kernel {
public:

    void KInit(const mxArray *kernels, unsigned int k);

    unsigned int RunOrder();

    void KRun();

private:

    char                m_type[_NAME_LEN];
    unsigned int        m_typeNo;

    unsigned int        m_runOrder;

    unsigned int        m_blockSize;

    unsigned int        m_bCount;
    const ushort4      *m_bPtr;

    unsigned int        m_zCount;
    const unsigned int *m_zPtr;

    unsigned int        m_gridYSize;
    unsigned int        m_gridXSize;
    unsigned int        m_blockYSize;
    unsigned int        m_blockXSize;

};

/**********************************************************************************************************************/

class _Result {
public:

    void RInit(const mxArray *p, unsigned int resultNo, const mxArray *res);
    void RAlloc(unsigned int resultNo, unsigned int totalSamples, mxArray *&res, bool needHold);

    void SampleToHold(unsigned int sampleNo);
    void HoldToResult(unsigned int totalSamples);
    void SampleToResult();
    void Update();

private:

    unsigned int  m_varType;

    float        *m_sBuf;
    unsigned int  m_sBufH;
    unsigned int  m_sBufW;

    unsigned int  m_hCount;
    unsigned int  m_wCount;
    unsigned int  m_dCount;
    unsigned int  m_count;

    float        *m_rBuf;
    float        *m_hBuf;

};

/**********************************************************************************************************************/

static void _Claim  (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Release(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Init   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Done   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Run    (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Get    (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void _Set    (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

static void _AtExit();
static void _Dealloc();
static void _Exit(const char *format, ...);

#include _USER_ALLKERNELS_DEC

static void _NeuronExit(unsigned int z, unsigned int y, unsigned int x, const char *format, ...);
static void _NeuronInfo(unsigned int z, unsigned int y, unsigned int x, const char *format, ...);

/**********************************************************************************************************************/

unsigned int _g_initLevel = 0;
unsigned int _g_exitLevel = 0;

_DeviceProps _g_props;

bool         _g_feedforward;

unsigned int _g_mvOff;
unsigned int _g_cOff;

#ifdef _GPU
    __constant__ float          _g_cData[_MAX_CDATA];
    __constant__ unsigned short _g_cMeta[_MAX_CMETA];
#else
    float          _g_cData[_MAX_CDATA];
    unsigned short _g_cMeta[_MAX_CMETA];
#endif

_OutTable     _g_tOut;

float        *_g_dData;
unsigned int *_g_dNeurons;
ushort4      *_g_dSynapses;
ushort4      *_g_dBlocks;
_LayerData   *_g_dLayers;

unsigned int *_g_hKernelZs;

unsigned int  _g_lCount;
_Layer       *_g_layers[_MAX_LAYERS];

unsigned int  _g_kCount;
_Kernel      *_g_kernels[_MAX_KERNELS];

unsigned int  _g_stepNo;

unsigned int  _g_holdBufferCount;
float        *_g_holdBuffers[_MAX_RESULTS];

/**********************************************************************************************************************/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_CB = prhs[0];

    unsigned int mode = (unsigned int)mxGetScalar(prhs[1]);

    switch (mode) {
    case 0: _Claim  (nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 1: _Release(nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 2: _Init   (nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 3: _Done   (nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 4: _Run    (nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 5: _Get    (nlhs, plhs, nrhs - 2, prhs + 2); break;
    case 6: _Set    (nlhs, plhs, nrhs - 2, prhs + 2); break;
    }

}

/**********************************************************************************************************************/

void _Claim(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_initLevel = 1;
    _g_exitLevel = 0;

    int desiredDeviceNo = (int)mxGetScalar(prhs[0]);
    bool nice = ((int)mxGetScalar(prhs[1]) > 0);

    _ClaimDevice(desiredDeviceNo, nice, _g_props);

    mxArray *p = mxCreateStructMatrix(1, 1, 0, NULL);

    mxSetFieldByNumber(p, 0, mxAddField(p, "deviceNo"       ), mxCreateDoubleScalar(_g_props.deviceNo       ));
    mxSetFieldByNumber(p, 0, mxAddField(p, "blockSizeAlign" ), mxCreateDoubleScalar(_g_props.blockSizeAlign ));
    mxSetFieldByNumber(p, 0, mxAddField(p, "blockYSizeAlign"), mxCreateDoubleScalar(_g_props.blockYSizeAlign));
    mxSetFieldByNumber(p, 0, mxAddField(p, "maxTexYSize"    ), mxCreateDoubleScalar(_g_props.maxTexYSize    ));
    mxSetFieldByNumber(p, 0, mxAddField(p, "maxTexXSize"    ), mxCreateDoubleScalar(_g_props.maxTexXSize    ));

    plhs[0] = p;

    mexLock();
    mexAtExit(_AtExit);

}

/**********************************************************************************************************************/

void _Release(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_exitLevel = 0;

    _Dealloc();

}

/**********************************************************************************************************************/

void _AtExit() {

    _g_exitLevel = 0;

    _Dealloc();

}

/**********************************************************************************************************************/

void _Init(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_initLevel = 2;
    _g_exitLevel = 1;

    _ClearConst("_g_cData", _g_cData);
    _ClearConst("_g_cMeta", _g_cMeta);

    for (int f = 0; f < _NUM_T; f++) {
        _ClearTex(*_GetTTexA(f), *_GetTTexB(f));
    }

    for (int f = 0; f < _NUM_CC; f++) {
        _ClearTex(*_GetCTexA(f), *_GetCTexB(f));
    }

    for (int f = 0; f < _NUM_CV; f++) {
        _ClearTex(*_GetVTexA(f), *_GetVTexB(f));
        _ClearBuf(_g_tOut.m_ptr[f]);
    }

    _ClearBuf(_g_dData    );
    _ClearBuf(_g_dNeurons );
    _ClearBuf(_g_dSynapses);
    _ClearBuf(_g_dBlocks  );
    _ClearBuf(_g_dLayers  );

    _ClearHostBuf(_g_hKernelZs);

    _g_lCount = 0;
    _g_kCount = 0;

    const mxArray *s = prhs[0];
    const mxArray *h = prhs[1];

    _g_feedforward = ((int)mxGetScalar(mxGetField(s, 0, "feedforward")) > 0);

    _g_mvOff = (unsigned int)mxGetScalar(mxGetField(s, 0, "mvOff"));
    _g_cOff  = (unsigned int)mxGetScalar(mxGetField(s, 0, "cOff" ));

    const mxArray *tTData = mxGetField(h, 0, "tTData");
    for (int f = 0; f < _NUM_T; f++) {
        const mxArray *tex = mxGetCell(tTData, f);
        unsigned int yCount = mxGetM(tex);
        unsigned int xCount = mxGetN(tex);
        _AllocTexArray(*_GetTTexA(f), *_GetTTexB(f), tex, yCount, xCount, "texture constants");
    }

    const mxArray *tCData = mxGetField(h, 0, "tCData");
    for (int f = 0; f < _NUM_CC; f++) {
        const mxArray *tex = mxGetCell(tCData, f);
        unsigned int ySize  = mxGetM(tex);
        unsigned int xCount = mxGetN(tex);
        _AllocTexArray(*_GetCTexA(f), *_GetCTexB(f), tex, ySize, xCount, "common constants");
    }

    const mxArray *tVData = mxGetField(h, 0, "tVData");
    for (int f = 0; f < _NUM_CV; f++) {
        const mxArray *tex = mxGetCell(tVData, f);
        unsigned int ySize  = mxGetM(tex);
        unsigned int xCount = mxGetN(tex);
        _AllocBufA(_g_tOut.m_ptr[f], tex, 1, "common variables output");
        _g_tOut.m_h[f] = ySize;
        _g_tOut.m_w[f] = xCount;
        if (_g_feedforward) {
            _AllocTexLinear(*_GetVTexA(f), *_GetVTexB(f), _g_tOut.m_ptr[f], ySize, xCount, "common variables");
        } else {
            _AllocTexArray (*_GetVTexA(f), *_GetVTexB(f), NULL            , ySize, xCount, "common variables");
        }
    }

    _AllocBufA(_g_dData    , mxGetField(h, 0, "dData"    ), 1, "device memory");
    _AllocBufA(_g_dNeurons , mxGetField(h, 0, "dNeurons" ), 1, "neurons");
    _AllocBufA(_g_dSynapses, mxGetField(h, 0, "dSynapses"), 4, "synapses");
    _AllocBufA(_g_dBlocks  , mxGetField(h, 0, "dBlocks"  ), 4, "blocks");

    _AllocHostBufA(_g_hKernelZs, mxGetField(h, 0, "hKernelZs"), 1, "kernel zs");

    const mxArray *layers = mxGetField(s, 0, "layers");
    unsigned int lCount = mxGetNumberOfElements(layers);
    if (lCount > _MAX_LAYERS) {
        _Exit("maximum number of layers (%u) exceeded", _MAX_LAYERS);
    }
    for (unsigned int z = 0; z < lCount; z++) {
        _g_layers[z] = new _Layer();
        _g_lCount = z + 1;
        _g_layers[z]->LInit(layers, z);
    }

    if (sizeof(_LayerData) != _LAYERDATA_UINTS * sizeof(unsigned int)) {
        _Exit("size of LayerData structure is not %u bytes", _LAYERDATA_UINTS * sizeof(unsigned int));
    }
    _LayerData layerData[_MAX_LAYERS];
    for (unsigned int z = 0; z < lCount; z++) {
        layerData[z] = *(_LayerData *)_g_layers[z];
    }
    _AllocBufB(_g_dLayers, layerData, lCount, "layer data");

    const mxArray *kernels = mxGetField(s, 0, "kernels");
    unsigned int kCount = mxGetNumberOfElements(kernels);
    if (kCount > _MAX_KERNELS) {
        _Exit("maximum number of kernels (%u) exceeded", _MAX_KERNELS);
    }
    for (unsigned int k = 0; k < kCount; k++) {
        _g_kernels[k] = new _Kernel();
        _g_kCount = k + 1;
        _g_kernels[k]->KInit(kernels, k);
    }

    _g_stepNo = 0;

    const mxArray *cData = mxGetField(h, 0, "cData");
    const mxArray *cMeta = mxGetField(h, 0, "cMeta");
    if (mxGetNumberOfElements(cData) > _MAX_CDATA) {
        _Exit("maximum number of constants (%u) exceeded", _MAX_CDATA);
    }
    if (mxGetNumberOfElements(cMeta) > _MAX_CMETA) {
        _Exit("maximum number of metadata elements (%u) exceeded", _MAX_CMETA);
    }
    _AllocConst("_g_cData", _g_cData, (float          *)mxGetData(cData), mxGetNumberOfElements(cData));
    _AllocConst("_g_cMeta", _g_cMeta, (unsigned short *)mxGetData(cMeta), mxGetNumberOfElements(cMeta));

}

/**********************************************************************************************************************/

void _Done(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_exitLevel = 1;

    _Dealloc();

}

/**********************************************************************************************************************/

void _Dealloc() {

    if ((_g_initLevel >= 3) && (_g_exitLevel < 3)) {

        for (unsigned int i = 0; i < _g_holdBufferCount; i++) {
            _DeallocBuf(_g_holdBuffers[i]);
        }
        _g_holdBufferCount = 0;

        _g_initLevel = 2;

    }

    if ((_g_initLevel >= 2) && (_g_exitLevel < 2)) {

        _DeallocConst("_g_cData", _g_cData);
        _DeallocConst("_g_cMeta", _g_cMeta);

        for (int f = 0; f < _NUM_T; f++) {
            _DeallocTex(*_GetTTexA(f), *_GetTTexB(f));
        }

        for (int f = 0; f < _NUM_CC; f++) {
            _DeallocTex(*_GetCTexA(f), *_GetCTexB(f));
        }

        for (int f = 0; f < _NUM_CV; f++) {
            _DeallocTex(*_GetVTexA(f), *_GetVTexB(f));
            _DeallocBuf(_g_tOut.m_ptr[f]);
        }

        _DeallocBuf(_g_dData    );
        _DeallocBuf(_g_dNeurons );
        _DeallocBuf(_g_dSynapses);
        _DeallocBuf(_g_dBlocks  );
        _DeallocBuf(_g_dLayers  );

        _DeallocHostBuf(_g_hKernelZs);

        for (unsigned int z = 0; z < _g_lCount; z++) {
            delete _g_layers[z];
        }
        _g_lCount = 0;

        for (unsigned int k = 0; k < _g_kCount; k++) {
            delete _g_kernels[k];
        }
        _g_kCount = 0;

        _g_initLevel = 1;

    }

    if ((_g_initLevel >= 1) && (_g_exitLevel < 1)) {

        _ReleaseDevice();

        mexUnlock();

        _g_initLevel = 0;

    }

}

/**********************************************************************************************************************/

void _Exit(const char *format, ...) {

    _Dealloc();

    char msg[_ERRMSG_LEN];

    va_list argList;
    va_start(argList, format);
    vsprintf(msg, format, argList);
    va_end(argList);

    mexErrMsgTxt(msg);

}

/**********************************************************************************************************************/

void _Layer::LInit(const mxArray *layers, unsigned int z) {

    m_yCount0    = (unsigned int)mxGetScalar(mxGetField(layers, z, "yCount0"   ));
    m_ySize0     = (unsigned int)mxGetScalar(mxGetField(layers, z, "ySize0"    ));
    m_yCount     = (unsigned int)mxGetScalar(mxGetField(layers, z, "yCount"    ));
    m_ySize      = (unsigned int)mxGetScalar(mxGetField(layers, z, "ySize"     ));
    m_xCount     = (unsigned int)mxGetScalar(mxGetField(layers, z, "xCount"    ));
    m_sSize      = (unsigned int)mxGetScalar(mxGetField(layers, z, "sSize"     ));
    m_gmvOff     = (unsigned int)mxGetScalar(mxGetField(layers, z, "gmvOff"    ));
    m_mvOff      = (unsigned int)mxGetScalar(mxGetField(layers, z, "mvOff"     ));
    m_gcOff      = (unsigned int)mxGetScalar(mxGetField(layers, z, "gcOff"     ));
    m_cOff       = (unsigned int)mxGetScalar(mxGetField(layers, z, "cOff"      ));
    m_tOff       = (unsigned int)mxGetScalar(mxGetField(layers, z, "tOff"      ));
    m_blockYSize = (unsigned int)mxGetScalar(mxGetField(layers, z, "blockYSize"));

    m_ndPtr = _g_dData     + (unsigned int)mxGetScalar(mxGetField(layers, z, "ndOff"));
    m_nmPtr = _g_dNeurons  + (unsigned int)mxGetScalar(mxGetField(layers, z, "nmOff"));
    m_sdPtr = _g_dData     + (unsigned int)mxGetScalar(mxGetField(layers, z, "sdOff"));
    m_smPtr = _g_dSynapses + (unsigned int)mxGetScalar(mxGetField(layers, z, "smOff"));

    const double *entry = mxGetPr(mxGetField(layers, z, "entry"));
    for (unsigned int i = 0; i < _LT_LEN; i++) {
        m_entry[i] = (unsigned short)entry[i];
    }

    m_z      = z;
    m_typeNo = (unsigned int)mxGetScalar(mxGetField(layers, z, "typeNo")) - 1;

}

/**********************************************************************************************************************/

float *_Layer::GetNDPtr() {

    return m_ndPtr;

}

/**********************************************************************************************************************/

float *_Layer::GetSDPtr() {

    return m_sdPtr;

}

/**********************************************************************************************************************/

void _Kernel::KInit(const mxArray *kernels, unsigned int k) {

    mxGetString(mxGetField(kernels, k, "type"), m_type, _NAME_LEN);

    m_typeNo    = (unsigned int)mxGetScalar(mxGetField(kernels, k, "typeNo"   )) - 1;
    m_runOrder  = (unsigned int)mxGetScalar(mxGetField(kernels, k, "runOrder" )) - 1;
    m_blockSize = (unsigned int)mxGetScalar(mxGetField(kernels, k, "blockSize"));
    m_bCount    = (unsigned int)mxGetScalar(mxGetField(kernels, k, "bCount"   ));
    m_zCount    = (unsigned int)mxGetScalar(mxGetField(kernels, k, "zCount"   ));

    m_bPtr = _g_dBlocks   + (unsigned int)mxGetScalar(mxGetField(kernels, k, "bOff"));
    m_zPtr = _g_hKernelZs + (unsigned int)mxGetScalar(mxGetField(kernels, k, "zOff"));

    // We use both dimensions to avoid exceeding limits.  The particular 2D shapes of the grid and blocks are
    // irrelevant.

    if (m_bCount <= 65535) {
        m_gridXSize = 1;
        m_gridYSize = m_bCount;
    } else {
        m_gridXSize = (unsigned int)sqrt((double)m_bCount);
        m_gridYSize = (unsigned int)ceil((double)m_bCount / (double)m_gridXSize);
    }

    m_blockXSize = _g_props.blockSizeAlign;
    m_blockYSize = m_blockSize / _g_props.blockSizeAlign;

}

/**********************************************************************************************************************/

unsigned int _Kernel::RunOrder() {

    return m_runOrder;

}

/**********************************************************************************************************************/

void _Run(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_initLevel = 3;
    _g_exitLevel = 2;

    _g_holdBufferCount = 0;

    unsigned int steps      = (unsigned int)mxGetScalar(prhs[0]);
    unsigned int sampleRate = (unsigned int)mxGetScalar(prhs[1]);

    unsigned int totalSamples = steps / sampleRate;

    unsigned int numResults = mxGetNumberOfElements(prhs[2]);
    if (numResults > _MAX_RESULTS) {
        _Exit("maximum number of results (%u) exceeded", _MAX_RESULTS);
    }

    _Result results[_MAX_RESULTS];

    for (unsigned int i = 0; i < numResults; i++) {
        results[i].RInit(prhs[2], i, NULL);
        results[i].RAlloc(i, totalSamples, plhs[i], true);
    }

    unsigned int stepsUntilSample = sampleRate;
    unsigned int sampleNo         = 0;

    for (unsigned int i = 0; i < steps; i++) {

        for (unsigned int k = 0; k < _g_kCount; k++) {

            unsigned int j = 0;

            if ((k == 0) || (j < _g_kernels[k]->RunOrder())) {

                for (int f = 0; f < _NUM_CV; f++) {
                    // NOTE: in the future, if using substeps, we might only update the changed part of the texture.
                    _PublishTex(*_GetVTexA(f), *_GetVTexB(f), _g_tOut.m_ptr[f], _g_tOut.m_h[f], _g_tOut.m_w[f], "common variables");
                }

                j = _g_kernels[k]->RunOrder();

            }

            _g_kernels[k]->KRun();

        }

        if (--stepsUntilSample == 0) {
            for (unsigned int j = 0; j < numResults; j++) {
                results[j].SampleToHold(sampleNo);
            }
            stepsUntilSample = sampleRate;
            sampleNo++;
        }

        _g_stepNo++;

    }

    for (unsigned int i = 0; i < numResults; i++) {
        results[i].HoldToResult(totalSamples);
    }

    _Dealloc();

}

/**********************************************************************************************************************/

void _Get(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_exitLevel = 2;

    _Result result;

    result.RInit(prhs[0], 0, NULL);
    result.RAlloc(0, 1, plhs[0], false);

    result.SampleToResult();

}

/**********************************************************************************************************************/

void _Set(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_exitLevel = 2;

    _Result result;

    result.RInit(prhs[0], 0, prhs[1]);

    result.Update();

}

/**********************************************************************************************************************/

void _Result::RInit(const mxArray *p, unsigned int resultNo, const mxArray *res) {

    m_varType = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "varType"));

    unsigned int z   = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "z"  )) - 1;
    unsigned int pos = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "pos")) - 1;

    switch (m_varType) {
    case 0: m_sBuf = _g_tOut.m_ptr[pos]      ; break;
    case 1: m_sBuf = _g_layers[z]->GetNDPtr(); break;
    case 2: m_sBuf = _g_layers[z]->GetSDPtr(); break;
    case 3: m_sBuf = _g_cData                ; break;
    }

    m_sBufH = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "height"));
    m_sBufW = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "width" ));

    unsigned int hOff = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "hOff"));
    unsigned int wOff = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "wOff"));
    unsigned int dOff = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "dOff"));

    m_sBuf += (dOff * m_sBufW + wOff) * m_sBufH + hOff;

    m_hCount = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "hCount"));
    m_wCount = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "wCount"));
    m_dCount = (unsigned int)mxGetScalar(mxGetField(p, resultNo, "dCount"));

    m_count = m_hCount * m_wCount * m_dCount;

    if (res == NULL) {
        m_rBuf = NULL;
    } else {
        m_rBuf = (float *)mxGetData(res);
    }

    m_hBuf = NULL;

}

/**********************************************************************************************************************/

void _Result::RAlloc(unsigned int resultNo, unsigned int totalSamples, mxArray *&res, bool needHold) {

    res = mxCreateNumericMatrix(m_count, totalSamples, mxSINGLE_CLASS, mxREAL);
    m_rBuf = (float *)mxGetData(res);

    if (needHold) {

        #ifdef _GPU
            _AllocBufB(m_hBuf, (float *)NULL, m_count * totalSamples, "result");
            _g_holdBufferCount = resultNo + 1;
            _g_holdBuffers[resultNo] = m_hBuf;
        #else
            m_hBuf = m_rBuf;
        #endif

    } else {

        m_hBuf = NULL;

    }

}

/**********************************************************************************************************************/

// NOTE: If a layer has more than one runOrder, we probably want to call different kernels.  This will probably
// require a little more configuration on the part of the user.

#ifdef _GPU

    void _Kernel::KRun() {

        if (m_bCount == 0) return;

        dim3 gridDim;
        dim3 blockDim;
        gridDim.x  = m_gridYSize;
        gridDim.y  = m_gridXSize;
        gridDim.z  = 1;
        blockDim.x = m_blockYSize;
        blockDim.y = m_blockXSize;
        blockDim.z = 1;

        switch (m_typeNo) {
        #include _USER_ALLKERNELS_RUN
        default:
            _Exit("invalid type number (%u)", m_typeNo + 1);
            break;
        }

    }

#else

    void _Kernel::KRun() {

        for (unsigned int i = 0; i < m_zCount; i++) {
            _g_layers[m_zPtr[i]]->LRun();
        }

    }

    void _Layer::LRun() {

        switch (m_typeNo) {
        #include _USER_ALLKERNELS_RUN
        default:
            _Exit("invalid type number (%u)", m_typeNo + 1);
            break;
        }

    }

#endif

/**********************************************************************************************************************/

#include "kernel_macros.h"

#include _USER_ALLKERNELS_DEF

/**********************************************************************************************************************/

void _NeuronExit(unsigned int z, unsigned int y, unsigned int x, const char *format, ...) {

    _Dealloc();

    char msg[_ERRMSG_LEN];

    sprintf(msg, "step=%u z=%u i=%u: ", _g_stepNo + 1, z + 1, _CBYX2E(z, y, x) + 1);

    va_list argList;
    va_start(argList, format);
    vsprintf(msg + strlen(msg), format, argList);
    va_end(argList);

    mexErrMsgTxt(msg);

}

/**********************************************************************************************************************/

void _NeuronInfo(unsigned int z, unsigned int y, unsigned int x, const char *format, ...) {

    char msg[_ERRMSG_LEN];

    sprintf(msg, "step=%u z=%u i=%u: ", _g_stepNo + 1, z + 1, _CBYX2E(z, y, x) + 1);

    va_list argList;
    va_start(argList, format);
    vsprintf(msg + strlen(msg), format, argList);
    va_end(argList);

    mexPrintf("%s\n", msg);
    mexEvalString("drawnow");

}

/**********************************************************************************************************************/

void _Result::SampleToHold(unsigned int sampleNo) {

    _CopyBuf3D(
        m_hBuf + sampleNo * m_count, 'd',
        m_sBuf, m_sBufH, m_sBufW,
        m_hCount, m_wCount, m_dCount,
        "variables");

}

/**********************************************************************************************************************/

void _Result::HoldToResult(unsigned int totalSamples) {

    #ifdef _GPU
        _CopyBuf1D(m_rBuf, 'h', m_hBuf, m_count * totalSamples, "hold");
    #endif

}

/**********************************************************************************************************************/

void _Result::SampleToResult() {

    switch (m_varType) {
    case 0:
    case 1:
    case 2:
        _CopyBuf3D(
            m_rBuf, 'h',
            m_sBuf, m_sBufH, m_sBufW,
            m_hCount, m_wCount, m_dCount,
            "variables");
        break;
    case 3:
        _CopyConst(m_rBuf, "_g_cData", _g_cData, m_sBuf - _g_cData, m_count);
        break;

    }

}

/**********************************************************************************************************************/

void _Result::Update() {

    switch (m_varType) {
    case 0:
    case 1:
    case 2:
        _UpdateBuf3D(
            m_sBuf, m_sBufH, m_sBufW,
            m_rBuf, 'h',
            m_hCount, m_wCount, m_dCount,
            "variables");
        break;
    case 3:
        _UpdateConst("_g_cData", _g_cData, m_sBuf - _g_cData, m_rBuf, m_count);
        break;
    }

}

/**********************************************************************************************************************/

#include "common_def.h"

#include "util_def.h"
