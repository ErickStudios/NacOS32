dd if=/dev/zero of=images/nacos32.img bs=512 count=2880 status=none
mkfs.fat -F 12 -n "NACOS32" images/nacos32.img > /dev/null

dd if=build/boot.bin of=images/nacos32.img conv=notrunc bs=512 count=1 status=none

for file in disk/*; do
    [ -f "$file" ] || continue

    filename=$(basename "$file")
    name="${filename%.*}"
    ext="${filename##*.}"

    name=$(printf '%s' "$name" | tr '[:lower:]' '[:upper:]')
    ext=$(printf '%s' "$ext" | tr '[:lower:]' '[:upper:]')

    mcopy -i images/nacos32.img "$file" "::${name}.${ext}"
done