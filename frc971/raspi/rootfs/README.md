This modifies a stock debian root filesystem to be able to operate as a vision
pi.  It is not trying to be reproducible, but should be good enough for FRC
purposes.

The default hostname and IP is pi-971-1, 10.9.71.101.
  Username pi, password raspberry.

Download 2021-10-30-raspios-bullseye-armhf-lite.img (or any newer
bullseye version, as a .zip file) from
`https://www.raspberrypi.org/downloads/raspberry-pi-os/`, extract
(unzip) the .img file, and edit `modify_rootfs.sh` to point to it.

Run modify_rootfs.sh to build the filesystem (you might need to hit
return in a spot or two and will need sudo privileges to mount the
partition):
  * `modify_root.sh`

VERY IMPORTANT NOTE: Before doing the next step, use `lsblk` to find
the device and make absolutely sure this isn't your hard drive or
something else.  It will target /dev/sda by default, which in some
computers is your default hard drive.

After confirming the target device, edit the `make_sd.sh` script to point to the correct IMAGE filename, and run the `make_sd.sh` command,
which takes the name of the pi as an argument:
  * `make_sd.sh pi-971-1`

OR, if you want to manually run this, you can deploy the image by
copying the contents of the image to the SD card.  You can do this
manually, via
  `dd if=2020-02-13-raspbian-bullseye-lite-frc-mods.img of=/dev/sdX bs=1M`

From there, transfer the SD card to the pi, log in, `sudo` to `root`,
and run `/root/bin/change_hostname.sh` to change the hostname to the
actual target.


A couple additional notes on setting this up:
   * You'll likely need to install (`sudo apt install`) the emulation packages `proot` and `qemu-user-static` (or possibly `qemu-arm-static`)
   * If the modify_root.sh script fails, you may need to manually unmount the image (`sudo umount ${PARTITION}` and `rmdir ${PARTITION}`) before running it again
   * Don't be clever and try to link to the image file in a different folder.  These scripts need access directly to the file and will fail otherwise


Things to do once the SD card is complete, and you've booted a PI with it:

  * Download the code:
    Once this is completed, you can boot the pi with the newly flashed SD
    card, and download the code to the pi using:
      `bazel run -c opt --cpu=armv7 //y2022:pi_download_stripped -- PI_IP_ADDR

    where PI_IP_ADDR is the IP address of the target pi, e.g., 10.9.71.101