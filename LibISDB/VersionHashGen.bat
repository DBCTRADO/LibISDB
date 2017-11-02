@echo off
cd /d %~dp0
set headerfile=LibISDBVersionHash.hpp
for /f "usebackq tokens=*" %%i in (`git rev-parse --short HEAD`) do set hash=%%i
if not "%hash%"=="" (
	echo #define LIBISDB_VERSION_HASH_ "%hash%">%headerfile%
) else if exist %headerfile% (
	del %headerfile%
)
exit /b 0
