@echo off
REM C++ Build script. To use, simply make adjustments to the compiler debug, release, common, and linker flags.
REM In particular, you may need to adjust the output executable name, include paths, and libraries.

REM Set build tool and library paths as well as compile flags here.

set debug_flags=/Od /Z7 /MTd
set release_flags=/O2 /GL /MT /analyze- /D NDEBUG
set common_flags=/W3 /Gm- /EHsc /nologo /Fe: ImageViewer.exe /I ..\..\src /I ..\..\ext ..\..\src\UnityBuild.cpp
REM set linker_flags=/INCREMENTAL:no /NOLOGO /SUBSYSTEM:WINDOWS user32.lib d3d11.lib dxgi.lib shell32.lib ole32.lib
set linker_flags=/INCREMENTAL:no /NOLOGO /SUBSYSTEM:WINDOWS d3d11.lib d3dcompiler.lib
REM Run the build tools, but only if they aren't set up already.

cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Running VS build tool setup.
echo Initializing MS build tools...
call scripts\setup_cl.bat
cl >nul 2>nul
if %errorlevel% neq 9009 goto :build
echo Unable to find build tools! Make sure that you have Microsoft Visual Studio 10 or above installed!
exit /b 1

REM Use the first command-line argument to set the build mode to debug or release (defaulting to debug).
REM If the build directory doesn't exist, create one.

:build
set mode=debug
if /i $%1 equ $release (set mode=release)
if %mode% equ debug (
set flags=%common_flags% %debug_flags%
) else (
set flags=%common_flags% %release_flags%
)
echo Building in %mode% mode.
if not exist bin\%mode% mkdir bin\%mode%
pushd bin\%mode%

REM Perform the actual build, skipping resource copy if it failed.

echo.     -Compiling:
call cl %flags% /link %linker_flags%
if %errorlevel% neq 0 (
echo Error during compilation!
popd
goto :fail
)
popd

REM Copy shaders, if they have changed.
REM if exist shaders (
REM echo.    -Copying Shaders:
REM if not exist bin\%mode%\shaders mkdir bin\%mode%\shaders
REM robocopy shaders bin\%mode%\shaders *.hlsl /s /purge /ns /nc /ndl /np /njh /njs
REM )

REM if exist assets (
REM echo.    Copying Assets:
REM if not exist bin\%mode%\assets mkdir bin\%mode%\assets
REM robocopy assets bin\%mode%\assets *.* /s /purge /ns /nc /ndl /np /njh /njs
REM )
REM If we made it here, the build was successful!

echo Build complete!
exit /b 0

REM Error state. Print failure message and exit.

:fail
echo Build failed!
exit /b %errorlevel%
