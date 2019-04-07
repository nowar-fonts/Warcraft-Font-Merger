#! /bin/bash

cd "$(dirname "$0")"

echo 拖动需要补全的字体到此窗口，按回车键确定。
read base

./otfccdump --ignore-hints -o base.otd "$base"
./otfccdump --ignore-hints -o latin.otd latin.ttf
./otfccdump --ignore-hints -o cjk.otd cjk.ttf

./merge-otd base.otd latin.otd cjk.otd

./otfccbuild -q -O3 -o out.ttf base.otd

rm base.otd latin.otd cjk.otd
