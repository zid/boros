.PHONY : \
	all \
	clean \
	run \
	run-qemu-cd-grub \
	run-qemu-cd-isolinux \
	run-qemu-hd-efi

CFLAGS = -W -Wall -nodefaultlibs \
	-nostdlib -ffreestanding -O3 -ggdb -mgeneral-regs-only \
	-mcmodel=kernel -mno-red-zone -Iinclude/ -fno-pic \
	-fno-stack-protector -masm=intel -flto -no-pie

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
ASMSRC = $(wildcard *.asm)
ASMOBJ = $(ASMSRC:.asm=.o)
BOOT_CD_GRUB_MENU_FILE = output/cdroot/boot/grub/menu.lst
BOOT_CD_ISOLINUX_MENU_FILE = output/cdroot/isolinux/isolinux.cfg

all: kernel.bin boot.bin user.bin

prep:
	mkdir -p fs
	mount -o loop=/dev/loop1 fs.bin fs/

install: kernel.bin boot.bin user.bin
	cp boot/boot.bin fs/boot/
	cp kernel.bin fs/boot/
	cp user/user.bin fs/boot/
	sync

boot.bin:
	$(MAKE) -C boot/

user.bin:
	$(MAKE) -C user/

kernel.bin: $(OBJ) $(ASMOBJ)
	gcc -Wl,-Tlinker.ld $(OBJ) $(ASMOBJ) -o kernel.bin \
	-Wl,-z,max-page-size=4096 -Wl,-Map,kernel.map $(CFLAGS)

%.o : %.asm
	nasm -felf64 $^ -o $@

%.o : %.c
	$(CC) $^ -c -o $@ $(CFLAGS)

output:
	mkdir $@

output/boot-cd-grub.iso : user.bin kernel.bin boot.bin output
	mkdir -p output/cdroot/boot/grub
	cp -f 3rdparty/grub/stage2_eltorito output/cdroot/boot/grub/
	cp -f boot/boot.bin output/cdroot/boros.bin
	cp -f kernel.bin output/cdroot/kernel.bin
	cp -f user/user.bin output/cdroot/user.bin

	echo "default 0"                >  $(BOOT_CD_GRUB_MENU_FILE)
	echo "timeout 1"                >> $(BOOT_CD_GRUB_MENU_FILE)
	echo "title boros"              >> $(BOOT_CD_GRUB_MENU_FILE)
	echo "  root (cd)"              >> $(BOOT_CD_GRUB_MENU_FILE)
	echo "  kernel (cd)/boros.bin"  >> $(BOOT_CD_GRUB_MENU_FILE)
	echo "	module (cd)/kernel.bin" >> $(BOOT_CD_GRUB_MENU_FILE)
	echo "  module (cd)/user.bin"   >> $(BOOT_CD_GRUB_MENU_FILE)

	mkisofs \
		-o $@ \
		-b boot/grub/stage2_eltorito \
		-boot-info-table \
		-boot-load-size 4 \
		-no-emul-boot \
		-joliet \
		-rational-rock \
		-graft-points boot/grub/grub.conf=output/cdroot/boot/grub/menu.lst \
		output/cdroot

output/boot-cd-isolinux.iso : kernel.bin boot.bin output
	mkdir -p output/cdroot/isolinux
	cp -f 3rdparty/syslinux/bios/isolinux.bin output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/ldlinux.c32  output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/libcom32.c32 output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/libmenu.c32  output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/libutil.c32  output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/mboot.c32    output/cdroot/isolinux/
	cp -f 3rdparty/syslinux/bios/menu.c32     output/cdroot/isolinux/
	cp -f boot/boot.bin output/cdroot/boros.bin
	cp -f kernel.bin output/cdroot/kernel.bin

	echo "ui menu.c32"                  >  $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "prompt 0"                     >> $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "timeout 10"                   >> $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "default boros"                >> $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "label boros"                  >> $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "  kernel /isolinux/mboot.c32" >> $(BOOT_CD_ISOLINUX_MENU_FILE)
	echo "  append /boros.bin"          >> $(BOOT_CD_ISOLINUX_MENU_FILE)

	mkisofs \
		-o $@ \
		-b isolinux/isolinux.bin \
		-boot-info-table \
		-boot-load-size 4 \
		-c boot.cat \
		-joliet \
		-no-emul-boot \
		-rational-rock \
		output/cdroot

output/hd-efi.img : kernel.bin boot.bin output
	dd if=/dev/zero of=$@ bs=1M count=64
	sgdisk --new=1:1M:+62M --typecode=1:ef00 $@

	dd if=/dev/zero of=output/fat.img bs=1M count=62
	mkdosfs -F 32 output/fat.img

	mmd -i output/fat.img ::/EFI
	mmd -i output/fat.img ::/EFI/BOOT
	mmd -i output/fat.img ::/loader
	mmd -i output/fat.img ::/loader/entries

	echo "timeout 1"          >  output/boros.conf
	echo "default boros"      >> output/boros.conf
	echo "title boros"        >> output/boros.conf
	echo "linux /boros.bin"   >> output/boros.conf

	echo "timeout 1"          >  output/loader.conf
	echo "default boros"      >> output/loader.conf

	mcopy -i output/fat.img 3rdparty/systemd/systemd-bootx64.efi ::/EFI/BOOT/bootx64.efi
	mcopy -i output/fat.img output/loader.conf           ::/loader/
	mcopy -i output/fat.img output/boros.conf            ::/loader/entries/
	mcopy -i output/fat.img boot/boot.bin                ::/boros.bin

	dd if=output/fat.img of=$@ bs=1M seek=1 conv=notrunc

run: boot.bin kernel.bin install
	bochs

run-qemu-cd-grub : output/boot-cd-grub.iso
	qemu-system-x86_64 -enable-kvm -m 256 -boot d \
		-drive file=$<,format=raw,media=cdrom \

run-qemu-cd-isolinux : output/boot-cd-isolinux.iso
	qemu-system-x86_64 -enable-kvm -m 256 -boot d \
		-drive file=$<,format=raw,media=cdrom \

run-qemu-hd-efi : output/hd-efi.img
	qemu-system-x86_64 -enable-kvm -m 256 -boot c \
		-drive file=$<,format=raw \
		-bios /usr/share/ovmf/ovmf_code_x64.bin		

clean:
	$(MAKE) -C boot clean
	$(MAKE) -C user clean
	-rm *.o kernel.bin kernel.map
	-rm -rf output/*
