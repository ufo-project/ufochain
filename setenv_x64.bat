 @echo off
set GRA_ROOT=E:\ufodep
set OPENSSL_ROOT=%GRA_ROOT%\openssl
set OPENSSL_ROOT_DIR=%OPENSSL_ROOT%
set OPENSSL_INCLUDE_DIR=%OPENSSL_ROOT%\include
set BOOST_ROOT=%GRA_ROOT%\boost_1_68_0
set PATH=%GRA_ROOT%\cmake\bin;%BOOST_ROOT%\lib;%PATH%
set CMAKE_PREFIX_PATH=E:\qt5.11.2\5.11.2\msvc2017_64

echo Setting up VS2017 environment...
call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64