@echo off
setlocal

:: %~dp0 gets the folder containing this script (includes a trailing slash)
set "ROOT_DIR=%~dp0"
:: Strip the trailing slash for cleaner concatenation
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%

:: Expose CLion's bundled MinGW compiler to this script
set "PATH=C:\Program Files\JetBrains\CLion 2026.1.1\bin\mingw\bin;%PATH%"

set "EXTERNAL_DIR=%ROOT_DIR%\external"
set "INTERMEDIATE_DIR=%ROOT_DIR%\intermediate"
set "BINARIES_DIR=%ROOT_DIR%\binaries"

echo =========================
echo      Shaderc Build
echo =========================
echo Project Root: %ROOT_DIR%

:: 1. Run python script in shaderc
echo.
echo [1/3] Syncing dependencies (requires Python)
pushd "%EXTERNAL_DIR%\shaderc"
python utils\git-sync-deps
popd

:: 2. Configure and build shaderc to intermediate
echo.
echo [2/3] Building shaderc_combined
:: -S points to the source code, -B points to where the build files should go
::cmake -S "%EXTERNAL_DIR%\shaderc" -B "%INTERMEDIATE_DIR%\shaderc" -DCMAKE_BUILD_TYPE=Release -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON
::cmake --build "%INTERMEDIATE_DIR%\shaderc" --config Release --target shaderc_combined

cmake -S "%EXTERNAL_DIR%\shaderc" -B "%INTERMEDIATE_DIR%\shaderc" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON
cmake --build "%INTERMEDIATE_DIR%\shaderc" --config Release --target shaderc_combined

:: 3. Copy the binary out
echo.
echo [3/3] Copying static library to binaries folder
if not exist "%BINARIES_DIR%\shaderc" mkdir "%BINARIES_DIR%\shaderc"
copy /Y "%INTERMEDIATE_DIR%\shaderc\libshaderc\Release\shaderc_combined.lib" "%BINARIES_DIR%\shaderc\libshaderc_combined.a"

echo.
echo =========================
echo  Shaderc Setup Complete
echo =========================
pause
