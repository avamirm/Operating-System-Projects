CC=g++ --std=c++17
PATH_BUILD := build
FIFO_DIR := fifo

PATH_SRC = ./src
PATH_INC = $(PATH_SRC)/include

vpath %.cpp $(PATH_SRC)
vpath %.hpp $(PATH_INC)

all: $(PATH_BUILD) $(FIFO_DIR) GenreCounter.out parts.out genres.out

GenreCounter.out: $(PATH_BUILD)/main.o
	$(CC) -o GenreCounter.out $(PATH_BUILD)/main.o

parts.out: $(PATH_BUILD)/parts.o
	$(CC) -o parts.out $(PATH_BUILD)/parts.o

genres.out: $(PATH_BUILD)/genres.o
	$(CC) -o genres.out $(PATH_BUILD)/genres.o

$(PATH_BUILD)/main.o: main.cpp
	$(CC) -c $< -o $@

$(PATH_BUILD)/parts.o: parts.cpp
	$(CC) -c $< -o $@

$(PATH_BUILD)/genres.o: genres.cpp
	$(CC) -c $< -o $@

$(PATH_BUILD): ; mkdir -p $@
$(FIFO_DIR): ; mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(PATH_BUILD) $(FIFO_DIR) *.out
