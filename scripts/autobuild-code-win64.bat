@echo off

:: Generate and build Zombie's own C++ (Zombie and Zombie.Editor) against a built Traktor:
:: the one in TRAKTOR_HOME when set, else the one in traktor\ beside the scripts folder, as the
:: CI lays it out. MSBuild must be on the path. No -i: Traktor is linked as built, not compiled.

for %%i in ("%~dp0.") do (set GAME_HOME=%%~dpi)
if %GAME_HOME:~-1%==\ set GAME_HOME=%GAME_HOME:~0,-1%

if not defined TRAKTOR_HOME set TRAKTOR_HOME=%GAME_HOME%\traktor
if not exist "%TRAKTOR_HOME%\scripts\config.bat" (
	echo No Traktor in "%TRAKTOR_HOME%".
	exit /b 1
)
call "%TRAKTOR_HOME%\scripts\config.bat"
echo Traktor in "%TRAKTOR_HOME%".

set SOLUTIONBUILDER=%TRAKTOR_HOME%\bin\win64\releasestatic\Traktor.SolutionBuilder.App

pushd %GAME_HOME%
%SOLUTIONBUILDER% -f=msvc "-p=$(TRAKTOR_HOME)\resources\build\configurations\msvc-2022-win64.xml" ZombieWin64.xms || goto failed
msbuild -m "build\win64\Zombie Win64.sln" /p:Configuration=ReleaseShared /p:Platform=x64 || goto failed
popd
exit /b 0

:failed
popd
exit /b 1
