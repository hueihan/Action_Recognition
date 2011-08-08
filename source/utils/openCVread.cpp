
#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include "mex.h"

#include "opencv/cv.h"
#include "opencv/cvaux.h"
#include "opencv/highgui.h"

using namespace std;

static CvCapture* capture = NULL;
static int H, W;

static void CloseStream(void)
{

  //mexPrintf("Closing video file\n");
  cvReleaseCapture(&capture);
}
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  
  IplImage  *tmp1, *tmp2, *tmp3;
  char *filename;

  if (nrhs != 1) {
    mexErrMsgTxt("One input argument required.");
  } 
  if (!(mxIsChar(prhs[0]))){
    mexErrMsgTxt("Input must be a fiename.\n.");
  }


   if (capture==NULL){
     /* copy the string data from prhs[0] into a C string.    */
     filename = mxArrayToString(prhs[0]);     
     capture = cvCaptureFromFile(filename);  
     //cvSetCaptureProperty(capture, CV_CAP_PROP_CONVERT_RGB,true);       

     if (capture == NULL){
       mexErrMsgTxt("Could not open video file.txt."); 
     }
     mexAtExit(CloseStream);
     //mexPrintf("Opening video file.\n");

     if (!cvGrabFrame(capture)){
       plhs[0] =   mxCreateDoubleMatrix(1,1,mxREAL);
       double *x = mxGetPr(plhs[0]);
       *x = 0;   
       return;  
     }
     tmp1 = (IplImage*)cvClone(cvRetrieveFrame(capture));           // retrieve the captured frame
  
       H    = tmp1->height;
       W    = tmp1->width;
   
   }
   else{
     if (!cvGrabFrame(capture)){
       plhs[0] =   mxCreateDoubleMatrix(1,1,mxREAL);       
       double *x = mxGetPr(plhs[0]);
       *x = 0;
       return;   
     }
     tmp1 = (IplImage*)cvClone(cvRetrieveFrame(capture));           // retrieve the captured frame
   }
   plhs[0] =   mxCreateNumericMatrix(H,W,mxUINT8_CLASS,mxREAL);
   tmp2 = cvCreateImage(cvSize(H, W),tmp1->depth,3);	
   tmp3 = cvCreateImage(cvSize(H, W),tmp1->depth,1);   
   cvTranspose(tmp1,tmp2);	
   cvCvtColor(tmp2, tmp3,CV_BGR2GRAY);         
   memcpy((uchar* )mxGetPr(plhs[0]),(uchar*)tmp3->imageData, H*W);     
   

   cvReleaseImage(&tmp1);  
   cvReleaseImage(&tmp2); 
   cvReleaseImage(&tmp3);  
 
}

   
