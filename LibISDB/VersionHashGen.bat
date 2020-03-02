@echo off
cd /d %~dp0
set headerfile=LibISDBVersionHash.hpp
for /f "usebackq tokens=*" %%i in (`git rev-parse --short HEAD`) do set hash=%%i
if "%hash%"=="" (
	if exist %headerfile% del %headerfile%
	exit /b 0
)
find "%hash%" %headerfile% >nul
if %errorlevel% neq 0 (
	echo #define LIBISDB_VERSION_HASH_ "%hash%">%headerfile%
)
exit /b 0
