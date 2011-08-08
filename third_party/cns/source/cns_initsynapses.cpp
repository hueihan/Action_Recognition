#include "common_dec.h"

/**********************************************************************************************************************/

class TransTable {
public:

    void Init(const mxArray *ts);
    bool ValidY(unsigned int z, unsigned int y);
    bool ValidE(unsigned int z, unsigned int e);
    void E2YX(unsigned int z, unsigned int e, unsigned int &y, unsigned int &x);

private:

    unsigned int NDim   [_MAX_LAYERS];
    unsigned int Siz2   [_MAX_LAYERS][_MAX_DIMS];
    unsigned int Perm   [_MAX_LAYERS][_MAX_DIMS];
    unsigned int IDim   [_MAX_LAYERS][_MAX_DIMS];
    unsigned int Siz3a  [_MAX_LAYERS][_MAX_DIMS];
    unsigned int ECount [_MAX_LAYERS];
    unsigned int YCount0[_MAX_LAYERS];
    unsigned int YSize0 [_MAX_LAYERS];

};

/**********************************************************************************************************************/

static const unsigned short *Input16   (const unsigned short *a, unsigned int y, unsigned int x, unsigned int s);
static const unsigned int   *Input32   (const unsigned int   *a, unsigned int y, unsigned int x, unsigned int s);
static       unsigned int   *NeuronPtr (                         unsigned int y, unsigned int x                );
static       unsigned short *SynapsePtr(                         unsigned int y, unsigned int x, unsigned int s);

/**********************************************************************************************************************/

char Msg[_ERRMSG_LEN];

#define Exit(...) do { sprintf(Msg, __VA_ARGS__); mexErrMsgTxt(Msg); } while(false)

/**********************************************************************************************************************/

unsigned int          Z;

unsigned int          YCount;
unsigned int          XCount;
unsigned int          SSize;

const unsigned int   *SynapseIs;
const unsigned short *SynapseZs;
const unsigned short *SynapseTs;

unsigned int          YSize;

unsigned int          ZCount;
TransTable            TTable;
const unsigned int   *ValidLayer;

unsigned int         *Neurons;
unsigned short       *Synapses;

/**********************************************************************************************************************/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    _g_CB = prhs[0];

    Z = (unsigned int)mxGetScalar(prhs[1]) - 1;

    YCount = _GetDimSize(prhs[2], 0);
    XCount = _GetDimSize(prhs[2], 1);
    SSize  = _GetDimSize(prhs[2], 2);

    SynapseIs = (const unsigned int   *)mxGetData(prhs[2]);
    SynapseZs = (const unsigned short *)mxGetData(prhs[3]);
    SynapseTs = (const unsigned short *)mxGetData(prhs[4]);

    YSize = (unsigned int)mxGetScalar(prhs[5]);

    ZCount = mxGetNumberOfElements(prhs[6]);

    TTable.Init(prhs[6]);

    ValidLayer = (const unsigned int *)mxGetData(prhs[7]);

    mwSize sizes[4];

    sizes[0] = 1;
    sizes[1] = YSize;
    sizes[2] = XCount;
    plhs[0] = mxCreateNumericArray(3, sizes, mxUINT32_CLASS, mxREAL);
    Neurons = (unsigned int *)mxGetData(plhs[0]);

    sizes[0] = 4;
    sizes[1] = YSize;
    sizes[2] = XCount;
    sizes[3] = SSize;
    plhs[1] = mxCreateNumericArray(4, sizes, mxUINT16_CLASS, mxREAL);
    Synapses = (unsigned short *)mxGetData(plhs[1]);

    for (unsigned int x = 0; x < XCount; x++) {
        for (unsigned int y = 0; y < YCount; y++) {

            if (!TTable.ValidY(Z, y)) continue;

            unsigned int *nPtr = NeuronPtr(y, x);
            nPtr[0] = SSize;
            bool done = false;

            for (unsigned int s = 0; s < SSize; s++) {

                unsigned short *sPtr = SynapsePtr(y, x, s);

                unsigned int pe = *Input32(SynapseIs, y, x, s);
                unsigned int pz = *Input16(SynapseZs, y, x, s);
                unsigned int pt = *Input16(SynapseTs, y, x, s);

                if (!done && (pe == 0)) {
                    nPtr[0] = s;
                    done = true;
                }

                if (done) {

                    if (pz != 0) {
                        Exit("z=%u, i=%u, synapse %u: nonzero synapseZs value after end of synapses",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }

                    if (pe != 0) {
                        Exit("z=%u, i=%u, synapse %u: nonzero synapseIs value after end of synapses",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }

                    if (pt != 0) {
                        Exit("z=%u, i=%u, synapse %u: nonzero synapseTs value after end of synapses",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }

                } else {

                    if ((pz == 0) || (pz > ZCount) || !ValidLayer[pz - 1]) {
                        Exit("z=%u, i=%u, synapse %u: invalid synapseZs value",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }
                    pz--;

                    if ((pe == 0) || !TTable.ValidE(pz, pe - 1)) {
                        Exit("z=%u, i=%u, synapse %u: invalid synapseIs value",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }
                    pe--;

                    if (pt == 0) {
                        Exit("z=%u, i=%u, synapse %u: invalid synapseTs value",
                            Z + 1, _CBYX2E(Z, y, x) + 1, s + 1);
                    }
                    pt--;

                    unsigned int py, px;
                    TTable.E2YX(pz, pe, py, px);

                    sPtr[0] = (unsigned short)px;
                    sPtr[1] = (unsigned short)py;
                    sPtr[2] = (unsigned short)pz;
                    sPtr[3] = (unsigned short)pt;

                }

            }

        }
    }

}

/**********************************************************************************************************************/

void TransTable::Init(const mxArray *ts) {

    unsigned int nz = mxGetNumberOfElements(ts);
    if (nz > _MAX_LAYERS) Exit("maximum number of layers (%u) exceeded", _MAX_LAYERS);

    for (unsigned int z = 0; z < nz; z++) {

        const mxArray *siz2  = mxGetField(ts, z, "siz2" );
        const mxArray *perm  = mxGetField(ts, z, "perm" );
        const mxArray *idim  = mxGetField(ts, z, "idim" );
        const mxArray *siz3  = mxGetField(ts, z, "siz3" );
        const mxArray *siz3a = mxGetField(ts, z, "siz3a");

        NDim[z] = mxGetNumberOfElements(siz2);
        if (NDim[z] > _MAX_DIMS) Exit("maximum number of dimensions (%u) exceeded", _MAX_DIMS);

        ECount[z] = 1;
        for (unsigned int d = 0; d < NDim[z]; d++) {
            Siz2 [z][d] = (unsigned int)mxGetPr(siz2 )[d];
            Perm [z][d] = (unsigned int)mxGetPr(perm )[d] - 1;
            IDim [z][d] = (unsigned int)mxGetPr(idim )[d] - 1;
            Siz3a[z][d] = (unsigned int)mxGetPr(siz3a)[d];
            ECount[z] *= Siz2[z][d];
        }

        YCount0[z] = (unsigned int)mxGetPr(siz3 )[0];
        YSize0 [z] = (unsigned int)mxGetPr(siz3a)[0];

        for (unsigned int d = 1; d < NDim[z]; d++) {
            Siz2[z][d] *= Siz2[z][d - 1];
        }

    }

}
/**********************************************************************************************************************/

bool TransTable::ValidY(unsigned int z, unsigned int y) {

    return ((y % YSize0[z]) < YCount0[z]);

}

/**********************************************************************************************************************/

bool TransTable::ValidE(unsigned int z, unsigned int e) {

    return (e < ECount[z]);

}

/**********************************************************************************************************************/

void TransTable::E2YX(unsigned int z, unsigned int e, unsigned int &y, unsigned int &x) {

    unsigned int c2[_MAX_DIMS];
    unsigned int c3[_MAX_DIMS];
    unsigned int d;

    for (d = NDim[z] - 1; d >= 1; d--) {
        c2[d] = e / Siz2[z][d - 1];
        e -= c2[d] * Siz2[z][d - 1];
    }
    c2[0] = e;

    for (d = 0; d < NDim[z]; d++) {
        c3[d] = c2[Perm[z][d]];
    }

    d = NDim[z] - 1;

    x = c3[d];
    for (; IDim[z][d - 1] == 1; d--) {
        x = x * Siz3a[z][d - 1] + c3[d - 1];
    }

    y = c3[--d];
    for (; d >= 1; d--) {
        y = y * Siz3a[z][d - 1] + c3[d - 1];
    }

}

/**********************************************************************************************************************/

const unsigned short *Input16(const unsigned short *a, unsigned int y, unsigned int x, unsigned int s) {

    return a + (s * XCount + x) * YCount + y;

}

/**********************************************************************************************************************/

const unsigned int *Input32(const unsigned int *a, unsigned int y, unsigned int x, unsigned int s) {

    return a + (s * XCount + x) * YCount + y;

}

/**********************************************************************************************************************/

unsigned int *NeuronPtr(unsigned int y, unsigned int x) {

    return Neurons + x * YSize + y;

}

/**********************************************************************************************************************/

unsigned short *SynapsePtr(unsigned int y, unsigned int x, unsigned int s) {

    return Synapses + ((s * XCount + x) * YSize + y) * 4;

}

/**********************************************************************************************************************/

#include "common_def.h"
