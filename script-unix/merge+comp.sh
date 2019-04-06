#! /bin/bash

cd $(dirname $0)

echo 拖动英文字体到此窗口，按回车键确定。
read base
echo 拖动中文字体到此窗口，按回车键确定。
read ext

./otfccdump.exe --ignore-hints -o base.otd $base
./otfccdump.exe --ignore-hints -o latin.otd latin.ttf
./otfccdump.exe --ignore-hints -o ext.otd $ext
./otfccdump.exe --ignore-hints --name-by-hash -o cjk.otd cjk.ttf

./merge-otd.exe base.otd latin.otd ext.otd cjk.otd

./otfccbuild.exe -O3 -o out.ttf base.otd

rm base.otd latin.otd ext.otd cjk.otd
