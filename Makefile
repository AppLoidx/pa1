all:
	clang -std=c99 -Wall -fsanitize=address -pedantic *.c


tar:
	cd ..
	cp -r lab1 pa1
	tar cfvz pa1.tar.gz pa1

clear:
	rm -f a.out
	rm -f events.log
	rm -f pipes.log
