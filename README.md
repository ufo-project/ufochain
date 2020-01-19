# How to build

### Windows

1. Install **cmake>=3.13.0** (https://cmake.org/download)


2. Install **Visual Studio >= 2017** (https://www.visualstudio.com/vs/), **Visual Studio 2019** is strongly recommended.


3. Download and install **Boost** prebuilt binaries https://sourceforge.net/projects/boost/files/boost-binaries/1.68.0/boost_1_68_0-msvc-14.1-64.exe, also add ```BOOST_ROOT``` to the Environment Variables.


4. Download and install **OpenSSL** prebuilt binaries https://slproweb.com/products/Win32OpenSSL.html (Win64 OpenSSL v1.1.0h for example) and add ```OPENSSL_ROOT_DIR``` to the Environment Variables.


5. Download and install **QT 5.11** https://download.qt.io/official_releases/qt/5.11/5.11.0/qt-opensource-windows-x86-5.11.0.exe.mirrorlist and add ```QT5_ROOT_DIR``` to the Environment Variables (usually it looks like .../5.11.0/msvc2017_64), also add ```QML_IMPORT_PATH``` (it should look like ```%QT5_ROOT_DIR%\qml```). BTW disabling system antivirus on Windows makes QT installing process much faster.


6. Add .../qt511/5.11.1/msvc2017_64/bin and .../boost_1_68_0/lib64-msvc-14.1 to the System Path.


7. ```git clone https://github.com/ufo-project/ufochain.git```


8. Cmake-gui -> Set Project Folder -> Configure -> Generate -> Open Project -> Build


**Btw**:
If you want to get a static release build, change ```set(UFO_USE_STATIC FALSE)``` to ```set(UFO_USE_STATIC TRUE)``` in CMakeList.txt, and use QT static lib (https://github.com/ufo-project/qt5-static-win)  to replace Qt dynamic lib in step 5.


### Ubuntu 18.04

1. Install dependencies

```sudo add-apt-repository ppa:beineri/opt-qt-5.11.0-bionic
sudo add-apt-repository ppa:mhier/libboost-latest
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install g++-8 libssl-dev curl wget git make
sudo apt-get install libgl1-mesa-dev zlib1g-dev libboost1.67-dev
sudo apt-get install qt511base qt511declarative qt511svg qt511tools```
	

2. Install cmake

```wget https://cmake.org/files/v3.13/cmake-3.13.0-Linux-x86_64.sh
sudo sh ./cmake-3.13.0-Linux-x86_64.sh --skip-license --prefix=/usr```


3. Go to the Ufo project folder and start the release build

```export PATH=/opt/qt511/bin:$PATH && export CC=gcc-8 && export CXX=g++-8
cmake -DCMAKE_BUILD_TYPE=Release && make -j4```
