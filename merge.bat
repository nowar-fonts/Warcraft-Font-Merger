%~d0
cd "%~dp0"

.\otfccdump.exe --ignore-hints -o base.otd %1
.\otfccdump.exe --ignore-hints -o latin.otd latin.ttf
.\otfccdump.exe --ignore-hints --name-by-hash -o cjk.otd cjk.ttf

.\merge-otd.exe base.otd latin.otd cjk.otd

.\otfccbuild -O3 -o out.ttf base.otd

del base.otd latin.otd cjk.otd

pause
