# to be `source`-ed
# do not call this script directly

function package_sc() {
	mkdir -p release/$_Dist
	cp script/homepage.url release/$_Dist/主页-使用说明.$_url_extension
	cp build/{otfccbuild,otfccdump,merge-otd}$_binary_suffix release/$_Dist/
	cp font/$_cjk.ttf release/$_Dist/cjk.ttf
	cp font/$_latin.ttf release/$_Dist/latin.ttf

	case $_platform in
		unix)
			cp script/unix/comp.sh release/$_Dist/补全.$_script_extension
			cp script/unix/merge.sh release/$_Dist/合并.$_script_extension
			cp script/unix/merge+comp.sh release/$_Dist/合并补全.$_script_extension
			cp script/unix/pack.sh release/$_Dist/打包.$_script_extension
			;;
		windows)
			cp script/windows/comp.bat release/$_Dist/补全.bat
			cp script/windows/merge.bat release/$_Dist/合并.bat
			cp script/windows/merge+comp.bat release/$_Dist/合并补全.bat
			cp script/windows/pack.bat release/$_Dist/打包.bat
			;;
	esac

	case $_archive in
		tar.xz)
			cd release/
			tar -cJf $_dist.tar.xz $_Dist/
			cp $_dist.tar.xz $_Dist.tar.xz
			cd ../
			;;
		7z)
			cd release/
			7z a -mx -myx -ms=on $_dist.7z $_Dist/
			cp $_dist.7z $_Dist.7z
			cd ../
			;;
	esac
}

function package_tc() {
	mkdir -p release/$_Dist
	cp script/homepage.url release/$_Dist/主頁-使用說明\(簡體\).$_url_extension
	cp build/{otfccbuild,otfccdump,merge-otd}$_binary_suffix release/$_Dist/
	cp font/$_cjk.ttf release/$_Dist/cjk.ttf
	cp font/$_latin.ttf release/$_Dist/latin.ttf

	case $_platform in
		unix)
			cp script/unix/comp.sh release/$_Dist/補全.$_script_extension
			cp script/unix/merge.sh release/$_Dist/合併.$_script_extension
			cp script/unix/merge+comp.sh release/$_Dist/合併補全.$_script_extension
			cp script/unix/pack.sh release/$_Dist/打包.$_script_extension
			;;
		windows)
			cp script/windows/comp.bat release/$_Dist/補全.bat
			cp script/windows/merge.bat release/$_Dist/合併.bat
			cp script/windows/merge+comp.bat release/$_Dist/合併補全.bat
			cp script/windows/pack.bat release/$_Dist/打包.bat
			;;
	esac

	case $_archive in
		tar.xz)
			cd release/
			tar -cJf $_dist.tar.xz $_Dist/
			cp $_dist.tar.xz $_Dist.tar.xz
			cd ../
			;;
		7z)
			cd release/
			7z a -mx -myx -ms=on $_dist.7z $_Dist/
			cp $_dist.7z $_Dist.7z
			cd ../
			;;
	esac
}
