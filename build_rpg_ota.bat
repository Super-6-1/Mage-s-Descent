@echo off
REM Build mages_descent RPG OTA App for Windows

echo ===================================
echo Building mages_descent RPG OTA App
echo ===================================

cd Code\PocketMage_V3

REM Clean previous build
echo Cleaning previous build...
pio run -e OTA_APP -t clean

REM Build OTA app
echo Building OTA app...
pio run -e OTA_APP

REM Check if build succeeded
if exist .pio\build\OTA_APP\firmware.bin (
  echo.
  echo Build successful!
  echo Firmware: .pio\build\OTA_APP\firmware.bin

  REM Show firmware size
  for %%A in (.pio\build\OTA_APP\firmware.bin) do echo Firmware size: %%~zA bytes

  REM Create package directory
  if exist .pio\build\OTA_APP\package rmdir /s /q .pio\build\OTA_APP\package
  mkdir .pio\build\OTA_APP\package

  REM Copy firmware with app name
  copy .pio\build\OTA_APP\firmware.bin .pio\build\OTA_APP\package\mages_descent.bin >nul

  REM Copy icon if it exists
  if exist ..\..\assets\mages_descent_ICON.bin (
    copy ..\..\assets\mages_descent_ICON.bin .pio\build\OTA_APP\package\mages_descent_ICON.bin >nul
    echo Icon copied: mages_descent_ICON.bin
  ) else (
    echo Warning: No icon found at assets\mages_descent_ICON.bin, app will use default icon
  )

  REM Create tar package
  echo Creating TAR package...
  cd .pio\build\OTA_APP\package
  tar -cf ..\mages_descent.tar * 2>nul
  if errorlevel 1 (
    echo Error: tar command failed. Make sure tar is available in PATH.
    cd ..\..\..\..
    goto done
  )
  cd ..\..\..\..

  echo.
  echo Package created: .pio\build\OTA_APP\mages_descent.tar
  echo.
  echo Next steps:
  echo 1. Copy .pio\build\OTA_APP\mages_descent.tar to your SD card's /apps/ folder
  echo 2. Copy assets\sd_card_files\rpg\ folder to your SD card root
  echo 3. Use App Loader on Pocket Mage to install
  echo.
) else (
  echo Build failed! Check the error messages above.
  pause
  exit /b 1
)

:done
echo Done!
pause
exit /b 0
