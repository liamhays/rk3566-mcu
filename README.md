# rk3566-mcu
This repository is a collection of information and example code for
the RISC-V MCU found inside the Rockchip RK3566 SoC used in the
Pine64 Quartz64, PineTab2, and PineNote.

To start programming the MCU in C and control it from the Linux
kernel, look at `riscv/c_mailbox` and `kernel/read_c_mailbox`. **Make
sure you compile the riscv project first, as it generates
`riscv_code.h` in the kernel project.** This is a very basic example
that compiles some C into a binary for the MCU that writes one value
to a mailbox register. The kernel module for this example resets the
MCU and mailbox, writes the MCU program to system SRAM, runs the MCU,
and then reads the mailbox register.

The `mailbox` and `store_deadbeef_code` (kernel module is called
`read_deadbeef_code`) are assembly-only examples that I used for basic
testing. I don't recommend trying to base a project off of these
unless you really want to use RISC-V assembly.


# TODO
- use the `rockchip-mailbox` driver in the Linux kernel to access the
mailbox (see `rk3566_mcu_notes.md` for more info on this)
- configure the IPIC in the MCU and the INTMUX that connects it to the
wider RK3566 world
- use MCU as wakeup source (what sleep support is in the kernel for
the RK3566 currently?)
