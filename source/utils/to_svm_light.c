/*
  convert the matlab matrix to SVMlight format
  2008/09/19
  Hueihan Jhuang
 */
#include <stdio.h>
#include <string.h>
#include "mex.h"



void mexFunction( int nlhs,       mxArray *plhs[],
		  int nrhs, const mxArray *prhs[] )
{

  
  double *values;
  double *labels;
  char *fname;
  char mode[80];
  int h,w;
  int i,j;
  FILE* fID; 
  int l;
  double *id;

  l              = mxGetN(prhs[0])+1;
  fname          = mxCalloc(l,sizeof(char));
  mxGetString(prhs[0],fname,l);

  values = mxGetPr(prhs[1]);
  h      = mxGetN(prhs[1]);
  w      = mxGetM(prhs[1]);
  labels = mxGetPr(prhs[2]);
  id     = mxGetPr(prhs[3]);
  mxGetString(prhs[4],mode,l);
  if (strcmp(mode,"append")==0)
    fID    = fopen(fname, "a");
  else 
    if (strcmp(mode,"write")==0)
      fID    = fopen(fname, "w");
    else{
      mexPrintf("invalid mode.\n");    
      return;
    }
  for(i=0;i<h;i++)
    {
      fprintf(fID,"%d",(int)labels[i]);
      if(id[0]>0)
	{
	  fprintf(fID," qid:%d",(int)id[0]);
	}
      for(j=0;j<w;j++)
	{
	  fprintf(fID," %d:%f",j+1,values[i*w+j]);
	}
      fprintf(fID,"\n");
    }
  
  fclose(fID);
  return;
}

