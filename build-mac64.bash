#! /bin/bash

source ./version.bash

VERSION=$VERSION-mac64

clang++ src/merge-otd.cpp src/merge-name.cpp src/ps2tt.cpp src/tt2ps.cpp src/iostream.cpp -Isrc/ -std=c++17 -O3 -s -o bin-mac64/merge-otd

mkdir -p release
cd release


R=字体合并补全工具-简体中文压缩字库-$VERSION

mkdir -p $R

cp ../script-unix/link.url $R/主页-使用说明.url
cp ../bin-mac64/{otfccbuild,otfccdump,merge-otd} $R/
cp ../script-unix/comp.sh $R/补全.command
cp ../script-unix/merge.sh $R/合并.command
cp ../script-unix/merge+comp.sh $R/合并补全.command
cp ../script-unix/pack-zh.sh $R/打包.command
cp ../font/Nowar-Sans-CJK-XS-Regular.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Apache-Regular.ttf $R/latin.ttf

tar -cJf $R.tar.xz $R/
cp $R.tar.xz WarFontMerger-SC1-$VERSION.tar.xz


R=字体合并补全工具-简体中文标准字库-$VERSION

mkdir -p $R

cp ../script-unix/link.url $R/主页-使用说明.url
cp ../bin-mac64/{otfccbuild,otfccdump,merge-otd} $R/
cp ../script-unix/comp.sh $R/补全.command
cp ../script-unix/merge.sh $R/合并.command
cp ../script-unix/merge+comp.sh $R/合并补全.command
cp ../script-unix/pack-zh.sh $R/打包.command
cp ../font/Nowar-Sans-CJK-CN-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

tar -cJf $R.tar.xz $R/
cp $R.tar.xz WarFontMerger-SC2-$VERSION.tar.xz


R=字体合并补全工具-简体中文大字库-$VERSION

mkdir -p $R

cp ../script-unix/link.url $R/主页-使用说明.url
cp ../bin-mac64/{otfccbuild,otfccdump,merge-otd} $R/
cp ../script-unix/comp.sh $R/补全.command
cp ../script-unix/merge.sh $R/合并.command
cp ../script-unix/merge+comp.sh $R/合并补全.command
cp ../script-unix/pack.sh $R/打包.command
cp ../font/Nowar-Sans-CJK-SC-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

tar -cJf $R.tar.xz $R/
cp $R.tar.xz WarFontMerger-SC3-$VERSION.tar.xz


R=字型合併補全工具-繁體中文大字庫-$VERSION

mkdir -p $R

cp ../script-unix/link.url $R/主頁-使用說明\(簡體\).url
cp ../bin-mac64/{otfccbuild,otfccdump,merge-otd} $R/
cp ../script-unix/comp.sh $R/補全.command
cp ../script-unix/merge.sh $R/合併.command
cp ../script-unix/merge+comp.sh $R/合併補全.command
cp ../script-unix/pack.sh $R/打包.command
cp ../font/Nowar-Sans-CJK-TC-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

tar -cJf $R.tar.xz $R/
cp $R.tar.xz WarFontMerger-TC-$VERSION.tar.xz


R=字型合併補全工具-傳統字形大字庫-$VERSION

mkdir -p $R

cp ../script-unix/link.url $R/主頁-使用說明\(簡體\).url
cp ../bin-mac64/{otfccbuild,otfccdump,merge-otd} $R/
cp ../script-unix/comp.sh $R/補全.command
cp ../script-unix/merge.sh $R/合併.command
cp ../script-unix/merge+comp.sh $R/合併補全.command
cp ../script-unix/pack.sh $R/打包.command
cp ../font/Nowar-Sans-CJK-CL-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

tar -cJf $R.tar.xz $R/
cp $R.tar.xz WarFontMerger-Classic-$VERSION.tar.xz
