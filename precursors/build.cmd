@echo off

@del /F /Q .\%1.obj
@del /F /Q .\%1.dumpbin
@del /F /Q .\%1.code
@del /F /Q .\%1_decl.h
@del /F /Q .\%1_data.h
@del /F /Q .\%1_impl.h

@cl.exe /MP /GS- /GS- /Zc:rvalueCast- /TC /Qpar- /W3 /Gy- /Zc:wchar_t- /Fx /Gm- /Ox /Ob2 /sdl- /Zc:inline /fp:precise /Zp1 /D "NDEBUG" /fp:except- /GF- /WX- /Zc:forScope- /GR- /Gz /Oy /Oi /MT /nologo /Zl /FAc /Ot /GS- .\%1.c
@dumpbin /DISASM:BYTES .\%1.obj > .\%1.dumpbin
@dmp .\%1.dumpbin
@mkc %1 .\%1.code


