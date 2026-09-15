dd if=/dev/zero of=images/nacos32.img bs=512 count=2880
mkfs.fat -F 12 -n "NACOS32" images/nacos32.img

mkdir -p /tmp/nacos_mnt
sudo mount -o loop images/nacos32.img /tmp/nacos_mnt
sudo cp test.txt /tmp/nacos_mnt/
sudo umount /tmp/nacos_mnt