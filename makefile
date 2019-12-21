CC = g++
CFLAGS = -c -Os
SOURCES =  main.cpp SPI_Interface_software.cpp NandReader.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = x360nanddump
LINKER = -lgpiod

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
		$(CC) $(OBJECTS) $(LINKER) -o $@  

.cpp.o: *.h
	$(CC) $(CFLAGS) $< -o $@

clean :
	-rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean