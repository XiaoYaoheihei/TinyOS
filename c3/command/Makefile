.PHONY:build

build:
	gcc -Wall -c -g -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes \
		-Wsystem-headers -I ../lib -o prog_no_arg.o prog_no_arg.c -m32
	ld -e main prog_no_arg.o ../out/kernel/string.o ../out/kernel/syscall.o\
		../out/kernel/stdio.o ../out/kernel/assert.o -o prog_no_arg -melf_i386
	dd if=prog_no_arg of=/home/xiaoyao/desktop/TinyOS/c3/os.img \
		bs=512 count=100 seek=300 conv=notrunc

clean:
	rm -rf prog_no_arg

all:build