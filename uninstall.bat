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
echo Removing installed files ...

del "%InstallationFolder%Pinky.dll"
del "%InstallationFolder%Pinky.json"
del "%InstallationFolder%The Settlers Online.exe"

move "%InstallationFolder%The Settlers Online.original.exe" "%REGISTRY_ENTRY%"
echo Done
pause