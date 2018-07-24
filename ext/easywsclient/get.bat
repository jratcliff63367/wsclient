@echo off
set SOURCE=F:\nvidiagit\proxyserver\src
set SOURCE_INCLUDE=f:\nvidiagit\proxyserver\include

copy %SOURCE%\easywsclient.cpp
copy %SOURCE%\FastXOR.cpp
copy %SOURCE%\SimpleBuffer.cpp
copy %SOURCE%\wplatform.cpp
copy %SOURCE%\wsocket.cpp

copy %SOURCE_INCLUDE%\wsocket.h
copy %SOURCE_INCLUDE%\easywsclient.h
copy %SOURCE_INCLUDE%\wplatform.h
copy %SOURCE_INCLUDE%\SimpleBuffer.h
copy %SOURCE_INCLUDE%\FastXOR.h
