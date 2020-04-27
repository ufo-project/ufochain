# How to build

### Windows

1. Install **cmake>=3.13.0** (https://cmake.org/download).


2. Install **Visual Studio >= 2017** (https://www.visualstudio.com/vs/), and **Visual Studio 2019** is strongly recommended.


3. Download and install **Boost** prebuilt binaries https://sourceforge.net/projects/boost/files/boost-binaries/1.68.0/boost_1_68_0-msvc-14.1-64.exe, also add ```BOOST_ROOT``` to the Environment Variables.


4. Download and install **OpenSSL** prebuilt binaries https://slproweb.com/products/Win32OpenSSL.html (Win64 OpenSSL v1.1.0h for example) and add ```OPENSSL_ROOT_DIR``` to the Environment Variables.


5. Download and install **QT 5.11** https://download.qt.io/official_releases/qt/5.11/5.11.0/qt-opensource-windows-x86-5.11.0.exe.mirrorlist and add ```QT5_ROOT_DIR``` to the Environment Variables (usually it looks like .../5.11.0/msvc2017_64), also add ```QML_IMPORT_PATH``` (it should look like ```%QT5_ROOT_DIR%\qml```). BTW disabling system antivirus on Windows makes QT installing process much faster.


6. Add .../qt511/5.11.1/msvc2017_64/bin and .../boost_1_68_0/lib64-msvc-14.1 to the System Path.


7. Fetch the ufochain project

```
git clone https://github.com/ufo-project/ufochain.git
```


8. Cmake-gui -> Set Project Folder -> Configure -> Generate -> Open Project -> Build


* **Btw**:
If you want to get a static release build, change line 131 ```set(UFO_USE_STATIC FALSE)``` to ```set(UFO_USE_STATIC TRUE)``` at CMakeList.txt, and use QT static lib (https://github.com/ufo-project/qt5-static-win)  to replace Qt dynamic lib in step 5.


### Ubuntu 18.04

1. Upgrade gcc and libstdc++

```
sudo apt update
sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa
sudo apt update
sudo apt install wget gcc-8 unzip libssl1.0.0 software-properties-common
sudo apt-get install --only-upgrade libstdc++6
```


2. Install dependencies

```
sudo add-apt-repository ppa:beineri/opt-qt-5.11.0-bionic
sudo add-apt-repository ppa:mhier/libboost-latest
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install g++-8 libssl-dev curl wget git make
sudo apt-get install libgl1-mesa-dev zlib1g-dev libboost1.67-dev
sudo apt-get install qt511base qt511declarative qt511svg qt511tools
```
	

3. Install cmake

```
wget https://cmake.org/files/v3.13/cmake-3.13.0-Linux-x86_64.sh
sudo sh ./cmake-3.13.0-Linux-x86_64.sh --skip-license --prefix=/usr
```


4. Fetch the ufochain project

```
git clone https://github.com/ufo-project/ufochain.git
```


5. Go to the Ufo project folder and start the release build

```
export PATH=/opt/qt511/bin:$PATH && export CC=gcc-8 && export CXX=g++-8
cmake -DCMAKE_BUILD_TYPE=Release && make -j4
```

* **Btw**:
If you want to get a static link with NO-QT-GUI release build

1.  Get OpenSSL-1.1.0k and build static libraries

```
wget https://www.openssl.org/source/old/1.1.0/openssl-1.1.0k.tar.gz
tar xvfz openssl-1.1.0k.tar.gz
cd openssl-1.1.0k
./config 
make -j4
```

2.  Get Boost-1.67.0 and build static libraries

```
wget https://iweb.dl.sourceforge.net/project/boost/boost/1.67.0/boost_1_67_0.zip
unzip boost_1_67_0.zip
cd boost_1_67_0
sh bootstrap.sh
./b2 link=static
```

3.  Modify and change line 131 ```set(UFO_USE_STATIC FALSE)``` to ```set(UFO_USE_STATIC TRUE)``` at CMakeList.txt, and then
add two lines in line 14 just as 

```
set(OPENSSL_ROOT_DIR "<your openssl source file dir>")
set(BOOST_ROOT "<your boost source file dir>")
```

4.  Build static link and NO-QT-QUI release

```
cmake -DCMAKE_BUILD_TYPE=Release -DUFO_QT_UI_WALLET=FALSE .
make -j4
```


# How to run

### ufo-node

```
./ufo-node --peer=mainnet-node01.ufo.link:20015,mainnet-node02.ufo.link:20015,mainnet-node03.ufo.link:20015 --treasury_path=treasury.bin --port=20015 
```

### explorer-node

```
./explorer-node --peer=mainnet-node01.ufo.link:20015,mainnet-node02.ufo.link:20015,mainnet-node03.ufo.link:20015 --api_port=20020
```

### wallet-api

```
./wallet-api --node_addr=mainnet-node01.ufo.link:20015,mainnet-node02.ufo.link:20015,mainnet-node03.ufo.link:20015 --pass=<your wallet pass> --use_http=1 --ip_whitelist=<ip allow to access>
```
