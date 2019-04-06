#! /bin/bash

cd $(dirname $0)

echo 拖动需要打包的字体到此窗口，按回车键确定。
read base

[[ -d Fonts/ ]] && rm -rf Fonts/
mkdir Fonts/

cp $base Fonts/FRIZQT__.ttf
cp $base Fonts/ARIALN.ttf
cp $base Fonts/skurri.ttf
cp $base Fonts/MORPHEUS.ttf

cp $base Fonts/FRIZQT___CYR.ttf
cp $base Fonts/MORPHEUS_CYR.ttf
cp $base Fonts/SKURRI_CYR.ttf

cp $base Fonts/ARHei.ttf
cp $base Fonts/ARKai_C.ttf
cp $base Fonts/ARKai_T.ttf

cp $base Fonts/bHEI00M.ttf
cp $base Fonts/bHEI01B.ttf
cp $base Fonts/bKAI00M.ttf
cp $base Fonts/blei00d.ttf

cp $base Fonts/2002.ttf
cp $base Fonts/2002B.ttf
cp $base Fonts/K_Damage.ttf
cp $base Fonts/K_Pagetext.ttf

echo 完成
