% adjust the opencv_path to your opencv directory

opencv_path = '/gpfs/runtime/opt/opencv/2.2.0/'

openCVflags = ['-I' opencv_path 'include/ -L' opencv_path 'lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc'];


system(['mex  -outdir bin ' openCVflags ' source/utils/get_video_info.cpp']);
system(['mex  -outdir bin ' openCVflags ' source/utils/openCVread.cpp']);
system(['mex  -outdir bin  source/utils/to_svm_light.c']);
      
