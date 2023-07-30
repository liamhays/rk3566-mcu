# RK3566 MCU notes
This document details the RISC-V MCU inside the RK3566 SoC. You should
have a good knowledge of RISC-V system architecture (CSRs, instruction
set) as well as low-level computer architecture (registers and memory)
in general.

Much of this information is drawn from the [RK3568 Technical Reference
Manual](https://opensource.rock-chips.com/images/2/26/Rockchip_RK3568_TRM_Part1_V1.3-20220930P.PDF). The
RK3568 and RK3566 are basically the same chip with [some slight
changes](https://www.96rocks.com/blog/2020/11/28/introduce-rockchip-rk3568/)
(scroll down to the end of the page).

If you reference the [RISC-V ISA
spec](https://riscv.org/technical/specifications/), be sure to include
the privileged spec document.

Helpful hint: `|` next to a signal name is
the Verilog bit-reduction operator, which is equivalent to ANDing all
the bits of a bitfield together. Because the TRM appears to basically
be the output of Doxygen on a Verilog source, a lot of Verilog
notation shows up.

# Toolchains and code
To compile code for the MCU, I recommend using either the [xPack GNU
RISC-V tools](https://xpack.github.io/dev-tools/riscv-none-elf-gcc) or
[building a toolchain from
scratch](https://github.com/riscv-collab/riscv-gnu-toolchain). Do not
use a `-unknown-linux` or `-linux-gnu` toolchain, as there's no Linux
on the MCU to link to. I also did not have success using the
`gcc-riscv64-unknown-elf` and `binutils-riscv64-unknown-elf` packages
in the Debian repository (in spite of the fact that compiling from
scratch also creates an `-unknown-elf` toolchain).

The xPack tools are available pre-built for x64, ARM, and ARM64
architectures. If you're building RISC-V code on the RK3566, use the
ARM64 build. Each example project shows how to invoke the compiler in
the Makefile, but here are the flags I've used with success:

- `-march=rv32imc -mabi=ilp32 -misa-spec=2.2`: RV32IMC CPU, ILP32 ABI
  (no floating-point), and version 2.2 of the user-mode ISA spec. Note
  that gcc cannot produce code specifically intended for machine mode,
  but setting the version to 2.2 enables CSR registers and the `wfi`
  instruction.
- `-static -nostdlib`: static linking (there's no runtime
  environment), and disable any stdlib features (again, no runtime
  environment. It is probably possible to set up newlib on the MCU).
- `-mstrict-align -fno-common`: from the SCR1 example Makefile
- `-nostartfiles`: We have a `crt0.S`, but it's added in the link
  stage.
- `-Wall -lgcc`: Enable warnings and link to `libgcc.a`, used for
  hardware compatibility.

# RK3566 MCU control registers
These registers are about the MCU in the context of the RK3566 as a
whole. The MCU also has dedicated registers that control subsystems
within it.

When write enable bits are 1, write access is enabled to the
respective bit.

`W1C` means "write 1 to clear", usually used for interrupt status
registers.

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

`wfi_halted` is asserted when the MCU runs the `wfi` (wait for
interrupt) instruction. I assume it's de-asserted on MCU reset or when
MCU leaves `wfi` state.

**TODO**: This register also has stuff for `buf_flush`
status. `GRF_SOC_CON6` also has ahb2axi control bits.

## `GRF_SOC_CON3`
| Bit   | Attr | Reset value | Description                                           |
|-------|------|-------------|-------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                          |
| 15    | RW   | 0x0         | `mcu_ahb2axi_d_buf_flush`                             |
| 14    | RW   | 0x0         | `mcu_ahb2axi_i_buf_flush`                             |
| 13    | RW   | 0x0         | `mcu_sel_axi`, 0=MCU uses AHB bus, 1=MCU uses AXI bus |
| 12    | RW   | 0x0         | `mcu_soft_irq`, "MCU soft interrupt request"          |

The SCR1 core supports both AHB-Lite and AXI4 bus connections, but it
appears from the Rockchip MCU block diagram that the MCU is connected
over AHB. Presumably there is some kind of buffer between the AHB and
AXI buses in the interconnect block, controllable with the `flush`
bits. (I haven't tested anything with these two bits yet).

`mcu_sel_axi` sets the MCU to use either the AHB or AXI bus. Enabling
it makes the MCU work exactly as it does with the bit disabled. I
think this is actually slightly more complicated than it sounds:

- The SCR1 core has AHB-Lite and AXI4 bus interfaces. It does not
  appear to require you to use one interface exclusively, but there
  also appears to be no way to tell the SCR1 which bus to use.
- The RK3566 TRM chapter on the MCU shows a block diagram where the I
  and D buses from the SCR1 are called `I_BUS_AHB` and `D_BUS_AHB` and
  both go to a block called `INTERCONNECT`. This suggests to me that
  the SCR1 is connected to the rest of the RK3566 through *only* the
  AHB bus.
- However, the existence of the `ahb2axi_*_flush` bits suggests that
  the AHB<->AXI interconnect for the MCU has buffers for the I and D
  buses.

My theory is that Rockchip enabled only the AHB interface on the SCR1
to save silicon space, and used an existing AHB<->AXI IP block with an
added toggle. This doesn't really make a lot of sense---what
particular reason would require the MCU to sit on AXI---but I guess
the option exists.

Another possibility is that the MCU has both bus interfaces connected,
but certain buses are only usable in certain power conditions. For
example, the AXI bus might not be usable in a low-power state, while
the AHB bus would. **I have no evidence for this theory.**

`mcu_soft_irq` is the `soft_irq` (software interrupt) signal described
on page 86 of the SCR1 EAS. By asserting this bit, you can trigger a
software interrupt in the MCU. See the `soft_irq` example for
details---it's not very complicated. This example also sets
`mcu_sel_axi`, to show that the MCU still works with it enabled.

See the "ahb2axi" section below for some ahb2axi investigation.

The `buf_flush` bits do not seem to be automatically reset.

## `GRF_SOC_CON4`
| Bit   | Attr | Reset value | Description                       |
|-------|------|-------------|-----------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits      |
| 15:0  | RW   | 0x0000      | `mcu_boot_addr`, MCU boot address |

See below for how this field works.

## `PMU_INT_MASK_CON`
| Bit   | Attr | Reset value | Description                                                     |
|-------|------|-------------|-----------------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                                    |
| 15    | RW   | 0x0         | `wakeup_mcu_sft`, 1=wakeup PMU when MCU is set as wakeup source |

If the PMU has been configured to wakeup by the MCU, the MCU can write
1 to this bit to wakeup the PMU. A similar method is used in the
RK3399: the bit `wakeup_sft_m0` in `PMU_SFT_CON` is written to by the
M0 to wake up the PMU.

`mcu_sft` means "MCU software".

**Idea:** make the MCU blink the LED on loop, then put the Quartz to
sleep and see if it's still running.

## `PMU_WAKEUP_INT_CON`
| Bit   | Attr | Reset value | Description                                                  |
|-------|------|-------------|--------------------------------------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits                                 |
| 15    | RW   | 0x0         | `wakeup_mcu_sft_en`, 1=enable mcusft source as wakeup source |

Write 1 to this bit to enable using the register above as a wakeup
method.

## `PMU_WAKEUP_INT_ST`
| Bit   | Attr | Reset value | Description                                            |
|-------|------|-------------|--------------------------------------------------------|
| 31:16 | RO   | 0x0000      | reserved (this is a read-only register)                |
| 15    | RO   | 0x0         | `wakeup_sys_int_st`, 1=MCU sft as wakeup source status |

This register is used to find the wakeup source, and this bit
indicates when the MCU was the reason it woke up.

# Programming the MCU
See the `timer_irq` example for code, but the basic flow is:
1. Disable the clock gate on the MCU, mailbox, and INTMUX---the
   mailbox and possible the INTMUX have the clock gate enabled in
   Linux by default
1. Put MCU, mailbox, and INTMUX in reset by writing to `CRU_SOFTRST_CON26`
2. Put RISC-V code at a known address on a 64KB boundary (four low nibbles equal
   to 0), for example, system SRAM at 0xFDCC0000
3. Write the RISC-V boot address to `GRF_SOC_CON4`, with system SRAM,
   this would be 0xFDCC
4. Configure other MCU-related registers
5. Take the MCU, mailbox, and INTMUX out of reset by writing again to
   `CRU_SOFTRST_CON26`


# MCU Details
The MCU is an implementation of the open-source [SCR1
core](https://github.com/syntacore/scr1). This includes separate
instruction and data buses, a JTAG debug unit, only machine privilege
mode, and a 16-line interrupt controller. Rockchip has chosen to
implement an SCR1 core with a 3-stage pipeline and the RV32IMC
instruction set.

The name `SCR1` is used in various parts of the RK356x TRM and
Rockchip BSP code in reference to the RISC-V core, but Rockchip
documentation never explicitly defines the core as an implementation
of the SCR1 core.

The external memory interface can be either an AXI or an AHB
interface. Which bus the MCU uses seems to be configurable through
`GRF_SOC_CON3` (though I haven't tested this yet).

Much of the documentation in the TRM is literal copy-paste from the
[SCR1 External Architecture
Specification (EAS)](https://github.com/syntacore/scr1/blob/master/docs/scr1_eas.pdf). 

The MCU is connected to the system through an interconnect over
`I_BUS_AHB` and `D_BUS_AHB`, but `GRF_SOC_CON3` probably allows this
to be changed to AXI. Interrupts look fairly complicated but
manageable, because of the INTMUX in the IPIC.

The same SCR1 core is used in the RV1126.

## MCU boot
The boot address field in `GRF_SOC_CON4` is only 16 bits, even though
the MCU boots from a 32-bit address. The following function, found in
the Rockchip BSP u-boot, sets the boot address of the MCU and shows
that these 16 bits are the high 16 bits of a 32-bit value. For
example, if the boot address field in `GRF_SOC_CON4` is set to 0xFDCC,
then the boot address of the MCU will be 0xFDCC0000. This means that
MCU code must be aligned on a 64KB boundary.

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

The RV1126 has similar code for its SCR1.

## MCU CPU control registers
The Rockchip register names seen in the TRM are build by adding
`MCU_CSR_` to the start of the official SCR1 CSR names. The SCR1
specification also specifies a timer and IPIC registers. Check the
SCR1 EAS for info on the registers in a more accessible format than
the copypasta in the RK356x TRM.

**Note:** The "Offset" column in the RK3566 TRM is a lie for all MCU
registers except `MCU_TIMER_CTRL`, `MCU_TIMER_DIV`, `MCU_MTIME`,
`MCU_MTIMEH`, `MCU_MTIMECMP`, and `MCU_MTIMECMPH`, because these six
are memory-mapped registers (**these are not CSRs!**). The rest of the
registers listed are CSRs, and their "Offset" is actually the CSR
number. (see the SCR1 EAS for info on these registers).

The timer control registers are in the Reserved section after the
`MCU_INTC` section, at 0xFE7F0000. These registers are not mirrored,
as they are implemented in the SCR1 core as an address mask, as
opposed to a peripheral inside the RK3566. 

The timer follows the RISC-V timer standard. See the `timer_irq` for
an example.

## MCU memory
The SCR1 core provides an optional 64KB of tightly coupled
memory. The RK3566 does not integrate this.

`SYSTEM_SRAM` is 64KB used as part of either the DDR blob or TF-A or
both (I'm not sure which combination), but once the system has booted
into Linux, you are free to modify it as you wish.

`PMU_SRAM` is 8KB and can only be accessed in secure mode. I think
this means that only the ARM cores can access this region of
memory: what good is it to have secure-only access if there's a
RISC-V side channel right there? Furthermore, TF-A puts some code in
`PMU_SRAM`, and if you try to write on top of this, the entire system
will hard crash and reboot.

As far as I can tell, DDR RAM cannot be accessed from the MCU. I
allocated 128KB of DRAM from the kernel memory pool, found a
64KB-aligned address within that allocated space, copied a RISC-V
program to it, and started the MCU from that address. The MCU did not
start, regardless of whether it was set to AXI or AHB. I don't know
why this is---`SYSTEM_SRAM` is on AXI and works just fine, for
example---but my conclusion is that you can't use DDR RAM with the
MCU.

# MCU interrupts
The SCR1 core has a 16-line "Integrated Programmable Interrupt
Controller", configurable with IPIC CSRs. Interrupt number 0 has the
lowest priority and interrupt number 16 has the highest priority.

Although the IPIC has 16 input lines, only 4 are used in the
RK3566. The INTMUX selects one of 4 interrupts out of 256 inputs, and
interrupts from the IPIC are external interrupts to the RISC-V
core. The TRM states that "the Interrupt Multiplexer will select 4
interrupts from 256 interrupts using round robin algorithm". This
sounds like time division multiplexing to me, and my theory is that
the INTMUX cycles through all 256 inputs very, very quickly, and each
time it comes to an enabled interrupt input, it enables the
smallest-numbered output line that isn't in use. For example, imagine
that interrupt inputs 0, 10, 34, and 46 are enabled. On the first pass
around all four enabled interrupts, interrupt 0 fires, triggering the
INTMUX to enable output line 0. Then, on the next pass, interrupt 10
fires, and the INTMUX feeds that to line 1. **Note that I have no idea
if this is accurate.**

Another theory I have is that the INTMUX will assign inputs to outputs
at random, but that doesn't explain why there's four outputs.

You can only write to the IPIC CSRs using the `CSRRW`/`CSRRWI`
instructions, not the `CSRRS`/`CSRRC` (set/clear CSR bits)
instructions.

The approach I've used so far is to set the MCU core to direct
interrupt mode and branch from there, since I haven't figured out the
INTMUX-IPIC connections yet.

**Note:** the `edp` interrupt appears to be continuously firing. I
reported this in the Quartz64 bridged chat, we'll see what happens.

## INTMUX registers
The TRM does not explicitly say the base address of the INTMUX the way
it does for every other peripheral in the chip. However, the MCU
section of memory is called `MCU_INTC` and is located at 0xFE790000. I
discovered that this is the INTMUX range by probing the `MCU_INTC`
section and matching the writable registers to the registers described
for the INTMUX.

## INTMUX mapping
Using the mailbox, I can confirm that the INTMUX appears to have
Reserved interrupts connected to its inputs. The only GIC interrupts
that don't map to INTMUX inputs are the PPIs at the start.

| INTMUX number | Global interrupt name          | GIC number |
|---------------|--------------------------------|------------|
| 0             | audpwm                         | 32         |
| 1             | can0                           | 33         |
| 2             | can1                           | 34         |
| 3             | can2                           | 35         |
| 4             | crypto_ns                      | 36         |
| 5             | _Reserved                      | 37         |
| 6             | csirx0_1                       | 38         |
| 7             | csirx0_2                       | 39         |
| 8             | csirx1_1                       | 40         |
| 9             | csirx1_2                       | 41         |
| 10            | dcf                            | 42         |
| 11            | ddrmon                         | 43         |
| 12            | dma2ddr                        | 44         |
| 13            | dmac0_abort                    | 45         |
| 14            | \|dmac0                        | 46         |
| 15            | dmac1_abort                    | 47         |
| 16            | \|dmac1                        | 48         |
| 17            | ebc                            | 49         |
| 18            | edp                            | 50         |
| 19            | emmc                           | 51         |
| 20            | gic_err                        | 52         |
| 21            | gic_fault                      | 53         |
| 22            | gic_pmu                        | 54         |
| 23            | gmac0_lpi                      | 55         |
| 24            | gmac0_pmt                      | 56         |
| 25            | gmac0_sbd_perch_rx             | 57         |
| 26            | gmac0_sbd_perch_tx             | 58         |
| 27            | gmac0_sbd                      | 59         |
| 28            | gmac1_lpi                      | 60         |
| 29            | gmac1_pmt                      | 61         |
| 30            | gmac1_sbd_perch_rx             | 62         |
| 31            | gmac1_sbd_perch_tx             | 63         |
| 32            | gmac1_sbd                      | 64         |
| 33            | gpio0_pmu                      | 65         |
| 34            | gpio1                          | 66         |
| 35            | gpio2                          | 67         |
| 36            | gpio3                          | 68         |
| 37            | gpio4                          | 69         |
| 38            | gpu_event                      | 70         |
| 39            | gpu_gpu                        | 71         |
| 40            | gpu_job                        | 72         |
| 41            | gpu_mmu                        | 73         |
| 42            | hdcp                           | 74         |
| 43            | _Reserved                      | 75         |
| 44            | hdmi_wakeup                    | 76         |
| 45            | hdmi                           | 77         |
| 46            | i2c0_pmu                       | 78         |
| 47            | i2c1                           | 79         |
| 48            | i2c2                           | 80         |
| 49            | i2c3                           | 81         |
| 50            | i2c4                           | 82         |
| 51            | i2c5                           | 83         |
| 52            | i2s0_8ch                       | 84         |
| 53            | i2s1_8ch                       | 85         |
| 54            | i2s2_2ch                       | 86         |
| 55            | i2s3_2ch                       | 87         |
| 56            | iep                            | 88         |
| 57            | isp_mipi                       | 89         |
| 58            | isp_mi                         | 90         |
| 59            | isp_mmu                        | 91         |
| 60            | isp                            | 92         |
| 61            | jpeg_dec_mmu                   | 93         |
| 62            | jpeg_dec                       | 94         |
| 63            | jpeg_enc_mmu                   | 95         |
| 64            | jpeg_enc                       | 96         |
| 65            | lfps_beacon_multi_phy0         | 97         |
| 66            | lfps_beacon_multi_phy1         | 98         |
| 67            | lfps_beacon_multi_phy2         | 99         |
| 68            | mipi_dsi_0                     | 100        |
| 69            | mipi_dsi_1                     | 101        |
| 70            | nandc                          | 102        |
| 71            | pcie20_err                     | 103        |
| 72            | pcie20_legacy                  | 104        |
| 73            | pcie20_msg_rx                  | 105        |
| 74            | pcie20_pmc                     | 106        |
| 75            | pcie20_sys                     | 107        |
| 76            | pdm                            | 108        |
| 77            | pmu                            | 109        |
| 78            | pvtm_core                      | 110        |
| 79            | pvtm_gpu                       | 111        |
| 80            | pvtm_npu                       | 112        |
| 81            | pvtm_pmu                       | 113        |
| 82            | pwm_pmu                        | 114        |
| 83            | pwm1                           | 115        |
| 84            | pwm2                           | 116        |
| 85            | pwm3                           | 117        |
| 86            | pwr_pwm_pmu                    | 118        |
| 87            | pwr_pwm1                       | 119        |
| 88            | pwr_pwm2                       | 120        |
| 89            | pwr_pwm3                       | 121        |
| 90            | rga                            | 122        |
| 91            | rkvdec_m_dec                   | 123        |
| 92            | rkvdec_m_mmu                   | 124        |
| 93            | saradc                         | 125        |
| 94            | sata0                          | 126        |
| 95            | sata1                          | 127        |
| 96            | sata2                          | 128        |
| 97            | scr                            | 129        |
| 98            | sdmmc0                         | 130        |
| 99            | sdmmc1                         | 131        |
| 100           | sdmmc2                         | 132        |
| 101           | fspi                           | 133        |
| 102           | spdif_8ch                      | 134        |
| 103           | spi0                           | 135        |
| 104           | spi1                           | 136        |
| 105           | spi2                           | 137        |
| 106           | spi3                           | 138        |
| 107           | stimer0                        | 139        |
| 108           | stimer1                        | 140        |
| 109           | timer0                         | 141        |
| 110           | timer1                         | 142        |
| 111           | timer2                         | 143        |
| 112           | timer3                         | 144        |
| 113           | timer4                         | 145        |
| 114           | timer5                         | 146        |
| 115           | tsadc                          | 147        |
| 116           | uart0_pmu                      | 148        |
| 117           | uart1                          | 149        |
| 118           | uart2                          | 150        |
| 119           | uart3                          | 151        |
| 120           | uart4                          | 152        |
| 121           | uart5                          | 153        |
| 122           | uart6                          | 154        |
| 123           | uart7                          | 155        |
| 124           | uart8                          | 156        |
| 125           | uart9                          | 157        |
| 126           | upctl_alert_err                | 158        |
| 127           | upctl_arpoison                 | 159        |
| 128           | upctl_awpoison                 | 160        |
| 129           | usb2host0_arb                  | 161        |
| 130           | usb2host0_ehci                 | 162        |
| 131           | usb2host0_ochi                 | 163        |
| 132           | usb2host1_arb                  | 164        |
| 133           | usb2host1_ehci                 | 165        |
| 134           | usb2host1_ochi                 | 166        |
| 135           | usbphy0_grf                    | 167        |
| 136           | usbphy1_grf                    | 168        |
| 137           | vad                            | 169        |
| 138           | vdpu_mmu                       | 170        |
| 139           | vdpu_xintdec                   | 171        |
| 140           | rkvenc_enc                     | 172        |
| 141           | rkvenc_mmu0                    | 173        |
| 142           | rkvenc_mmu2                    | 174        |
| 143           | _Reserved                      | 175        |
| 144           | vop_lb                         | 176        |
| 145           | vicap0                         | 177        |
| 146           | vicap1                         | 178        |
| 147           | vop_ddr                        | 179        |
| 148           | vop                            | 180        |
| 149           | wdtns                          | 181        |
| 150           | wdts                           | 182        |
| 151           | npu                            | 183        |
| 152           | sdmmc0_detectn_grf             | 184        |
| 153           | sdmmc1_detectn_grf             | 185        |
| 154           | sdmmc2_detectn_grf             | 186        |
| 155           | sdmmc0_dectn_in_flt_grf        | 187        |
| 156           | pcie30x1_err                   | 188        |
| 157           | pcie30x1_legacy                | 189        |
| 158           | pcie30x1_msg_rx                | 190        |
| 159           | pcie30x1_pmc                   | 191        |
| 160           | pcie30x1_sys                   | 192        |
| 161           | pcie30x2_err                   | 193        |
| 162           | pcie30x2_legacy                | 194        |
| 163           | pcie30x2_msg_rx                | 195        |
| 164           | pcie30x2_pmc                   | 196        |
| 165           | pcie30x2_sys                   | 197        |
| 166           | key_reader                     | 198        |
| 167           | otpc_ns                        | 199        |
| 168           | otp_s                          | 200        |
| 169           | usb3otg0                       | 201        |
| 170           | usb3otg1                       | 202        |
| 171           | sbr_done_intr                  | 203        |
| 172           | ecc_corrected_err_intr         | 204        |
| 173           | ecc_corrected_err_intr_fault   | 205        |
| 174           | ecc_uncorrected_err_intr       | 206        |
| 175           | ecc_uncorrected_err_intr_fault | 207        |
| 176           | derate_temp_limit_intr         | 208        |
| 177           | derate_temp_limit_intr_fault   | 209        |
| 178           | eink                           | 210        |
| 179           | _Reserved                      | 211        |
| 180           | hwffc                          | 212        |
| 181           | ahb2axi_i                      | 213        |
| 182           | ahb2axi_d                      | 214        |
| 183           | mailbox_ca55[0]                | 215        |
| 184           | mailbox_ca55[1]                | 216        |
| 185           | mailbox_ca55[2]                | 217        |
| 186           | mailbox_ca55[3]                | 218        |
| 187           | mailbox_mcu[0]                 | 219        |
| 188           | mailbox_mcu[1]                 | 220        |
| 189           | mailbox_mcu[2]                 | 221        |
| 190           | mailbox_mcu[3]                 | 222        |
| 191           | trng_chk                       | 223        |
| 192           | xpcs_sbd                       | 224        |
| 193           | otp_mask                       | 225        |
| 194-227       | Reserved block                 | 226-259    |
| 228           | ca55_pmuirq[0]                 | 260        |
| 229           | ca55_pmuirq[1]                 | 261        |
| 230           | ca55_pmuirq[2]                 | 262        |
| 231           | ca55_pmuirq[3]                 | 263        |
| 232           | nvcpumntirq[0]                 | 264        |
| 233           | nvcpumntirq[1]                 | 265        |
| 234           | nvcpumntirq[2]                 | 266        |
| 235           | nvcpumntirq[3]                 | 267        |
| 236           | ncommirq[0]                    | 268        |
| 237           | ncommirq[1]                    | 269        |
| 238           | ncommirq[2]                    | 270        |
| 239           | ncommirq[3]                    | 271        |
| 240           | nfaultirq[0]                   | 272        |
| 241           | nfaultirq[1]                   | 273        |
| 242           | nfaultirq[2]                   | 274        |
| 243           | nfaultirq[3]                   | 275        |
| 244           | nfaultirq[4]                   | 276        |
| 245           | nerrirq[0]                     | 277        |
| 246           | nerrirq[1]                     | 278        |
| 247           | nerrirq[2]                     | 279        |
| 248           | nerrirq[3]                     | 280        |
| 249           | nerrirq[4]                     | 281        |
| 250           | nclusterpmuirq                 | 282        |


# Power and MCU in suspend
The MCU is turned off during sleep. I checked this by making a small
MCU program that blinks the builtin LED, then suspended the whole
system with `systemctl suspend`. RK3566 TF-A (a version is available
at
[https://review.trustedfirmware.org/c/TF-A/trusted-firmware-a/+/16952](https://review.trustedfirmware.org/c/TF-A/trusted-firmware-a/+/16952))
requests bus idle on suspend for `BIU_BUS`, which contains the MCU and
many other peripherals, as well as `BIU_SECURE_FLASH`, which contains
`SYSTEM_SRAM` and other memory controllers. I can confirm this is why
the MCU stops by writing to `PMU_BUS_IDLE_SFTCON0` from the kernel,
which causes the MCU to stop (as well as the rest of the system to
hang, but that's to be expected).

TF-A is responsible for the low-level power management on the RK3566,
and the code in `pmu_pd_powerdown_config()` in `pmu.c` in the above
pull request sets bus idle and prepares some of the system for
powerdown/suspend. **Therefore: you cannot use the MCU during sleep on
the RK3566.**

This code is very likely from Rockchip themselves, as the PR is
signed-off-by `shengfei Xu <xsf@rock-chips.com>`. Personally, I find
it impressive that ~1500 lines of hardware configs are enough to port
TF-A to a new platform.
	
# MCU clock
I think the MCU is clocked rather fast, but I don't know how fast. 
The MCU sits in the `ALIVE` power domain, in the `VD_LOGIC` voltage
domain, under the `BIU_BUS` bus interface unit. Most simpler I/O
peripherals (UART, I2C, etc.) are in this same group as well.

## `CRU_CLKSEL_CON50`
| Bit   | Attr | Reset value | Description                  |
|-------|------|-------------|------------------------------|
| 31:16 | RW   | 0x0000      | Write enable for low 16 bits |
| 15:6  | RO   | 0x000       | reserved                     |
| 5:4   | RW   | 0x0         | `pclk_bus_sel`               |
| 3:2   | RO   | 0x0         | reserved                     |
| 1:0   | RW   | 0x0         | `aclk_bus_sel`               |

Options for `pclk_bus_sel`:
- 0b00: `clk_gpll_div_100m`
- 0b01: `clk_gpll_div_75m`
- 0b10: `clk_cpll_div_50m`
- 0b11: `xin_osc0_func_mux`

Options for `aclk_bus_sel`:
- 0b00: `clk_gpll_div_200m`
- 0b01: `clk_gpll_div_150m`
- 0b10: `clk_cpll_div_100m`
- 0b11: `xin_osc0_func_mux`

Fractional PLLs are APLL, PPLL, HPLL, DPLL, CPLL, and GPLL. Integer
PLLs are MPLL, NPLL, and VPLL. Presumably `pclk` is PPLL (maybe the
`FOUTPOSTDIV` output, which comes after the division) and `aclk` is
APLL.

# Mailbox
The mailbox is nothing more than a set of 32-bit registers. Every
register can be accessed by both the ARM cores and the MCU. From Linux
under Plebian with kernel 6.1.0-9, at least, the clock gate on the
mailbox is enabled by default, so it must be disabled before the
mailbox can be used. 

The interrupt documentation in the Mailbox section of the RK3568 TRM
seems to be ripped directly from the RK3368, as the interrupt numbers
haven't been updated.

The mailbox has a "four elements combined interrupt" that is generated
when all 4 bits of the `INTEN` register for one direction of the
mailbox are enabled. I think that you have to write to all the CMD/DAT
registers (in that order, all 4 pairs) to generate this
interrupt. Furthermore, if it is a separate signal, it doesn't appear
to be connected to any interrupt controller. (The Mailbox section of
the TRM is clearly not up-to-date for the RK3566---it mentions a
Cortex-A7, for example).

## Mailbox through kernel driver
The `rockchip-mailbox` driver in the Linux kernel source implements a
mailbox interface for the RK3368, at least according to the
`compatible` field. This driver uses the same register offsets as the
RK3368, and `rk3566_mbox.dts` is device tree overlay source that
*should* work with the RK3566---it's straight from the Rockchip BSP
Linux source---but I still need to test it.

**Update:** This would probably work if the Rockchip mailbox driver
were compiled into the Plebian kernel. I discussed this on the bridged
#Quartz64 channel and `diederik` made a pull request to Debian to
enable the config option `CONFIG_ROCKCHIP_MBOX`. However, they said
that it won't make it into Debian Bookworm, so I think mailbox work
through the kernel driver will have to wait a bit.


# ahb2axi
## Overview
`ahb2axi` is some kind of IP block, maybe to connect AHB/APB
peripherals (there are a lot) to the AXI bus on the ARM cores or the
MCU.

There are two ahb2axi interrupts: `ahb2axi_i` and `ahb2axi_d`. These
interrupts and the following registers are the only mention of the
term `ahb2axi` in either part of the RK3566 TRM.

## Registers
| Register       | Bit | Attr | Reset Value | Name                      |
|----------------|-----|------|-------------|---------------------------|
| `GRF_SOC_CON3` | 15  | RW   | 0x0         | `mcu_ahb2axi_d_buf_flush` |
| `GRF_SOC_CON3` | 14  | RW   | 0x0         | `mcu_ahb2axi_i_buf_flush` |
|----------------|-----|------|-------------|---------------------------|
| `GRF_SOC_CON6` | 9   | RW   | 0x0         | `ahb2axi_d_rd_clean`      |
| `GRF_SOC_CON6` | 8   | RW   | 0x0         | `ahb2axi_i_rd_clean`      |
| `GRF_SOC_CON6` | 7:0 | RW   | 0xff        | `ahb2axi_d_timeout`       |

# Oddities
- Reading 0xFE7A0000 from ARM makes a segfault and from MCU causes a
  crash. Reading 0xFE7B0000 and up through this reserved region seems
  to be okay.

# tools
`27_26_25_24 23_22_21_20 19_18_17_16 15_14_13_12 11_10_9_8 7_6_5_4 3_2_1_0`

- `USER_LED2` is the blue one and sits on `GPIO0_A0_d`. The green LED
  is `VCC3V3_SYS`.
