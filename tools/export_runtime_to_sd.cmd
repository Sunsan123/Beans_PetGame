@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"
set "BUILD_SCRIPT=%REPO_ROOT%\Sprites\build_sprites.py"
set "THEME_PREVIEW_SCRIPT=%REPO_ROOT%\tools\build_dress_theme_rgb565.py"
set "RUNTIME_SOURCE=%REPO_ROOT%\assets\build\runtime"
set "THEME_SOURCE=%REPO_ROOT%\assets\src\shop\furniture"
for %%I in ("%REPO_ROOT%\assets") do set "REPO_ASSETS=%%~fI"
for %%I in ("%REPO_ROOT%\assets\src") do set "REPO_ASSETS_SRC=%%~fI"
for %%I in ("%REPO_ROOT%\assets\build") do set "REPO_ASSETS_BUILD=%%~fI"
for %%I in ("%REPO_ROOT%\assets\build\runtime") do set "REPO_RUNTIME_SOURCE=%%~fI"

echo.
echo Beans_PetGame runtime asset export
echo.

set "AUTO_DRIVE_COUNT=0"
set "AUTO_DRIVE_VALUE="
for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command "$ds=[System.IO.DriveInfo]::GetDrives(); foreach($d in $ds){ if($d.DriveType -eq 'Removable' -and $d.IsReady){ $d.Name } }"`) do (
  set /a AUTO_DRIVE_COUNT+=1
  set "AUTO_DRIVE_VALUE=%%D"
)

if "!AUTO_DRIVE_COUNT!"=="1" (
  echo Detected SD/removable drive: !AUTO_DRIVE_VALUE!
) else if not "!AUTO_DRIVE_COUNT!"=="0" (
  echo Detected multiple removable drives.
)

set "DEST_ROOT="
:prompt_dest
set /p "DEST_ROOT=Enter SD root or local export folder (blank uses detected SD): "

if not defined DEST_ROOT (
  if "!AUTO_DRIVE_COUNT!"=="1" (
    set "DEST_ROOT=!AUTO_DRIVE_VALUE!"
  ) else (
    echo No single SD drive could be auto-selected. Please enter a path like H:\ or C:\temp\sd_export
    goto prompt_dest
  )
)

set "DEST_ROOT=%DEST_ROOT:"=%"
for /f "tokens=* delims= " %%A in ("%DEST_ROOT%") do set "DEST_ROOT=%%~A"
if not "%DEST_ROOT:~-1%"=="\" set "DEST_ROOT=%DEST_ROOT%\"

set "DEST_BASE_ROOT=%DEST_ROOT%"
if /I "%DEST_ROOT:~-15%"=="assets\runtime\" set "DEST_BASE_ROOT=%DEST_ROOT:~0,-15%"

set "DEST_TARGET=%DEST_BASE_ROOT%assets\runtime"
set "DEST_THEME_TARGET=%DEST_BASE_ROOT%assets\src\shop\furniture"

for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%DEST_TARGET:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "DEST_TARGET_FULL=%%P"
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%REPO_ROOT:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "REPO_ROOT_FULL=%%P"
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%REPO_ASSETS:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "REPO_ASSETS_FULL=%%P"
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%REPO_ASSETS_SRC:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "REPO_ASSETS_SRC_FULL=%%P"
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%REPO_ASSETS_BUILD:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "REPO_ASSETS_BUILD_FULL=%%P"
for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$p='%REPO_RUNTIME_SOURCE:\=\\%'; [System.IO.Path]::GetFullPath($p).TrimEnd('\')"`) do set "REPO_RUNTIME_SOURCE_FULL=%%P"

echo Resolved export target: %DEST_TARGET_FULL%

if /I "%DEST_TARGET_FULL%"=="%REPO_ROOT_FULL%" (
  echo Refusing export: target resolves to repository root.
  goto :fail
)
if /I "%DEST_TARGET_FULL%"=="%REPO_ASSETS_FULL%" (
  echo Refusing export: target resolves to assets root.
  goto :fail
)
if /I "%DEST_TARGET_FULL%"=="%REPO_ASSETS_SRC_FULL%" (
  echo Refusing export: target resolves to assets\src.
  goto :fail
)
if /I "%DEST_TARGET_FULL%"=="%REPO_ASSETS_BUILD_FULL%" (
  echo Refusing export: target resolves to assets\build.
  goto :fail
)
if /I "%DEST_TARGET_FULL%"=="%REPO_RUNTIME_SOURCE_FULL%" (
  echo Refusing export: target resolves to the generated runtime source folder inside the repo.
  goto :fail
)

echo %DEST_TARGET_FULL% | findstr /I /B /C:"%REPO_ROOT_FULL%\\" >nul
if not errorlevel 1 (
  echo Refusing export: target is inside the repository tree.
  echo Use an SD root like H:\ or an external staging folder like C:\temp\sd_export
  goto :fail
)

echo.
echo [1/4] Building runtime bundles...
python "%BUILD_SCRIPT%"
if errorlevel 1 goto :fail

if not exist "%RUNTIME_SOURCE%\manifest.json" (
  echo Runtime bundles not found: %RUNTIME_SOURCE%
  goto :fail
)

echo [2/4] Exporting runtime bundles to %DEST_TARGET%
if exist "%DEST_TARGET%" rmdir /s /q "%DEST_TARGET%"
mkdir "%DEST_TARGET%" >nul 2>&1
robocopy "%RUNTIME_SOURCE%" "%DEST_TARGET%" /MIR /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
set "ROBO_EXIT=%ERRORLEVEL%"
if %ROBO_EXIT% GEQ 8 (
  echo Robocopy failed with code %ROBO_EXIT%.
  goto :fail
)

if not exist "%DEST_TARGET%\manifest.json" (
  echo Export verification failed: manifest.json missing.
  goto :fail
)
if not exist "%DEST_TARGET%\packs" (
  echo Export verification failed: packs folder missing.
  goto :fail
)
if not exist "%DEST_TARGET%\index" (
  echo Export verification failed: index folder missing.
  goto :fail
)

echo [3/5] Exporting dress theme source assets to %DEST_THEME_TARGET%
if exist "%THEME_SOURCE%" (
  mkdir "%DEST_THEME_TARGET%" >nul 2>&1
  robocopy "%THEME_SOURCE%" "%DEST_THEME_TARGET%" /MIR /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
  set "ROBO_EXIT=%ERRORLEVEL%"
  if %ROBO_EXIT% GEQ 8 (
    echo Theme export failed with code %ROBO_EXIT%.
    goto :fail
  )
)

if not exist "%DEST_THEME_TARGET%" (
  echo Export verification failed: theme folder missing.
  goto :fail
)

echo [4/5] Building RGB565 dress theme previews...
python "%THEME_PREVIEW_SCRIPT%" "%DEST_THEME_TARGET%"
if errorlevel 1 goto :fail

for /f %%C in ('dir /b /a-d "%DEST_TARGET%\packs\*.bpk" ^| find /c /v ""') do set "PACK_COUNT=%%C"
for /f %%C in ('dir /b /a-d "%DEST_TARGET%\index\*.bix" ^| find /c /v ""') do set "INDEX_COUNT=%%C"
for /f %%C in ('dir /s /b /a-d "%DEST_THEME_TARGET%\000_*.png" 2^>nul ^| find /c /v ""') do set "THEME_THUMB_COUNT=%%C"
for /f %%C in ('dir /s /b /a-d "%DEST_THEME_TARGET%\000_theme_preview.rgb565" 2^>nul ^| find /c /v ""') do set "THEME_RGB565_COUNT=%%C"

if "%THEME_THUMB_COUNT%"=="0" (
  echo Export verification failed: no theme preview images found.
  goto :fail
)
if "%THEME_RGB565_COUNT%"=="0" (
  echo Export verification failed: no RGB565 theme previews were generated.
  goto :fail
)

echo [5/5] Export complete.
echo Target: %DEST_TARGET%
echo Packs:  %PACK_COUNT%
echo Index:  %INDEX_COUNT%
echo Theme previews: %THEME_THUMB_COUNT%
echo RGB565 previews: %THEME_RGB565_COUNT%
echo.
pause
exit /b 0

:fail
echo.
echo Export failed.
echo.
pause
exit /b 1
