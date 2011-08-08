@echo off

rem 1 = mex include path
rem 2 = setup filepath
rem 3 = input filepath
rem 4 = option
rem 5 = intermediate filepath
rem 6 = output filepath

call %2

if not "%CUDA%"=="1" goto exit

if not exist %3 (
    echo can't find %3
    goto exit
)

%SETUP_COMPILER%

if "%4"=="compile" (
    nvcc -cuda -I %1 %NVCC_OPTIONS% -use_fast_math -o %5 %3
) else if "%4"=="preprocess" (
    nvcc -E -I %1 %NVCC_OPTIONS% -use_fast_math -o %5 %3
) else if "%4"=="info" (
    nvcc -cubin -I %1 %NVCC_OPTIONS% -use_fast_math -o %5 %3
)

if exist %5 (
    echo CUDA compilation successful
) else (
    echo CUDA compilation failed
    goto exit
)

if "%4"=="compile" (
    call mex -output %6 %5 %CUDA_LINK_LIB%
) else (
    goto exit
)

if exist %6 (
    echo MEX compilation of CUDA output successful
) else (
    echo MEX compilation of CUDA output failed
)

:exit
