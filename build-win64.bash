#! /bin/bash

source ./version.bash

VERSION=$VERSION-win64

x86_64-w64-mingw32-g++ src/merge-otd.cpp src/merge-name.cpp src/iostream.cpp -Isrc/ -std=c++17 -O3 -static -o bin-win64/merge-otd.exe
strip bin-win64/merge-otd.exe

mkdir -p release
cd release


R=字体合并补全工具-简体中文压缩字库-$VERSION

mkdir -p $R

cp ../script-windows/link.url $R/主页-使用说明.url
cp ../bin-win64/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script-windows/comp.bat $R/补全.bat
cp ../script-windows/merge.bat $R/合并.bat
cp ../script-windows/merge+comp.bat $R/合并补全.bat
cp ../script-windows/pack-zh.bat $R/打包.bat
cp ../font/Nowar-Sans-CJK-XS-Regular.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Apache-Regular.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC1-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字体合并补全工具-简体中文标准字库-$VERSION

mkdir -p $R

cp ../script-windows/link.url $R/主页-使用说明.url
cp ../bin-win64/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script-windows/comp.bat $R/补全.bat
cp ../script-windows/merge.bat $R/合并.bat
cp ../script-windows/merge+comp.bat $R/合并补全.bat
cp ../script-windows/pack-zh.bat $R/打包.bat
cp ../font/Nowar-Sans-CJK-CN-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC2-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字体合并补全工具-简体中文大字库-$VERSION

mkdir -p $R

cp ../script-windows/link.url $R/主页-使用说明.url
cp ../bin-win64/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script-windows/comp.bat $R/补全.bat
cp ../script-windows/merge.bat $R/合并.bat
cp ../script-windows/merge+comp.bat $R/合并补全.bat
cp ../script-windows/pack.bat $R/打包.bat
cp ../font/Nowar-Sans-CJK-SC-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC3-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字型合併補全工具-繁體中文大字庫-$VERSION

mkdir -p $R

cp ../script-windows/link.url $R/主頁-使用說明\(簡體\).url
cp ../bin-win64/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script-windows/comp.bat $R/補全.bat
cp ../script-windows/merge.bat $R/合併.bat
cp ../script-windows/merge+comp.bat $R/合併補全.bat
cp ../script-windows/pack.bat $R/打包.bat
cp ../font/Nowar-Sans-CJK-TC-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-TC-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字型合併補全工具-傳統字形大字庫-$VERSION

mkdir -p $R

cp ../script-windows/link.url $R/主頁-使用說明\(簡體\).url
cp ../bin-win64/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script-windows/comp.bat $R/補全.bat
cp ../script-windows/merge.bat $R/合併.bat
cp ../script-windows/merge+comp.bat $R/合併補全.bat
cp ../script-windows/pack.bat $R/打包.bat
cp ../font/Nowar-Sans-CJK-CL-Medium.ttf $R/cjk.ttf
cp ../font/Nowar-Sans-LCG-Medium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-Classic-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/
