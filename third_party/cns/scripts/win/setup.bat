@echo off

set           CUDA=1
set SETUP_COMPILER=call "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
set   NVCC_OPTIONS=-D_CRT_SECURE_NO_DEPRECATE
set  CUDA_LINK_LIB=C:\CUDA\lib\cudart.lib
