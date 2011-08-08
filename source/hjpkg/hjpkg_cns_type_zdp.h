
#PART start

    res   = 0.0f; 
    vsum  = 0.0f;
vssum = 0.0f;
int n = rfCount*rfCount*TFCOUNT;

#PART middle

     res += w * v;
   vsum += v;
  vssum += v * v;

#PART end

den = sqrtf(vssum-vsum*vsum/(float)n);
den = fmaxf(den, THRES*sqrtf((float)n));
res = fabsf(res/den);

