@echo off
setlocal

:: %~dp0 gets the folder containing this script (includes a trailing slash)
set "ROOT_DIR=%~dp0"
:: Strip the trailing slash for cleaner concatenation
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%

:: Expose CLion's bundled MinGW compiler to this script
set "PATH=C:\Program Files\JetBrains\CLion 2026.1.1\bin\mingw\bin;%PATH%"

:: Toolchain selection
::   Usage: setupshaderc.bat [msvc|clang-msvc|mingw|clang-mingw]
::   Default: mingw
set "TOOLCHAIN=%~1"
if "%TOOLCHAIN%"=="" set "TOOLCHAIN=mingw"

set "EXTERNAL_DIR=%ROOT_DIR%\external"
set "BINARIES_DIR=%ROOT_DIR%\binaries"

set "INTERMEDIATE_DIR=%ROOT_DIR%\intermediate\shaderc-%TOOLCHAIN%"
set "OUTPUT_DIR=%BINARIES_DIR%\shaderc\%TOOLCHAIN%"

set "CMAKE_COMMON_ARGS=-G "Ninja" -DCMAKE_BUILD_TYPE=Release -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON"

if /I "%TOOLCHAIN%"=="msvc" (
    set "OUTPUT_LIB=shaderc_combined.lib"
    set "SRC_LIB_NAME=shaderc_combined.lib"
    set "CMAKE_TOOLCHAIN_ARGS="

) else if /I "%TOOLCHAIN%"=="clang-msvc" (
    set "OUTPUT_LIB=shaderc_combined.lib"
    set "SRC_LIB_NAME=shaderc_combined.lib"
    set "CMAKE_TOOLCHAIN_ARGS=-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl"

) else if /I "%TOOLCHAIN%"=="mingw" (
    :: Expose CLion's bundled MinGW compiler to this script.
    :: Change this if you use a different MinGW installation.
    set "PATH=C:\Program Files\JetBrains\CLion 2026.1.1\bin\mingw\bin;%PATH%"
    set "OUTPUT_LIB=libshaderc_combined.a"
    set "SRC_LIB_NAME=libshaderc_combined.a"
    set "CMAKE_TOOLCHAIN_ARGS=-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"

) else if /I "%TOOLCHAIN%"=="clang-mingw" (
    :: Expose MinGW tools if your clang++ uses the MinGW runtime/linker.
    :: Change this if your MinGW installation is elsewhere.
    set "PATH=C:\Program Files\JetBrains\CLion 2026.1.1\bin\mingw\bin;%PATH%"
    set "OUTPUT_LIB=libshaderc_combined.a"
    set "SRC_LIB_NAME=libshaderc_combined.a"
    set "CMAKE_TOOLCHAIN_ARGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"

) else (
    echo Unknown toolchain "%TOOLCHAIN%".
    echo Use one of:
    echo   msvc
    echo   clang-msvc
    echo   mingw
    echo   clang-mingw
    exit /b 1
)

echo =========================
echo      Shaderc Build
echo =========================
echo Project Root: %ROOT_DIR%
echo Toolchain:    %TOOLCHAIN%
echo Output:       %OUTPUT_DIR%\%OUTPUT_LIB%

:: 1. Run python script in shaderc
echo.
echo [1/3] Syncing dependencies (requires Python)
pushd "%EXTERNAL_DIR%\shaderc"
python utils\git-sync-deps
if errorlevel 1 goto :fail
popd

:: 2. Configure and build shaderc to intermediate
echo.
echo [2/3] Building shaderc_combined (%TOOLCHAIN%)
cmake -S "%EXTERNAL_DIR%\shaderc" -B "%INTERMEDIATE_DIR%" %CMAKE_COMMON_ARGS% %CMAKE_TOOLCHAIN_ARGS%
if errorlevel 1 goto :fail

cmake --build "%INTERMEDIATE_DIR%" --config Release --target shaderc_combined
if errorlevel 1 goto :fail

:: 3. Copy the binary out
echo.
echo [3/3] Copying static library to binaries folder
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
copy /Y "%INTERMEDIATE_DIR%\libshaderc\%SRC_LIB_NAME%" "%OUTPUT_DIR%\%OUTPUT_LIB%"
if errorlevel 1 goto :fail

echo.
echo =========================
echo  Shaderc Setup Complete
echo =========================
pause

:fail
echo.
echo =========================
echo  Shaderc Setup FAILED
echo =========================
pause
