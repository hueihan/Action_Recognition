#include "common_dec.h"

/**********************************************************************************************************************/

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

    unsigned int mode = (unsigned int)mxGetScalar(prhs[0]);

    float val;
    switch (mode) {
    case 0: val = CNS_INTMIN; break;
    case 1: val = CNS_INTMAX; break;
    case 2: val = CNS_FLTMIN; break;
    case 3: val = CNS_FLTMAX; break;
    }

    plhs[0] = mxCreateDoubleScalar((double)val);

}
