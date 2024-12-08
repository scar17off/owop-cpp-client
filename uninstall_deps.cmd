@echo off
echo Uninstalling OWOP Client dependencies...

REM Check if vcpkg is installed
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found! Please install vcpkg and set VCPKG_ROOT environment variable.
    exit /b 1
)

REM Remove all packages at once
echo Removing all dependencies...
"%VCPKG_ROOT%\vcpkg.exe" remove --triplet=x64-windows ^
    glfw3 ^
    opengl ^
    glad ^
    imgui[glfw-binding,opengl3-binding] ^
    websocketpp ^
    boost-asio

echo.
echo All dependencies uninstalled successfully!
echo.

pause 