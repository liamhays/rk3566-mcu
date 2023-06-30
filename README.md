# rk3566-mcu
This repository is a collection of information and example code for
the RISC-V MCU found inside the Rockchip RK3566 SoC used in the Pine64
Quartz64, PineTab2, and PineNote. Documentation is in
`rk3566_mcu_notes.md`.

What's documented:
- Development environment in C
- Info about the core
- How to boot the MCU
- Some system-level MCU configuration registers
- INTMUX interrupt mapping (needs more testing but should be accurate)
  and register location


What's left:
- Configuring the MCU timer and finding its base address
- Using the `rockchip-mailbox` Linux driver---diederik put in a [pull
  request to
  Debian](https://salsa.debian.org/kernel-team/linux/-/merge_requests/730)
  to enable the `CONFIG_ROCKCHIP_MBOX` kernel flag, and it has been
  merged...it will appear in Plebian eventually.
- Using the MCU as a wakeup source, which depends on support for
  suspend of whatever type for the RK3566.


# Using the MCU
See `examples` for code examples.
