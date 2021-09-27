SRC = fdm.c fdc.c
EXE = fdm

$(EXE): $(SRC)
	gcc -Wall -O -o $@ $^

clean: 
	rm -f $(EXE)
