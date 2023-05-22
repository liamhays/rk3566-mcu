Much of this information is drawn from the [RK3568 Technical Reference
Manual](https://opensource.rock-chips.com/images/2/26/Rockchip_RK3568_TRM_Part1_V1.3-20220930P.PDF). The
RK3568 and RK3566 are basically the same chip with [some slight
changes](https://www.96rocks.com/blog/2020/11/28/introduce-rockchip-rk3568/)
(scroll down to the end of the page).

Mailbox is for certain types of communication. However, the MCU has
full memory access, so that seems like a better place to start.


`W1C` means "write 1 to clear"
# MCU registers accessible from ARM
These registers *control* the MCU in the context of the RK3566 as a
whole. The MCU also has dedicated registers that control subsystems
within it.

When write enable bits are 1, write access is enabled to the
respective bit.

I don't know what all of these registers do yet.

## `CRU_GATE_CON32`
| Bit   | Attr | Reset value | Description                             |
|-------|------|-------------|-----------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits            |
| 13    | RW   | 0x0         | `aclk_mcu_en` (1=enable MCU clock gate) |

## `CRU_SOFTRST_CON26`
| Bit   | Attr | Reset value | Description                         |
|-------|------|-------------|-------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits        |
| 10    | RW   | 0x1         | `aresetn_mcu` (1=hold MCU in reset) |

Note that the MCU defaults to reset, to prevent it from being a
nuisance for the rest of the chip.

## `GRF_SOC_STATUS0`
| Bit   | Attr | Reset value | Description                  |
|-------|------|-------------|------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits |
| 27    | RW   | 0x0         | `wfi_halted` status from MCU |

## `GRF_SOC_CON3`
| Bit   | Attr | Reset value | Description                                           |
|-------|------|-------------|-------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                          |
| 15    | RW   | 0x0         | `mcu_ahb2axi_d_buf_flush`                             |
| 14    | RW   | 0x0         | `mcu_ahb2axi_i_buf_flush`                             |
| 13    | RW   | 0x0         | `mcu_sel_axi`, 0=MCU uses AHB bus, 1=MCU uses AXI bus |
| 12    | RW   | 0x0         | `mcu_soft_irq`, "MCU soft interrupt request"          |

## `GRF_SOC_CON4`
| Bit   | Attr | Reset value | Description                       |
|-------|------|-------------|-----------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits      |
| 15:0  | RW   | 0x0000      | `mcu_boot_addr`, MCU boot address |

I think this is actually the high 16 bits of a 32 bit boot address for
the MCU, but I don't know that for certain yet.

## `PMU_INT_MASK_CON`
| Bit   | Attr | Reset value | Description                                                     |
|-------|------|-------------|-----------------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                                    |
| 15    | RW   | 0x0         | `wakeup_mcu_sft`, 1=wakeup PMU when MCU is set as wakeup source |

I think bit 15 of this register is like part 2 of a 2-step process to
enable the MCU as a wakeup source. First you have to set the bit in
`PMU_WAKEUP_INT_CON` (see below), then you have to enable bit 15 in
this register. Together, both bits enable the MCU as a wakeup source.

## `PMU_WAKEUP_INT_CON`
| Bit   | Attr | Reset value | Description                                                  |
|-------|------|-------------|--------------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                                 |
| 15    | RW   | 0x0         | `wakeup_mcu_sft_en`, 1=enable mcusft source as wakeup source |

## `PMU_WAKEUP_INT_ST`
| Bit   | Attr | Reset value | Description                                            |
|-------|------|-------------|--------------------------------------------------------|
| 31:16 | RO   | 0x0000      | reserved (this is a read-only register)                |
| 15    | RO   | 0x0         | `wakeup_sys_int_st`, 1=MCU sft as wakeup source status |

This bit is used to find the wakeup source, and bit 15 indicates when
the MCU was the wakeup source.

# Programming the MCU
See the example kernel module `read_mimpid` for code, but the basic
flow is:
1. Ensure MCU is in reset by writing to `CRU_SOFTRST_CON26`
2. Put RISC-V code at a known address with the four low nibbles equal
   to 0 (for example, system SRAM at 0xFDCC0000)
3. Write the RISC-V boot address to `GRF_SOC_CON4`
4. Configure other MCU-related registers, like interrupts
5. Take the MCU out of reset by writing again to `CRU_SOFTRST_CON26`


# MCU Details
The MCU is an implementation of the open-source [SCR1
core](https://github.com/syntacore/scr1). This includes separate
instruction and data buses, a JTAG debug unit, only machine privilege
mode, and a 16-line interrupt controller. Rockchip has chosen to
implement an SCR1 core with a 3-stage pipeline, the RV32IMC
instruction set and extensions.

The name `SCR1` is used in various parts of the RK356x TRM and
Rockchip BSP code in reference to the RISC-V core, but Rockchip
documentation never explicitly defines the core as an implementation
of the SCR1 core.

The external memory interface can be either an AXI or an AHB
interface. Which bus the MCU uses is configurable through
`GRF_SOC_CON3` (untested but seems plausible). 

Much of the documentation in the TRM is literal copy-paste from the
[SCR1 External Architecture
Specification (EAS)](https://github.com/syntacore/scr1/blob/master/docs/scr1_eas.pdf). 

The MCU is connected to the system through an interconnect over
`I_BUS_AHB` and `D_BUS_AHB`, but `GRF_SOC_CON3` probably allows this
to be changed to AXI. Interrupts look fairly complicated but
manageable, because of the INTMUX in the IPIC.

## MCU boot
The boot address field in `GRF_SOC_CON4` is only 16 bits. The
following function, taken from the Rockchip BSP u-boot, sets the boot
address of the MCU and shows that these 16 bits are the high 16 bits
of a 32-bit value. For example, if the boot address field in
`GRF_SOC_CON4` is set to 0xFDCC, then the boot address of the MCU will
be 0xFDCC0000.

```c
#ifdef CONFIG_SPL_BUILD
int spl_fit_standalone_release(char *id, uintptr_t entry_point)
{
	/* Reset the scr1 */
	writel(0x04000400, CRU_BASE + CRU_SOFTRST_CON26);
	udelay(100);
	/* set the scr1 addr */
	writel((0xffff0000) | (entry_point >> 16), GRF_BASE + GRF_SOC_CON4);
	udelay(10);
	/* release the scr1 */
	writel(0x04000000, CRU_BASE + CRU_SOFTRST_CON26);

	return 0;
}
#endif
```

## MCU CPU control registers
The Rockchip register names seen in the TRM are build by adding
`MCU_CSR_` to the start of the official SCR1 CSR names. The SCR1
specification also specifies a timer and IPIC registers. Check the
SCR1 EAS for info on the registers in a more accessible format than
the copypasta in the RK356x TRM.

The extra CSRs for the timers are memory-mapped, and cannot be
accessed with byte and halfword accesses, resulting in the following
note:

**Note:** The "Offset" column in the RK3566 TRM is a lie for all MCU
registers except `MCU_TIMER_CTRL`, `MCU_TIMER_DIV`, `MCU_MTIME`,
`MCU_MTIMEH`, `MCU_MTIMECMP`, and `MCU_MTIMECMPH`, because these are
all memory-mapped CSRs (see the SCR1 EAS for info on these registers).

Also note that reads from ARM, at least in a Linux kernel module, must
be word-aligned. This is likely common knowledge but I had no idea
until I figured out the above.

## MCU memory
The SCR1 core provides an optional 64KB of tightly coupled
memory. There is no definite statement in the RK356x TRM if this has
been included in the RK3566, but the `SYSTEM_SRAM` at `0xFDCC0000` in
the RK3566 is 64KB. Could this be the same TCM specified in the SCR1
specification? Probably not, since it can be used by other
peripherals, but it still works fine for the SCR1.

I have looked through the kernel sources and have found no use of
`SYSTEM_SRAM`, even though it reads non-zero values from Linux. The
data is also consistent across boots, and my theory right now is that
`SYSTEM_SRAM` is used by the boot blob. I have trampled on
`SYSTEM_SRAM` from Linux many times with seemingly no ill
effects. Note also that the RK3566 has 8KB of `PMU_SRAM` which has
"secure access only". I haven't touched this and I don't think there's
much reason to.

## MCU interrupts
The SCR1 core has a 16-line "Integrated Programmable Interrupt
Controller", configurable with IPIC CSRs. Interrupt number 0 has the
lowest priority and interrupt number 16 has the highest priority.

I haven't explored the interrupts yet, I will add more documentation
when I do.

### MCU interrupt control registers
You can only write to the IPIC CSRs using the `CSRRW`/`CSRRWI`
instructions, not the `CSRRS`/`CSRRC` (set/clear CSR bits)
instructions.

# Mailbox
The mailbox is nothing more than a set of 32-bit registers. Every
register can be accessed by both the ARM cores and the MCU. From Linux
under Plebian with kernel 6.1.0-9, at least, the clock gate on the
mailbox is enabled by default, so it must be disabled before the
mailbox can be used. 

The interrupt documentation in the Mailbox section of the RK3568 TRM
seems to be ripped directly from the RK3368, as the interrupt numbers
haven't been updated.

## Mailbox through kernel driver
The `rockchip-mailbox` driver in the Linux kernel source implements a
mailbox interface for the RK3368, at least according to the
`compatible` field. This driver uses the same register offsets as the
RK3566, so I think we should try to add a device tree overlay invoking
this driver.

Possible overlay code:
```
/* Mailbox at 0xfe780000 */
mbox: mbox@fe780000 {
	    /* abusing the driver */
        compatible: "rockchip,rk3368-mailbox";
        /* the mbox instance for the RK3368 uses 0x1000 */
        reg = <0x0 0xfe780000 0x0 0x1000>;
        /* SPI means "shared peripheral interrupts", these are the
        ones called "mailbox_ca55[0..3]" and are generated by the B2A registers.*/
        interrupts = <GIC_SPI 215 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 216 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 217 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 218 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&cru PCLK_MAILBOX>;
		/* the clock is still called "pclk_mailbox" but where it's
        defined or not is yet to be seen */
        clock-names = "pclk_mailbox";
        #mbox-cells = <1>;
		/* not disabling this here */
};
```


**Update:** This would probably work if the Rockchip mailbox driver
were compiled into the Plebian kernel. I discussed this on the bridged
#Quartz64 channel and `diederik` made a pull request to Debian to
enable the config option `CONFIG_ROCKCHIP_MBOX`. However, they said
that it won't make it into Debian Bookworm, so I think mailbox work
through the kernel driver will have to wait a bit.
