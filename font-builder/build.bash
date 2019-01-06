ver=0.4.0

for file in src/*.ttf ; do
	prefix=${file#*/}
	otfccdump --ignore-hints --pretty --glyph-name-prefix ${prefix,,}- $file -o ${file/ttf/otd} &
done
wait

mkdir -p out

python trim-droid.py &
python trim-noto.py Medium $ver &
python trim-shs-cn.py CN Medium gbk $ver &
python trim-shs.py SC Medium gbk $ver &
python trim-shs.py TC Medium big5 $ver &
python trim-shs.py CL Medium big5 $ver &
wait

for file in out/*.otd ; do
	otfccbuild -O3 $file -o ${file/otd/ttf} &
done
wait

rm src/*.otd out/*.otd
