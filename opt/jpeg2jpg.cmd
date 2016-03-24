@echo off

rem ========== 確認 ==========

cd
echo 以下の *.jpeg を *.jpg にリネームします。
echo キャンセルする場合は「閉じる」ボタンで終了してください。
pause

rem ========== 実行 ==========

rename *.jpeg *.jpg
for /D /R %%i in ( *.* ) do (
	cd %%i
	rename *.jpeg *.jpg
)
