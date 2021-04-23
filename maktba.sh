# @echo off
#
#      SET SRC=blak00.c
#      SET DEST=blak00.exe
#      SET DESTnix=blak00
#      SET includePath=R:/s/c/i
#      SET includePathNix=~/c/i
#      SET CC=gcc
#
# @echo on
#
# %CC% -D TEST -D BAOTIME_LONGLONG_ENABLED -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wno-long-long -Wvla -pedantic-errors -o %DEST% %SRC% -I %includePathNix% && %DEST%

#gcc -D TEST -D BAOTIME_LONGLONG_ENABLED -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wno-long-long -Wvla -pedantic-errors -o blak00 blak00.c -I ~/s/cBao && ./blak00
#gcc -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wvla -pedantic-errors -o blak00 blak00.c -I ~/s/cBao && ./blak00
gcc -ansi -g -static-libasan -fsanitize=address -O3 -Wall -Wextra -Wshadow -Wvla -pedantic-errors -o tba tba.c -I ~/s/cBao && ./tba test && ./tba showers
