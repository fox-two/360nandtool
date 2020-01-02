CC = g++
CFLAGS = -c -Os
SOURCES =  main.cpp SPI_Interface_software.cpp NandReader.cpp config.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = nand360tool
LINKER = -lgpiod

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
		$(CC) $(OBJECTS) $(LINKER) -o $@  

.cpp.o: *.h
	$(CC) $(CFLAGS) $< -o $@

clean :
	-rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean