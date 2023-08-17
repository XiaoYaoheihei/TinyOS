.PHONY:build image clean run out

img=/home/xiaoyao/desktop/bochs/hd60M.img

mbr_src=mbr.asm
loader_src=loader.asm

mbr=mbr.bin
loader=loader.bin

out:
	mkdir -p out/kernel &
	mkdir -p out/boot

mbr_loader:
	nasm -I boot/include/ -o out/boot/${mbr} boot/${mbr_src} -g
	nasm -I boot/include/ -o out/boot/${loader} boot/${loader_src} -g

build:
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/main.o  kernel/main.c -m32
	nasm -f elf32 -o out/kernel/print.o lib/kernel/print.asm -g
	nasm -f elf32 -o out/kernel/kernel.o kernel/kernel.asm -g
	nasm -f elf32 -o out/kernel/switch.o thread/switch.asm -g
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/interrupt.o  kernel/interrupt.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/init.o  kernel/init.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/timer.o  device/timer.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/debug.o  kernel/debug.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/memory.o  kernel/memory.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/bitmap.o  lib/kernel/bitmap.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/string.o  lib/string.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/list.o  lib/kernel/list.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/thread.o  thread/thread.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/sync.o  thread/sync.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/console.o  device/console.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/keyboard.o  device/keyboard.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/ioqueue.o  device/ioqueue.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/tss.o  userprog/tss.c -m32
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/process.o  userprog/process.c -m32
	ld -Ttext 0xc0001500 -e main -o out/kernel/kernel.bin out/kernel/main.o \
	out/kernel/init.o out/kernel/interrupt.o out/kernel/print.o out/kernel/kernel.o out/kernel/switch.o \
	out/kernel/timer.o out/kernel/debug.o out/kernel/memory.o out/kernel/bitmap.o out/kernel/string.o out/kernel/thread.o \
	out/kernel/list.o out/kernel/console.o out/kernel/sync.o out/kernel/keyboard.o out/kernel/ioqueue.o out/kernel/tss.o  \
	out/kernel/process.o -melf_i386

image:
	dd if=./out/boot/mbr.bin of=$(img) bs=512 count=1 conv=notrunc
	dd if=./out/boot/loader.bin of=$(img) bs=512 seek=2 count=3 conv=notrunc
	dd if=./out/kernel/kernel.bin of=$(img) bs=512 seek=9 count=200 conv=notrunc

clean:
	@-rm -rf out/boot/*.bin
	@-rm -rf out/kernel/*.bin out/kernel/*.o
	
#	@-rm -rf boot/*.img boot/*.bin boot/*.o /boot/*~
#	@-rm -rf lib/*.img lib/*.bin lib/*.o lib/*~
#	@-rm -rf lib/kernel/*.img lib/kernel/*.bin lib/kernel/*.o lib/kernel/*~
#	@-rm -rf kernel/*.img kernel/*.bin kernel/*.o kernel/*~
#	@-rm -rf *.o *.bin *.img *~

all :out mbr_loader build image

#BOCHS_PATH=/home/xiaoyao/desktop/bochs/bin/bochs
BOCHS_GDB_PATH=/home/xiaoyao/desktop/bochs/bin/bochs
BOCHS_PORT="target remote 127.0.0.1:1234"

run:
	${BOCHS_GDB_PATH} -qf ../bochsrc_gdb.disk 

gdb:all
	${BOCHS_GDB_PATH} -qf ../bochsrc_gdb.disk &
	gdb ./out/kernel/kernel.bin -ex ${BOCHS_PORT}
	pkill bochs
	make clean
