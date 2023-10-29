CC=g++ --std=c++11
PATH_BUILD := build

all: $(PATH_BUILD)  ImageFilters.out 

ImageFilters.out: $(PATH_BUILD)/main.o
	$(CC) -o ImageFilters.out $(PATH_BUILD)/main.o


$(PATH_BUILD)/main.o: main.cpp
	$(CC) -c $< -o $@


$(PATH_BUILD): ; mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(PATH_BUILD) *.out output.bmp


