.PHONY: clean run

SRC = dbgwin.c
EXE = dbgwin.exe
CFLAGS += -O2

all: $(EXE)

$(EXE): $(SRC)
	x86_64-w64-mingw32-gcc $(CFLAGS) $< -mwindows -o $@

clean:
	rm -f $(EXE)

run: $(EXE)
	wine $^
