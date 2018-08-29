set VSVER=vs2017
IF EXIST vs2015 set VSVER=vs2015
premake5 %VSVER%

xcopy .\3rdparty\sfml\win64\bin\*.* .\bin /c /q /i /e /h /y