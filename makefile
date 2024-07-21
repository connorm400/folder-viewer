install:
	@mkdir -p .build
	gcc -o ./.build/folderview main.c -lncurses

run:
	$(MAKE) install
	./.build/folderview