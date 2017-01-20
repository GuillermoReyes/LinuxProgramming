TARGET=parser.py


main: $(TARGET)
	python parser.py

run:
	python parser.py

dist:
	tar -zcf parser.tar.gz README $(TARGET) Makefile

clean:
	rm -rf lab3b_check.txt
