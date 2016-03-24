@echo off

set TMPFILE=$$$rmemptydir.tmp

rem ========== 確認 ==========

cd
echo 以下の空のフォルダを削除します。
echo キャンセルする場合は「閉じる」ボタンで終了してください。
pause

rem ========== 実行 ==========

dir /AD /S /B /ON | sort /R > %TMPFILE%
for /f "delims=" %%i in (%TMPFILE%) do (
  rmdir "%%i" 2> nul
)
del %TMPFILE%
