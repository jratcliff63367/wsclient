@echo off
set SOURCE=F:\nvidiagit\proxyserver\src
set SOURCE_INCLUDE=f:\nvidiagit\proxyserver\include

copy %SOURCE%\*.cpp
copy %SOURCE_INCLUDE%\*.h

del ProxyServer.cpp
del ProxyServer.h
del InputLine.cpp
del InputLine.h
del FileURI.cpp
del FileURI.h
del InParser.cpp
del InParser.h
del MemoryStream.h
del StringId.h
del itoa_jeaiii.cpp
del itoa_jeaiii.h
