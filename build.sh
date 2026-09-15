nasm -f elf32 kernel.asm -o build/kasm.o
gcc -m32 -c kernel.c -o build/kc.o
gcc -m32 -c nlc.c -o build/nlcc.o

ld -m elf_i386 -T link.ld -o build/kernel build/kasm.o build/nlcc.o build/kc.o

nasm -f bin lita.asm -o build/a3.bin
ndisasm -b 32 build/a3.bin

cp build/kernel iso/boot/kernel.bin

cat > iso/boot/grub/grub.cfg <<'EOF'
set timeout=0
set default=0

menuentry "MiKernel" {
    multiboot2 /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o images/nacos32.iso iso

source fat12.sh

qemu-system-i386 -cdrom images/nacos32.iso -hda images/nacos32.img -boot order=d