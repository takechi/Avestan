@echo off

cd ..

set BASEPATH=D:\web\app\avesta\

copy bin\avesta.exe  %BASEPATH%bin\
copy bin\avesta.dll  %BASEPATH%bin\
copy bin\pygmy.dll   %BASEPATH%bin\
copy bin\callback.py %BASEPATH%bin\

copy man\* %BASEPATH%man
copy usr\* %BASEPATH%usr

copy licence.txt %BASEPATH%
copy readme.html %BASEPATH%
copy ToDo.txt %BASEPATH%
