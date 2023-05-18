Mailbox is for certain types of communication. However, the MCU has
full memory access, so that seems like a better place to start.

# MCU related registers
These registers *control* the MCU in the context of the RK3566 as a
whole. The MCU also has dedicated registers that control subsystems
within it. 

When write enable bits are 1, write access is enabled to the
respective bit.


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


# MCU Details
Fortunately, this is one aspect of the MCU subsystem that is well
documented. The MCU has a RV32IMC (32-bit integer RISC-V core with
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

## MCU CPU control registers
These are globally mapped, some are RISC-V CSRs and I think some are not.

RISC-V names and privileges are from
https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf

Privilege column comes from the link above. "MRO" means "machine
level, read-only", and "MRW" is "machine level, read-write". If this
MCU supported modes other than Machine, the privilege level of CSRs
would matter. The RK356x TRM tries to document these, but you really
have to have at least a basic level of RISC-V knowledge.

I believe that the reason that Rockchip register names are available
is so that the ARM cores can change RISC-V CSRs.
| Number | Privilege | RISC-V Name | Rockchip name       | RISC-V Description                                    |
|--------|-----------|-------------|---------------------|-------------------------------------------------------|
| 0xF11  | MRO       | `mvendorid` | `MCU_CSR_MVENDORID` | Vendor ID                                             |
| 0xF12  | MRO       | `marchid`   | `MCU_CSR_MARCHID`   | Architecture ID                                       |
| 0xF13  | MRO       | `mimpid`    | `MCU_CSR_MIMPID`    | Implementation ID                                     |
| 0xF14  | MRO       | `mhartid`   | `MCU_CSR_MHARTID`   | Hardware thread ID (`harts` are like cores)           |
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

## MCU reset


# Memory ranges
`lsmem` output:

```
RANGE                                  SIZE  STATE REMOVABLE BLOCK
0x0000000000000000-0x00000000efffffff  3.8G online       yes  0-29

Memory block size:       128M
Total online memory:     3.8G
Total offline memory:      0B
```

After this comes the register space, including both SRAMs.
# Basic code to start with
No C, just assembly. Let's read the `mimpid` register and write it to
RAM, then have Linux read it back. RISC-V assembly for this:

Good assembly reference: https://itnext.io/risc-v-instruction-set-cheatsheet-70961b4bbe8

```
  .section .text
  .globl start

  start:
	  csrr x1,mimpid ; read CSR mimpid into x1
	  li x2,$fdcc0100 ; byte $100 of system SRAM (far beyond program)
	  sw x1,x2 ; store x1 in x2

  loop:
	  j loop
```

Linker script should probably put `.text` at start of system
SRAM. 

# Accessing SRAM from userspace
Looks like we should use `mmap()`. This is from
https://community.nxp.com/t5/i-MX-Processors/i-mx28-on-chip-SRAM/m-p/377296/highlight/true.

In this case, we want to map specifically to `/dev/mem`, from address
0, with an offset of the argument `address` and a size of argument
`size`. A map can exist anywhere (often the kernel picks the location
automatically), but can exist on a file descriptor. The file
descriptor can be safely closed after the map has been created.
```c++
/*!
   \brief Function to get a mapped to pointer to a specific address area
   in memory. The address should be 4K (blocksize) aligned
   \param address The 4K aligned physical address to map to
   \param size The size in bytes to be mapped
   \return The pointer to the mapped memory, NULL if mapping fails
*/
void * NonVolStorage::MapMemory(uint32_t address, size_t size)
{
   int fd;
   void *ret_addr;

   fd = open("/dev/mem", O_RDWR | O_SYNC);

   if (fd == -1) {
      perror("open");
      return NULL;
   }

   ret_addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, address);
   if (ret_addr == MAP_FAILED) {
      perror("mmap");
      ret_addr = NULL;
   }

   if (close(fd) == -1) {
      perror("close");
   }

   return ret_addr;
}
```

This can also be used to access registers without writing a kernel
driver: https://stackoverflow.com/a/44362566.
