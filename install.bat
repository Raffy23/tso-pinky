@echo OFF
setlocal ENABLEEXTENSIONS
:::
:::  _______     _              __                  
::: |_   __ \   (_)            [  |  _              
:::   | |__) |  __    _ .--.    | | / ]    _   __   
:::   |  ___/  [  |  [ `.-. |   | '' <    [ \ [  ]  
:::  _| |_      | |   | | | |   | |`\ \    \ '/ /   
::: |_____|    [___] [___||__] [__|  \_] [\_:  /    
:::                                       \__.'     
:::
:::               Installer v1.0                 
:::
for /f "delims=: tokens=*" %%A in ('findstr /b ::: "%~f0"') do @echo(%%A

for /f "tokens=3*" %%a in ('reg query "HKEY_CLASSES_ROOT\tso\DefaultIcon"') do set REGISTRY_ENTRY=%%a %%b
for %%A in ("%REGISTRY_ENTRY%") do (
    set InstallationFolder=%%~dpA
    set ExecutableName=%%~nxA
)

echo Installation folder is: %InstallationFolder%

move "%REGISTRY_ENTRY%" "%InstallationFolder%The Settlers Online.original.exe"

xcopy /y Pinky.dll "%InstallationFolder%"
xcopy /y PinkyLoader.exe "%InstallationFolder%"
move "%InstallationFolder%PinkyLoader.exe" "%REGISTRY_ENTRY%" 1>NUL

set CONFIG_FILE="%InstallationFolder%Pinky.json"

set CONFIG_BINARY="%InstallationFolder%The Settlers Online.original.exe"
set CONFIG_BINARY=%CONFIG_BINARY:\=\\%

set CONFIG_CWD=%InstallationFolder:\=\\%

echo { >%CONFIG_FILE%
echo     "binary": %CONFIG_BINARY%,>>%CONFIG_FILE%
echo     "cwd": "%CONFIG_CWD%">>%CONFIG_FILE%
echo } >>%CONFIG_FILE%

echo Done
pause

