#!/bin/bash

source ../build/config/config.sh

for file in src/*.ttf ; do
	prefix=${file#*/}
	otfccdump --ignore-hints --pretty --glyph-name-prefix ${prefix,,}- $file -o ${file/ttf/otd} &
done
wait

mkdir -p out

python trim-droid.py $VERSION &
python trim-noto.py Medium $VERSION &
python trim-shs.py SC Medium gbk $VERSION &
python trim-shs.py CL Medium big5 $VERSION &
wait

for file in out/*.otd ; do
	otfccbuild -O3 $file -o ${file/otd/ttf} &
done
wait

rm src/*.otd out/*.otd
