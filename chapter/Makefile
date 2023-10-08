.PHONY:build image clean run out

img=os.img
img1=hd80.img

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
	gcc -I lib/kernel -I shell -I fs -I lib -I lib/user -I device -I kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/main.o  kernel/main.c -m32
	nasm -f elf32 -o out/kernel/print.o lib/kernel/print.asm -g
	nasm -f elf32 -o out/kernel/kernel.o kernel/kernel.asm -g
	nasm -f elf32 -o out/kernel/switch.o thread/switch.asm -g
	gcc -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/interrupt.o  kernel/interrupt.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -I fs -c -fno-builtin -W -Wstrict-prototypes \
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
	gcc -I kernel -I device -I lib -I fs -I lib/kernel -c -fno-builtin -W -Wstrict-prototypes \
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
	gcc -I lib/kernel -I thread -I fs -I lib -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/syscall.o  lib/user/syscall.c -m32
	gcc -I lib -I lib/kernel -I kernel -I thread -I fs -I lib -I device -I shell -I userprog -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/syscall_init.o  userprog/syscall_init.c -m32
	gcc -I lib/kernel -I thread -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/stdio.o  lib/stdio.c -m32
	gcc -I lib/kernel -I thread -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/ide.o  device/ide.c -m32
	gcc -I lib/kernel -I thread -c -fno-builtin -W -Wstrict-prototypes \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/stdio_kernel.o  lib/kernel/stdio_kernel.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -I shell -c -fno-builtin \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/fs.o fs/fs.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -c -fno-builtin \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/inode.o fs/inode.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -c -fno-builtin \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/file.o fs/file.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -c -fno-builtin  \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/dir.o fs/dir.c -m32
	gcc -I lib -I device -I lib/kernel -I thread -I kernel -I fs -I shell -c -fno-builtin  \
      -Wmissing-prototypes -fno-stack-protector -ggdb -nostdinc -mno-sse -g -o out/kernel/fork.o userprog/fork.c -m32

	ld -Ttext 0xc0001500 -e main -o out/kernel/kernel.bin out/kernel/main.o \
	out/kernel/init.o out/kernel/interrupt.o out/kernel/print.o out/kernel/kernel.o out/kernel/switch.o \
	out/kernel/timer.o out/kernel/debug.o out/kernel/memory.o out/kernel/bitmap.o out/kernel/string.o out/kernel/thread.o \
	out/kernel/list.o out/kernel/console.o out/kernel/sync.o out/kernel/keyboard.o out/kernel/ioqueue.o out/kernel/tss.o  \
	out/kernel/process.o out/kernel/syscall.o out/kernel/syscall_init.o out/kernel/stdio.o out/kernel/ide.o out/kernel/stdio_kernel.o \
	out/kernel/fs.o out/kernel/inode.o out/kernel/file.o out/kernel/dir.o out/kernel/fork.o -melf_i386

image:
	dd if=/dev/zero of=$(img) bs=512 count=61440
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
