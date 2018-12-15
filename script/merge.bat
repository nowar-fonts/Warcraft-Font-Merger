%~d0
cd "%~dp0"

.\otfccdump.exe --ignore-hints -o base.otd %1
.\otfccdump.exe --ignore-hints -o ext.otd %2

.\merge-otd.exe base.otd ext.otd

.\otfccbuild -O3 -o out.ttf base.otd

del base.otd ext.otd

pause
