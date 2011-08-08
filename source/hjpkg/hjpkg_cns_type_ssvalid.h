#TEMPLATE

int nf = THIS_F;
int t  = THIS_T;
int rfCount = READ_FSIZES(nf, 0);
int rfSpace = RFSPACE;
int rfWidth = 1 + (rfCount - 1) * rfSpace;

int vy1, vy2, vx1, vx2, y1, y2, x1, x2;

GET_LAYER_Y_RF_NEAR(PZ, rfWidth, vy1, vy2, y1, y2);
GET_LAYER_X_RF_NEAR(PZ, rfWidth, vx1, vx2, x1, x2);
if (vx1!=x1 || vx2!=x2 || vy1!=y1 || vy2!=y2 )
{     
  WRITE_VAL(CNS_FLTMIN);
    return;
}


FVALS_HANDLE hw = GET_FVALS_HANDLE;
FMAP2_HANDLE hm = GET_FMAP2_HANDLE;
int pCount = READ_FMAP2_HANDLE(hm, 0, nf);
VAL_HANDLE hv = GET_LAYER_VAL_HANDLE(PZ);

float res;

#PART start

for (int p = 0; p < pCount; p++) {

    float w = READ_FVALS_HANDLE(hw, p    , nf);
    int   m = READ_FMAP2_HANDLE(hm, p + 1, nf);

    int f =   m & 0x0000FFFF;
    int y = ((m & 0x000F0000) >> 16)  + y1;
    int x = ((m & 0x00F00000) >> 20)  + x1;

    float v = READ_VAL_HANDLE(hv, f, t, y, x);
    if (v == CNS_FLTMIN) {
        res = CNS_FLTMIN;
        goto done;
    }

    #PART middle

}

#PART end

done:
WRITE_VAL(res);
