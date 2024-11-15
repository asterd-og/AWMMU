all:
	gcc -m32 $(shell find ./src -name "*.c") -o out/awmmu
	./out/awmmu