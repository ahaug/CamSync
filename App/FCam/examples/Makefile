
all:
	mkdir -p bin
	make -C example1
	make -C example2
	make -C example3	
	make -C example4
	make -C example5
	make -C example6
	cd example7 && qmake
	make -C example7

clean:
	make -C example1 clean
	make -C example2 clean
	make -C example3 clean
	make -C example4 clean
	make -C example5 clean
	make -C example6 clean
	make -C example7 distclean
	rm -rf *~ \#* .\#* 
