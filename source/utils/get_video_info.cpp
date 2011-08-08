
#include<iostream>
//#include<cv.h>
//#include<cvaux.h>
//#include<highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include "mex.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  CvCapture* capture = NULL;
  char *filename = mxArrayToString(prhs[0]);    
  double *x, *y, *z;

  capture = cvCaptureFromFile(filename);
  if (capture == NULL){
    mexErrMsgTxt("Could not open video file.txt."); 
  }

  y = mxGetPr( plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL) );
  *y = cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT );         
  x = mxGetPr( plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL) );
  *x = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);       
  z = mxGetPr( plhs[2] = mxCreateDoubleMatrix(1,1,mxREAL) );
  *z = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);       

  cvReleaseCapture(&capture);
  mxFree(filename);
}

