#TEMPLATE


FVALS_HANDLE hw = GET_FVALS_HANDLE;

VAL_HANDLE hv = GET_LAYER_VAL_HANDLE(PZ);
int f = THIS_F;
int rfCount = FVALS_HANDLE_Y_SIZE(hw);

int vy1, vy2, vx1, vx2, vt1, vt2, x1, x2, y1, y2, t1, t2;
GET_LAYER_T_RF_NEAR(PZ, TFCOUNT, vt1, vt2,t1, t2);
GET_LAYER_Y_RF_NEAR(PZ, rfCount, vy1, vy2, y1, y2);
GET_LAYER_X_RF_NEAR(PZ, rfCount, vx1, vx2, x1, x2);
if (vt1!=t1 || vt2!=t2 || vx1!=x1 || vx2!=x2 || vy1!=y1 || vy2!=y2 )
{     
  WRITE_VAL(CNS_FLTMIN);
    return;
}

float vsum,vssum;
float den, res;

#PART start

for (int k = 0, t = t1; t <= t2; k++, t++) {
    for (int j = 0, x = x1; x <= x2; j++, x++) {
        for (int i = 0, y = y1; y <= y2; i++, y++) {

            float w = READ_FVALS_HANDLE(hw, k, i, j, f);
            float v = READ_VAL_HANDLE(hv, 0, t, y, x);
	    if (v <0) {
	      res = CNS_FLTMIN;
	      goto done;
	    }
            #PART middle
        }
    }
}

#PART end
done:
WRITE_VAL(res);

