@echo off

call "..\traktor\scripts\config.bat"

%SOLUTIONBUILDER% ^
	-f=msvc ^
	"-p=$(TRAKTOR_HOME)\resources\build\configurations\msvc-2026-win64.xml" ^
	"ZombieWin64.xms" ^
	"-i"
