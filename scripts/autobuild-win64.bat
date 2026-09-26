@echo off

for %%i in ("%~dp0.") do (set GAME_HOME=%%~dpi)
if %GAME_HOME:~-1%==\ set GAME_HOME=%GAME_HOME:~0,-1%

pushd %GAME_HOME%
%GAME_HOME%\traktor\bin\latest\win64\releaseshared\Traktor.Runtime.Deploy.App Zombie.workspace build Zombie Windows
%GAME_HOME%\traktor\bin\latest\win64\releaseshared\Traktor.Runtime.Deploy.App Zombie.workspace migrate Zombie Windows
popd
