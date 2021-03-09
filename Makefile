SRC = blak00.c
DEST = blak00.exe
includePath = R:/s/c/i

.PHONY: all
all: $(SRC)
	gcc -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wvla -pedantic-errors -o $(DEST) $(SRC) -I $(includePath)

.PHONY: nolink
nolink: $(SRC)
	gcc -c -ansi -S -g0 -O3 -Wall -Wextra -Wshadow -Wvla -pedantic-errors -o $(DEST) $(SRC) -I $(includePath)

.PHONY: debug
debug:
	gcc -ansi -Wall -Wextra -Wshadow -Wvla -pedantic-errors -o $(DEST) $(SRC) -I $(includePath)
	./$(DEST)

.PHONY: clean
clean:
	rm $(DEST)
	
.PHONY: go
go:
	./$(DEST)

.PHONY: edit
edit:
	nano $(SRC)

.PHONY: emk
emk:
	nano Makefile
	
.PHONY: test-br
test-br:
	gcc -D BRANCH -D TEST -D BAOTIME_LONGLONG_ENABLED -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wvla -Wno-long-long -pedantic-errors -o $(DEST) $(SRC) -I $(includePath)
	./$(DEST)

.PHONY: test-nbr
test-nbr:
	gcc -D TEST -D BAOTIME_LONGLONG_ENABLED -ansi -g0 -O3 -Wall -Wextra -Wshadow -Wno-long-long -Wvla -pedantic-errors -o $(DEST) $(SRC) -I $(includePath)
	./$(DEST)