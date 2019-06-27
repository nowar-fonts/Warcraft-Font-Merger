@echo off

%~d0
cd "%~dp0"

if exist Fonts rmdir /s /q Fonts
mkdir Fonts

copy %1 Fonts\FRIZQT__.ttf
copy %1 Fonts\ARIALN.ttf
copy %1 Fonts\skurri.ttf
copy %1 Fonts\MORPHEUS.ttf

copy %1 Fonts\FRIZQT___CYR.ttf
copy %1 Fonts\MORPHEUS_CYR.ttf
copy %1 Fonts\SKURRI_CYR.ttf

copy %1 Fonts\ARHei.ttf
copy %1 Fonts\ARKai_C.ttf
copy %1 Fonts\ARKai_T.ttf

copy %1 Fonts\arheiuhk_bd.ttf
copy %1 Fonts\bHEI00M.ttf
copy %1 Fonts\bHEI01B.ttf
copy %1 Fonts\bKAI00M.ttf
copy %1 Fonts\blei00d.ttf

copy %1 Fonts\2002.ttf
copy %1 Fonts\2002B.ttf
copy %1 Fonts\K_Damage.ttf
copy %1 Fonts\K_Pagetext.ttf

@echo on
pause
