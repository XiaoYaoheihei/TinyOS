.PHONY:build

build:
	nasm -f elf ./start.asm -o ./start.o -g 
	ar rcs simple_crt.a ../out/kernel/string.o ../out/kernel/syscall.o \
	../out/kernel/stdio.o ../out/kernel/assert.o ./start.o
	gcc -Wall -c -g -fno-builtin -fno-stack-protector -W -Wstrict-prototypes -Wmissing-prototypes \
		-Wsystem-headers -I ../lib -I ../lib/user -I ../fs -I ../device -o cat.o cat.c -m32
	ld cat.o simple_crt.a -o cat -melf_i386
	dd if=cat of=/home/xiaoyao/desktop/TinyOS/c3/os.img bs=512 count=100 seek=300 conv=notrunc
	
	# gcc -Wall -c -g -fno-builtin -fno-stack-protector -W -Wstrict-prototypes -Wmissing-prototypes \
	# -Wsystem-headers -I ../lib -I ../lib/user -I ../fs -I ../device -o prog_pipe.o prog_pipe.c -m32
	# ld prog_pipe.o simple_crt.a -o prog_pipe -melf_i386
	# ld prog_arg.o simple_crt.a -o prog_arg -melf_i386
	# gcc -Wall -c -g -fno-builtin -fno-stack-protector -W -Wstrict-prototypes -Wmissing-prototypes \
	#	-Wsystem-headers -I ../lib -I ../lib/user -I ../fs ../device -o prog_arg.o prog_arg.c -m32
	# ld prog_arg.o simple_crt.a -o prog_arg -melf_i386
	# dd if=prog_pipe of=/home/xiaoyao/desktop/TinyOS/c3/os.img bs=512 count=100 seek=300 conv=notrunc

clean:
	rm -rf prog_no_arg

all:build