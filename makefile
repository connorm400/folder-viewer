install:
	gcc -o ./.build/folderview main.c -lncurses

run:
	$(MAKE) install
	./.build/folderview