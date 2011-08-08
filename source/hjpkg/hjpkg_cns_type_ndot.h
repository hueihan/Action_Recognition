#PART start

    res = 0.0f;
   
    float len = 0.0f;
    float len1 = 0.0f;
#PART middle

    len += v * v;
    len1 += w * w;
    res += v * w;

#PART end

if ((len>0.0f) && (len1>0.0f)){
    res  /= sqrtf(len+THRES);
    res /= sqrtf(len1+THRES);
}
if (res<0){
  res = 0;
}
res = pow(res,EXPONENT);
