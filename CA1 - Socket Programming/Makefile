CC=gcc 
PATH_SRC = ./src

vpath %.c $(PATH_SRC)

EXE_FILE=seller buyer

all: $(EXE_FILE) 

seller: seller.o
	$(CC) seller.o -o seller

buyer: buyer.o
	$(CC) buyer.o -o buyer
	
seller.o:  $(PATH_SRC)/seller.c 
	$(CC) -c  $(PATH_SRC)/seller.c -o seller.o
	
buyer.o:  $(PATH_SRC)/buyer.c 
	$(CC) -c  $(PATH_SRC)/buyer.c -o buyer.o

clean:
	rm *.o  *.txt $(EXE_FILE)
