.PHONY:build image clean

img=/home/xiaoyao/desktop/bochs/hd60M.img

mbr_src=mbr.asm
loader_src=loader.asm

mbr=mbr.bin
loader=loader.bin

mbr_loader:
	nasm -I boot/include/ -o boot/${mbr} boot/${mbr_src} -g
	nasm -I boot/include/ -o boot/${loader} boot/${loader_src} -g

build:
	nasm -f elf64 -o lib/kernel/print.o lib/kernel/print.asm -g
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o kernel/main.o  kernel/main.c
	ld -Ttext 0xc0001500 -e main -o kernel/kernel.bin kernel/main.o lib/kernel/print.o

image:
	dd if=./boot/mbr.bin of=$(img) bs=512 count=1 conv=notrunc
	dd if=./boot/loader.bin of=$(img) bs=512 seek=2 count=3 conv=notrunc
	dd if=./kernel/kernel.bin of=$(img) bs=512 seek=9 count=200 conv=notrunc

clean:
	@-rm -rf boot/*.img boot/*.bin boot/*.o /boot/*~
	@-rm -rf lib/*.img lib/*.bin lib/*.o lib/*~
	@-rm -rf lib/kernel/*.img lib/kernel/*.bin lib/kernel/*.o lib/kernel/*~
	@-rm -rf kernel/*.img kernel/*.bin kernel/*.o kernel/*~
	@-rm -rf *.o *.bin *.img *~

BOCHS_PATH=/home/xiaoyao/desktop/bochs0/bin/bochs
BOCHS_GDB_PATH=/home/xiaoyao/desktop/bochs/bin/bochs
BOCHS_PORT="target remote 127.0.0.1:1234"
BOCHS_GDB_FLAG='gdbstub:enabled=1,port=1234,text_base=0,data_base=0,bss_base=0'

gdb:
	${BOCHS_GDB_PATH} -qf ../bochsrc_gdb.disk  ${BOCHS_GDB_FLAG} &
	gdb kernel/kernel.bin -ex ${BOCHS_PORT}
	pkill bochs
	make clean

run: 
	${BOCHS_PATH} -qf ../bochsrc.disk

	
all : mbr_loader build image