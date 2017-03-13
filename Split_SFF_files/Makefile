
TARGET = split_sff

INCLUDE_DIR = include
SRC_DIR = src

CC  = gcc
INC = -I$(INCLUDE_DIR) $(CFLAGS)

OMP = -fopenmp

vpath %.h $(INCLUDE_DIR)
vpath %.c $(SRC_DIR)


.PHONY: clean all


$(TARGET): main.o sff.o match.o
	gcc -g -o $@  $^  $(OMP) $(LDFLAGS)

$(TARGET)_ser: main_ser.o sff_ser.o match_ser.o
	$(CC) -g -o $@  $^  $(LDFLAGS)

all: $(TARGET) $(TARGET)_ser

help:
	@echo "Usage: make [ all | clean | $(TARGET) | $(TARGET)_ser ]"


main.o: main.c main.h log.h
	$(CC) -g $(INC) $(OMP) -c $(SRC_DIR)/main.c

sff.o: sff.c sff.h log.h
	$(CC) -g $(INC) $(OMP) -c $(SRC_DIR)/sff.c

match.o: match.c match.h log.h
	$(CC) -g $(INC) $(OMP) -c $(SRC_DIR)/match.c


main_ser.o: main.c main.h log.h
	$(CC) -g $(INC) -o $@ -c $(SRC_DIR)/main.c

sff_ser.o: sff.c sff.h log.h
	$(CC) -g $(INC) -o $@ -c $(SRC_DIR)/sff.c

match_ser.o: match.c match.h log.h
	$(CC) -g $(INC) -o $@ -c $(SRC_DIR)/match.c

clean:
	rm -f *.o 

cleanall: clean
	rm -f $(TARGET) $(TARGET)_ser
