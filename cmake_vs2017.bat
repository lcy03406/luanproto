set DP0=%~dp0
cd %~dp0
set GENDIR=build/vs2017
mkdir "%GENDIR%"
cd "%GENDIR%"
cmake -G "Visual Studio 15 2017 Win64" ../..
cd %DP0%