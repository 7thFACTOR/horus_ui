@echo off

rem VS contains vswhere.exe
if "%VSWHERE%"=="" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  set InstallDir=%%i
)

echo %InstallDir%

if exist "%InstallDir%\MSBuild\Current\Bin\MSBuild.exe" (
	set msbuild="%InstallDir%\MSBuild\Current\Bin\MSBuild.exe"
	%msbuild%  .\build_msvc\horus.sln /m:4 /property:Configuration=Release
)

