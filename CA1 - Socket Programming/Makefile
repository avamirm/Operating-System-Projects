CC=gcc 
EXE_FILE=seller buyer
all: $(EXE_FILE) 

seller: seller.o
	$(CC) seller.o -o seller

buyer: buyer.o
	$(CC) buyer.o -o buyer
	
seller.o: seller.c 
	$(CC) -c seller.c -o seller.o
	
buyer.o: buyer.c 
	$(CC) -c buyer.c -o buyer.o

clean:
	rm *.o  *.txt $(EXE_FILE)
