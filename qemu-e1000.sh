make output/boot-cd-grub.iso && qemu-system-x86_64 -cdrom output/boot-cd-grub.iso -monitor stdio -m 1g -netdev type=bridge,id=net0 -device e1000,netdev=net0 -machine q35
