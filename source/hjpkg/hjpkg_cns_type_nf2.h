int f = THIS_F;
int t = THIS_T;
int y = THIS_Y;
int x = THIS_X;


float v = READ_LAYER_VAL(PZ, f, t, y, x);
float n = READ_LAYER_VAL(SZ, 0, t, y, x);


if (v == CNS_FLTMIN) 
  goto done;
if (n == CNS_FLTMIN) 
  goto done;


  v *= v;
if (n > 0) v /= n;

done:
WRITE_VAL(v);
