@echo off

set OUTPUT=$$$dumptree.txt

tree /F > %OUTPUT%
start %OUTPUT%
