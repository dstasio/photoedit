@echo off
set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4505 -DPHE_INTERNAL=1 -DPHE_DEBUG=1 -FAsc -Z7 /I ..\imgui
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib d3d11.lib

IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST ..\rundir mkdir ..\rundir
pushd ..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\code\.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\code\win32.cpp -Fmwin32_phe.map /link %CommonLinkerFlags% %RendererExports%
popd

pushd ..\rundir
set name=ui
fxc -nologo -Tvs_5_0 -DVERTEX_HLSL=1 -DPIXEL_HLSL=0 ..\code\ui.hlsl -Fo%name%v.tmp
fxc -nologo -Tps_5_0 -DVERTEX_HLSL=0 -DPIXEL_HLSL=1 ..\code\ui.hlsl -Fo%name%p.tmp
move /Y %name%v.tmp %name%.vsh
move /Y %name%p.tmp %name%.psh

popd


