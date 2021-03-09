@echo off

     SET SRC=blak00.c
     SET DEST=blak00.exe
     SET DESTnix=blak00
     SET includePath=R:/s/c/i
     SET includePathNix=~/c/i
     SET CC=gcc

@echo on

%CC% -D TEST -D BAOTIME_LONGLONG_ENABLED -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wno-long-long -Wvla -pedantic-errors -o %DEST% %SRC% -I %includePath% && rcedit-x64 %DEST% --set-icon raven1.ico  && %DEST%
