int fc = LAYER_F_SIZE(PZ);
int t = THIS_T;
int y = THIS_Y;
int x = THIS_X;

VAL_HANDLE h = GET_LAYER_VAL_HANDLE(PZ);

float res = 0.0f;

for (int f = 0; f < fc; f++) {

    float v = READ_VAL_HANDLE(h, f, t, y, x);
    if (v == CNS_FLTMIN){
      res =  CNS_FLTMIN;
      goto done;
    }
    res += v * v;

}


res += GAMMA;
done:
WRITE_VAL(res);
