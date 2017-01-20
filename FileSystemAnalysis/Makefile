CC=gcc
CFLAGS=-Wall

TARGET=FileSystemAnalysis

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c -lm -o $(TARGET)

clean:
	rm $(TARGET) *.csv 
dist:
	tar -czvf FileSystem.tar.gz FileSystemAnalysis.c Makefile
