#TEMPLATE


int vy1, vy2, vx1, vx2, vt1, vt2, y1, y2, x1, x2, t1, t2;
int fc = THIS_F;

float res;

#PART start


for (int is = 0; is< RSCOUNT; is++){
  GET_LAYER_T_RF_NEAR(SZ(is), 1, vt1, vt2,t1, t2);
  GET_LAYER_Y_RF_NEAR(SZ(is), RFCOUNT, vy1, vy2, y1, y2);
  GET_LAYER_X_RF_NEAR(SZ(is), RFCOUNT, vx1, vx2, x1, x2);
  if (vt1!=t1 || vt2!=t2 )//|| vx1!=x1 || vx2!=x2 || vy1!=y1 || vy2!=y2 )
  {     
    WRITE_VAL(CNS_FLTMIN);
    return;
  }

  VAL_HANDLE h = GET_LAYER_VAL_HANDLE(SZ(is));
 


  for (int t = t1; t<= t2; t++) {
    for (int x = vx1; x <= vx2; x++) {
      for (int y = vy1; y <= vy2; y++) {
	
	float v = READ_VAL_HANDLE(h, fc, t, y, x);
	if (v == CNS_FLTMIN & SPECIALMIN == 1) {	  
	  res = CNS_FLTMIN;
	  goto done;
	}
       #PART middle
      }
    }
  }
}

#PART end
done:
WRITE_VAL(res);
