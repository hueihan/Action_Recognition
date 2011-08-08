#PART start

    res = 0.0f;
    float len  = 0.0f;
    float len2 = 0.0f;
   
#PART middle

    res  += w * v;
    len  += v;                                                                                                                                                                                                                                                                                                                              
    len2 += 1;
   
#PART end

    
    len /=len2;

if (len > 0.0f) res /= len;
    res = fabsf(res);
