@ECHO OFF
CLS
call "%VS100COMNTOOLS%..\..\VC\bin\vcvars32.bat"
IF EXIST %WORKSPACE%\build rmdir %WORKSPACE%\build /S /Q
mkdir %WORKSPACE%\build
cd %WORKSPACE%\build
cmake -G "NMake Makefiles" -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release -DXBMC_BINDINGS=%XBMC_BINDINGS% ..
nmake
nmake package