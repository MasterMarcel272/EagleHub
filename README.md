# EagleHub

EagleHub is the desktop launcher for Eagle Studio applications. It can download, install, update, launch, and uninstall Feather Photo.

## Windows installer

Download `EagleHub-Setup.exe` from the latest GitHub Release and run it. EagleHub installs for the current Windows user and includes the Qt runtime needed to start the application.

## Build from source

Requirements: Visual Studio 2022 with the Desktop development with C++ workload, CMake 3.21 or newer, and Qt 6 Widgets and Network for MSVC x64.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DQt6_DIR="C:/Qt/6.x/msvc2022_64/lib/cmake/Qt6"
cmake --build build --config Release
```

The GitHub Actions workflow builds the standalone Windows installer when a version tag such as `v0.1.0` is pushed.
