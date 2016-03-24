@echo off

cd ..

rem **** main **** 

del /S *.log
del /S *.ncb
del /S *.aps
del /S *.pyc
del /S /AH Thumbs.db
del var\$*
del var\default.ave
del var\config.xml
del var\settings.dat
del var\$*
rmdir /S /Q tools\html
rmdir /S /Q __obj__

rem debug
cd debug
call :DELOBJ
call :DELEXE
rmdir /S /Q log
cd ..

rem bin
cd bin
call :DELOBJ
cd ..

goto :EOF

rem **** DELEXE **** 

:DELEXE
del *.exe
del *.dll
goto :EOF

rem **** DELOBJ **** 

:DELOBJ
del *.exp
del *.lib
del *.ilk
del *.pdb
goto :EOF

rem **** EOF ****

:EOF
