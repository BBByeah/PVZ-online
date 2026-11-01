@echo off
chcp 65001 >nul
setlocal
set "PATH=C:\Software\VSS\VC\Tools\MSVC\14.42.34433\bin\Hostx64\x86;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp\c\bin\10.0.26100.0\x64;C:\Software\VSS\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;C:\Software\VSS\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
set "INCLUDE=C:\Software\VSS\VC\Tools\MSVC\14.42.34433\include;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp\c\Include\10.0.26100.0\ucrt;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp\c\Include\10.0.26100.0\um;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp\c\Include\10.0.26100.0\shared;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp\c\Include\10.0.26100.0\winrt"
set "LIB=C:\Software\VSS\VC\Tools\MSVC\14.42.34433\lib\x86;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp.x86\c\um\x86;C:\Users\Byeah\Desktop\Small-project\third_party\windows-sdk\microsoft.windows.sdk.cpp.x86\c\ucrt\x86"
set VSLANG=1033
cl /nologo /EHsc /std:c++20 /utf-8 /I . tools\room_transport_test.cpp Lawn\Online\ObserverDirectLink.cpp /Fe:build-test-x86\room_transport_test.exe /Fo:build-test-x86\ /link ws2_32.lib
if errorlevel 1 exit /b 1
build-test-x86\room_transport_test.exe
