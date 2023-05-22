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
The MCU has a RV32IMC (32-bit integer RISC-V core with
integer multiply/divide and compressed instructions). It has:
- a 3-stage pipeline (custom, presumably)
- an interrupt controller known as the Integrated Programmable
  Interrupt Controller (IPIC), with 256 IRQ lines and a 256 to 4
  INTMUX
- a "32-bit AHB-Lite external memory interface"
- JTAG debug interface
- hardware breakpoints
- 3 64-bit "performance counters" (probably just timers)
- RTC
- cycle counter
- "Instructions-retired counter"

The MCU is connected to the system through an interconnect over
`I_BUS_AHB` and `D_BUS_AHB`, but `GRF_SOC_CON3` probably allows this
to be changed to AXI. Interrupts look fairly complicated but
manageable, because of the INTMUX in the IPIC.

PC register is 32 bits, which kind of stands to reason, given that the
whole SoC uses a 32-bit memory interface.

The RISC-V standard provides 3 basic levels of privilege. Rockchip has
chosen to provide only the highest, Machine (M), in this MCU, because
Supervisor (S) and User (U) are optional. See
https://danielmangum.com/posts/risc-v-bytes-privilege-levels/.

## MCU boot
The boot address field in `GRF_SOC_CON4` is only 16 bits. The
following function, taken from the Rockchip BSP u-boot, sets the boot
address of the MCU and shows that these 16 bits are the high 16 bits
of a value. For example, if the boot address field in `GRF_SOC_CON4`
is set to 0xFDCC, then the boot address of the MCU will be 0xFDCC0000.

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
These are globally mapped, some are RISC-V CSRs and I think some are not.

RISC-V names and privileges are from
https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf

Privilege column comes from the link above. "MRO" means "machine
level, read-only", and "MRW" is "machine level, read-write". If this
MCU supported modes other than Machine, the privilege level of CSRs
would matter. The RK356x TRM tries to document these, but you really
need to have at least a basic level of RISC-V knowledge.

| Number | Privilege | RISC-V Name | Rockchip name       | RISC-V Description                                    |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|
| 0xF11  | MRO       | `mvendorid` | `MCU_CSR_MVENDORID` | Vendor ID                                             |
| 0xF12  | MRO       | `marchid`   | `MCU_CSR_MARCHID`   | Architecture ID                                       |
| 0xF13  | MRO       | `mimpid`    | `MCU_CSR_MIMPID`    | Implementation ID                                     |
| 0xF14  | MRO       | `mhartid`   | `MCU_CSR_MHARTID`   | Hardware thread ID                                    |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|
| 0x300  | MRW       | `mstatus`   | `MCU_CSR_MSTATUS`   | Machine status register                               |
| 0x301  | MRW       | `misa`      | `MCU_CSR_MISA`      | ISA and extensions                                    |
| 0x304  | MRW       | `mie`       | `MCU_CSR_MIE`       | Machine interrupt-enable register                     |
| 0x305  | MRW       | `mtvec`     | `MCU_CSR_MTVEC`     | Machine trap-handler base address                     |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|
| 0x340  | MRW       | `mscratch`  | `MCU_CSR_MSCRATCH`  | Scratch register for machine trap handlers            |
| 0x341  | MRW       | `mepc`      | `MCU_CSR_MEPC`      | Machine exception program counter                     |
| 0x342  | MRW       | `mcause`    | `MCU_CSR_MCAUSE`    | Machine trap cause                                    |
| 0x343  | MRW       | `mtval`     | `MCU_CSR_MTVAL`     | Machine bad address or instruction                    |
| 0x344  | MRW       | `mip`       | `MCU_CSR_MIP`       | Machine interrupt pending                             |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|
| 0xB00  | MRW       | `mcycle`    | `MCU_CSR_MCYCLEL`   | Lower 32 bits of machine cycle counter                |
| 0xB02  | MRW       | `minstret`  | `MCU_CSR_MINSTRETL` | Lower 32 bits of machine instructions-retired counter |
| 0xB80  | MRW       | `mcycleh`   | `MCU_CSR_MCYCLEH`   | Upper 32 bits of `mcycle`                             |
| 0xB82  | MRW       | `minstreth` | `MCU_CSR_MINSTRETH` | Upper 32 bits of `minstret`                           |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|


And then, of course, there are some standard RISC-V CSR registers that
appear to be at non-standard addresses:
| Number | Rockchip number | Privilege | RISC-V name     | Rockchip name         | RISC-V Description               |
|--------|-----------------|-----------|-----------------|-----------------------|----------------------------------|
| 0x320  | 0x7E0           | MRW       | `mcountinhibit` | `MCU_CSR_MCOUNTEN`    | Machine counter-inhibit register |
| 0x7B2  | 0x7C8           | DRW       | `dscratch0`     | `MCU_CSR_DBG_SCRATCH` | Debug scratch register 0         |

**Note:** The "Offset" column in the RK3566 TRM is a lie for all MCU
registers except `MCU_TIMER_CTRL`, `MCU_TIMER_DIV`, `MCU_MTIME`,
`MCU_MTIMEH`, `MCU_MTIMECMP`, and `MCU_MTIMECMPH`, because these
registers are not RISC-V CSRs. All the other registers are CSRs, so
the Offset value is actually the CSR ID. This means that they cannot
be read directly from the ARM core.

Also note that reads from ARM, at least in a Linux kernel module, must
be word-aligned. This is likely common knowledge but I had no idea
until I figured out the above.

## MCU interrupts

### MCU interrupt control registers
According to page 517 of the Part 1 TRM, you can only write to the
IPIC CSRs using the `CSRRW`/`CSRRWI` instructions, not the
`CSRRS`/`CSRRC` (set/clear CSR bits) instructions.


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
