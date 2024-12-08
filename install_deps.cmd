@echo off
echo Installing OWOP Client dependencies using vcpkg...

REM Check if vcpkg is installed
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found! Please install vcpkg and set VCPKG_ROOT environment variable.
    exit /b 1
)

REM Install required packages
echo Installing dependencies...
"%VCPKG_ROOT%\vcpkg.exe" install --triplet=x64-windows ^
    glfw3 ^
    opengl ^
    glad ^
    imgui[glfw-binding,opengl3-binding] ^
    websocketpp ^
    boost-random ^
    boost-asio ^
    boost-beast ^
    openssl ^
    webview2 ^
    wil ^
    glm

echo.
echo All dependencies installed successfully!
echo Please integrate vcpkg with Visual Studio by running: vcpkg integrate install
echo.

pause 