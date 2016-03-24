@echo off

set UPLOADPATH=D:\web\app\
set OUTFILE=%UPLOADPATH%avesta.diff.txt

del %OUTFILE%

rem
set LABEL=ToDo.txt
set NAME=ToDo.txt
call :DIFF

rem
set LABEL=callback.py
set NAME=bin\callback.py
call :DIFF

rem
cd ..\man
for %%i in ( *.xml ) do (
    set LABEL=%%i
    set NAME=man\%%i
    call :DIFF
)

rem
cd ..\usr
for %%i in ( *.xml;*.ini ) do (
    set LABEL=%%i
    set NAME=usr\%%i
    call :DIFF
)

goto :EOF

rem **** DIFF **** 

:DIFF
set FILTER=find /v "%LABEL%"
echo ==================== %LABEL% ==================== >> %OUTFILE%
diff -u -r -b --label=%LABEL% %UPLOADPATH%avesta\%NAME% ..\%NAME% | %FILTER% >> %OUTFILE%
goto :EOF

rem **** EOF ****

:EOF
