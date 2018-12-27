VERSION=0.2.1

mkdir -p release

x86_64-w64-mingw32-g++ src/merge-otd.cpp src/iostream.cpp -Isrc/ -std=c++14 -O3 -static -o bin/merge-otd.exe
strip bin/merge-otd.exe

cd release


R=字体合并补全工具-简体中文压缩字库-$VERSION

mkdir -p $R

cp ../script/link.url $R/主页-使用说明.url
cp ../script/fonts.url $R/获取更多字体.url
cp ../bin/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script/comp-xs.bat $R/补全.bat
cp ../script/merge.bat $R/合并.bat
cp ../script/merge+comp-xs.bat $R/合并补全.bat
cp ../script/pack-zh.bat $R/打包.bat
cp ../font/DroidSansFallbackFull.ttf $R/cjk.ttf
cp ../font/SourceHanSansCN-Medium-WesternSymbol.ttf $R/latinx.ttf
cp ../font/NotoSans-SemiCondensedMedium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC1-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字体合并补全工具-简体中文标准字库-$VERSION

mkdir -p $R

cp ../script/link.url $R/主页-使用说明.url
cp ../script/fonts.url $R/获取更多字体.url
cp ../bin/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script/comp.bat $R/补全.bat
cp ../script/merge.bat $R/合并.bat
cp ../script/merge+comp.bat $R/合并补全.bat
cp ../script/pack-zh.bat $R/打包.bat
cp ../font/SourceHanSansCN-Medium.ttf $R/cjk.ttf
cp ../font/NotoSans-SemiCondensedMedium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC2-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字体合并补全工具-简体中文大字库-$VERSION

mkdir -p $R

cp ../script/link.url $R/主页-使用说明.url
cp ../script/fonts.url $R/获取更多字体.url
cp ../bin/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script/comp.bat $R/补全.bat
cp ../script/merge.bat $R/合并.bat
cp ../script/merge+comp.bat $R/合并补全.bat
cp ../script/pack.bat $R/打包.bat
cp ../font/SourceHanSansSC-Medium.ttf $R/cjk.ttf
cp ../font/NotoSans-SemiCondensedMedium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-SC3-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/


R=字型合併補全工具-繁體中文大字庫-$VERSION

mkdir -p $R

cp ../script/link.url $R/主頁-使用說明\(簡體\).url
cp ../script/fonts.url $R/獲取更多字體\(簡體\).url
cp ../bin/{otfccbuild,otfccdump,merge-otd}.exe $R/
cp ../script/comp.bat $R/補全.bat
cp ../script/merge.bat $R/合併.bat
cp ../script/merge+comp.bat $R/合併補全.bat
cp ../script/pack.bat $R/打包.bat
cp ../font/SourceHanSansTC-Medium.ttf $R/cjk.ttf
cp ../font/NotoSans-SemiCondensedMedium.ttf $R/latin.ttf

7z a -mx -myx -ms=on WarFontMerger-TC3-$VERSION.7z $R/
rar a -ma5 -m5 -s $R.rar $R/
