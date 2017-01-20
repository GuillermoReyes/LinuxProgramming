

.SILENT:

default: 
	gcc -g lab0.c -o lab0

check: default log catch same
	cat test.log
	if [[ -s test.log ]]; then \
	 echo "Failed tests"; \
	 cat test.log; \
	 rm -f test.log; \
	else \
	 echo "Passed All tests..."; \
	fi

log: 
	>test.log

catch:
	echo | ./lab0 --segfault --catch &>/dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "catch faile" >>test.log; \
	fi

readin: 
	touch input.txt
	echo | ./lab0 --input=input.txt &>/dev/null; \
	if [[ $$? -eq 1 ]]; then \
	echo "inpu topening faile" >>test.log; \
	fi
	rm -f input.txt; \

same:
	> out.txt; \
	echo "Test Test Test" >in.txt; \
	echo | ./lab0 --input=in.txt --output=out.txt &>/dev/null; \
	cmp in.txt out.txt &>/dev/null; \
	if [[ $$? -ne 0 ]]; then \
	echo "Error: output and input are not the same" >>log.txt; \
	fi
	rm -f out.txt in.txt

clean: 
	rm -f *.o *.txt *.log *.in *.out *.tar.gz lab0

dist: default
	tar -czf lab0-404671456.tar.gz README lab0.c backtrace.png badAssignment.png Makefile
