REM @echo off

Echo LIB Windows Build NuGet

REM # XEON win32 Build Vars #
set _SCRIPT_DRIVE=%~d0
set _SCRIPT_FOLDER=%~dp0
set INITDIR=%CD%
set SRC=%INITDIR%\..\..\
set BUILDTREE=%SRC%\build-win\
SET tbs_arch=x86
SET vcvar_arg=x86
SET cmake_platform="Visual Studio 15 2017"

REM # VC Vars #
SET VCVAR="%programfiles(x86)%\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if exist %VCVAR% call %VCVAR% %vcvar_arg%
SET VCVAR="%programfiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
if exist %VCVAR% call %VCVAR% %vcvar_arg%

REM # Clean Build Tree #
rd /s /q %BUILDTREE%
mkdir %BUILDTREE%
mkdir %BUILDTREE%\deps
cd %BUILDTREE%

:nuget_Dep
REM # packages from nuget #
cd %BUILDTREE%\deps
SET OGGVER=1.3.2.8787
nuget install ogg-msvc-%tbs_arch% -Version %OGGVER%

:copy_files
set BINDIR=%SRC%\build-nuget\
rd /s /q %BINDIR%
mkdir %BINDIR%

:static_LIB
REM # LIB STATIC #
ECHO %cmake_platform% STATIC

rd /s /q %BUILDTREE%\vorbis
mkdir %BUILDTREE%\vorbis
cd %BUILDTREE%\vorbis
cmake -G %cmake_platform% ^
-DBUILD_SHARED_LIBS:BOOL=OFF ^
-DCMAKE_CXX_FLAGS_RELEASE="/MD" ^
-DCMAKE_CXX_FLAGS_DEBUG="/MDd" ^
-DCMAKE_C_FLAGS_RELEASE="/MD" ^
-DCMAKE_C_FLAGS_DEBUG="/MDd" ^
-DOGG_LIBRARIES=%BUILDTREE%\deps\ogg-msvc-%tbs_arch%.%OGGVER%\build\native\lib_release\ogg.lib ^
-DOGG_INCLUDE_DIRS=%BUILDTREE%\deps\ogg-msvc-%tbs_arch%.%OGGVER%\build\native\include ^
-DCMAKE_INSTALL_PREFIX=%BINDIR% ^
-DCMAKE_BUILD_TYPE="Release" %SRC%
cmake --build . --config Release --target install

move %BINDIR%lib %BINDIR%lib_release
move %BINDIR%bin %BINDIR%bin_release

REM # Clean Build Tree #
rd /s /q %BUILDTREE%\vorbis

REM # DEBUG #
rd /s /q %BUILDTREE%\vorbis
mkdir %BUILDTREE%\vorbis
cd %BUILDTREE%\vorbis
cmake -G %cmake_platform% ^
-DBUILD_SHARED_LIBS:BOOL=OFF ^
-DCMAKE_CXX_FLAGS_RELEASE="/MD" ^
-DCMAKE_CXX_FLAGS_DEBUG="/MDd" ^
-DCMAKE_C_FLAGS_RELEASE="/MD" ^
-DCMAKE_C_FLAGS_DEBUG="/MDd" ^
-DOGG_LIBRARIES=%BUILDTREE%\deps\ogg-msvc-%tbs_arch%.%OGGVER%\build\native\lib_debug\ogg.lib ^
-DOGG_INCLUDE_DIRS=%BUILDTREE%\deps\ogg-msvc-%tbs_arch%.%OGGVER%\build\native\include ^
-DCMAKE_INSTALL_PREFIX=%BINDIR% ^
-DCMAKE_BUILD_TYPE="DEBUG" %SRC%
cmake --build . --config DEBUG --target install

move %BINDIR%lib %BINDIR%lib_debug
move %BINDIR%bin %BINDIR%bin_debug

REM # TODO: ENABLE SHARED Build
GOTO:nuget_req
mkdir %BINDIR%\static\
move /Y %BINDIR%\lib %BINDIR%\static\

:shared_LIB
REM # LIB SHARED #
ECHO %cmake_platform% SHARED


:nuget_req
REM # make nuget packages from binaries #
copy %INITDIR%\vorbis-msvc-%tbs_arch%.targets %BINDIR%\vorbis-msvc-%tbs_arch%.targets
cd %BUILDTREE%
nuget pack %INITDIR%\vorbis-msvc-%tbs_arch%.nuspec
cd %INITDIR%
REM --- exit ----
GOTO:eof
