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
- the MCU timer base address (undocumented in the RK356x TRM) and usage
- INTMUX interrupt mapping (needs more testing but should be accurate)
  and register location
- MCU execution during suspend: **due to system architecture and ARM
  TF-A, this is currently impossible**. TF-A requests bus idles on
  suspend for the MCU and the memory it likes to run from, making it
  impossible to run during system suspend.

What's left:
- Using the `rockchip-mailbox` Linux driver---diederik put in a [pull
  request to
  Debian](https://salsa.debian.org/kernel-team/linux/-/merge_requests/730)
  to enable the `CONFIG_ROCKCHIP_MBOX` kernel flag, and it has been
  merged...it will appear in Plebian eventually.

  
If there's interest in a kernel module for the MCU, open an issue on
this repository or contact me some other way and I may be able to
write one.


# Using the MCU
See `examples` for code examples. Each folder is a basic kernel module
and RISC-V source for the MCU. Make sure you install the
`linux-headers` package for your OS and architecture.

# Contributing
If you know anything about the MCU or have done experimentation, open
an issue or pull request to add information. I am also interested in
the TRM for the RV1126 chip, because this chip has the same MCU core
integrated but uses different registers and might have different
internal connections.
