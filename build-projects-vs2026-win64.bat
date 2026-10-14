@echo off

call "..\traktor\scripts\config.bat"

set SOLUTIONBUILDER=%TRAKTOR_HOME%\bin\win64\releasestatic\Traktor.SolutionBuilder.App

%SOLUTIONBUILDER% ^
	-f=msvc ^
	"-p=$(TRAKTOR_HOME)\resources\build\configurations\msvc-2026-win64.xml" ^
	"ZombieWin64.xms" ^
	"-i"
