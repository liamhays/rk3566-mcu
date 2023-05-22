# rk3566-mcu
This repository is a collection of information and example code for
the RISC-V MCU found inside the Rockchip RK3566 SoC, as used in the
Pine64 Quartz64, as well as the PineTab2 and the PineNote.

The file `rk3566_mbox.dts` is a device tree overlay source file that
*should* work with the Rockchip mailbox driver included in the Linux
kernel---the mailbox hardware is identical. However, Plebian and
likely other distributions do not have this module compiled by
default, and I have not yet tested if the driver actually works on the
RK3566 or not.
