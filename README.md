Clone via:
* git clone --recurse-submodules https://github.com/logixoul/shade-awesome

Steps to build on Windows:
* winget install -e --id Ninja-build.Ninja
* in the vcpkg dir, run `bootstrap-vcpkg.bat`
* cmake --preset=windows

Steps to build on Ubuntu:
* sudo apt update
* sudo apt install curl zip unzip tar pkg-config build-essential ninja-build git # for vcpkg
* sudo apt install libgl1-mesa-dev xorg-dev libglu1-mesa-dev xorg-dev libxinerama-dev libxcursor-dev # for opengl and glfw
* sudo apt install autoconf autoconf-archive automake libtool # for alsa (for PortAudio)
* in the vcpkg dir, run `bootstrap-vcpkg.sh`
* cmake --preset=linux
* cmake --build --preset=linux-release

Packages needed on Fedora:
* sudo dnf install curl zip unzip tar pkgconf-pkg-config gcc-c++ make ninja-build git # for vcpkg
* sudo dnf install mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXxf86vm-devel libXext-devel libXrender-devel libXfixes-devel libXau-devel libXdmcp-devel libxkbcommon-devel glu-devel # for opengl and glfw
* sudo dnf install autoconf autoconf-archive automake libtool alsa-lib-devel # for alsa (for PortAudio)
