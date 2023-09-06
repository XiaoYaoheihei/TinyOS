.PHONY:build

build:
	nasm -f elf ./start.asm -o ./start.o -g 
	ar rcs simple_crt.a ../out/kernel/string.o ../out/kernel/syscall.o \
	../out/kernel/stdio.o ../out/kernel/assert.o ./start.o
	gcc -Wall -c -g -fno-builtin -fno-stack-protector -W -Wstrict-prototypes -Wmissing-prototypes \
		-Wsystem-headers -I ../lib -I ../lib/user -I ../fs ../device -o prog_arg.o prog_arg.c -m32
	ld prog_arg.o simple_crt.a -o prog_arg -melf_i386
	
clean:
	rm -rf prog_no_arg

all:build