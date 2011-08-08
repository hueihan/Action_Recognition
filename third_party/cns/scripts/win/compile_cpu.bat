@echo off

rem 1 = mex include path
rem 2 = setup filepath
rem 3 = input filepath
rem 4 = option
rem 5 = output filepath

call %2

if not exist %3 (
    echo can't find %3
    goto exit
)

if "%4"=="compile" (
    call mex -output %5 %3
) else if "%4"=="preprocess" (
    %SETUP_COMPILER%
    cl -E -I %1 %3 > %5
)

if exist %5 (
    echo CPU compilation successful
) else (
    echo CPU compilation failed
)

:exit
