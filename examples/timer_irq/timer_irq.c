#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>

// I *know* that storing data in header files is not optimal, since if
// you get a lot of data it takes a long time to recompile. but it's
// like 30 bytes.
#include "riscv/build/timer_irq_rv_bin.h"

#define SYS_GRF_BASE (0xFDC60000)

#define GRF_SOC_CON3 (SYS_GRF_BASE + 0x50C)
#define GRF_SOC_CON3_LEN (4)
#define GRF_SOC_CON4 (SYS_GRF_BASE + 0x510)
#define GRF_SOC_CON4_LEN (4)

#define CRU_BASE (0xFDD20000)

#define CRU_GATE_CON32 (CRU_BASE + 0x380)
#define CRU_GATE_CON32_LEN (4)
#define CRU_SOFTRST_CON26 (CRU_BASE + 0x0468)
#define CRU_SOFTRST_CON26_LEN (4)

#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)
#define MAILBOX_B2A_STATUS (MAILBOX_BASE + 0x002C)
#define MAILBOX_B2A_INTEN (MAILBOX_BASE + 0x0028)
#define MAILBOX_ATOMIC_LOCK_0 (MAILBOX_BASE + 0x0100)
#define MAILBOX_REG_LEN (4)

#define RESERVED_BASE (0xFE7A0000)

#define SYSTEM_SRAM_BASE (0xFDCC0000)
#define SYSTEM_SRAM_LEN (0xFFFF)

void* __iomem reserve_iomem(phys_addr_t addr, resource_size_t size) {
	void* __iomem base;
	
	if (!request_mem_region(addr, size, "read_mimpid")) {
		pr_err("couldn't request region!\n");
		return NULL;
	}
	base = ioremap(addr, size);
	if (!base) {
		pr_err("couldn't remap region!\n");
		return NULL;
	}

	return base;
}

void release_iomem(phys_addr_t addr, resource_size_t size) {
	iounmap((void* __iomem)addr);
	release_mem_region(addr, size);
}

// The issue with the mailbox registers not working was that the clock
// gate was enabled. This is surely taken care of by the
// rockchip-mailbox driver.
static int __init timer_irq_init(void) {
	void* __iomem system_sram;
	void* __iomem grf_soc_con4;
	void* __iomem cru_gate_con32;
	void* __iomem cru_softrst_con26;
	void* __iomem mailbox_b2a_cmd_0;
	void* __iomem mailbox_b2a_dat_0;
	
	pr_info("timer_irq: ENTER mailbox_init()");

	// Accessing this reserved section appears to cause a segfault
	/*scratch = reserve_iomem((phys_addr_t)RESERVED_BASE, 4);
	pr_info("timer_irq: reserved 0: %d\n", ioread32(scratch));
	release_iomem((phys_addr_t)RESERVED_BASE, 4);*/
	
	// Disable clock gate on mailbox
	cru_gate_con32 = reserve_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	iowrite32(0x80000000 | 0x0000, cru_gate_con32); // only write to bit 15
	release_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	
	// put MCU, mailbox, and INTMUX in reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	iowrite32(0xffff0000 | (1 << 12) | (1 << 11) | (1 << 10), cru_softrst_con26);
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	
	pr_info("timer_irq: MCU and mailbox reset");
	
	// copy RISC-V program into SYSTEM_SRAM
	system_sram = reserve_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);
	memcpy_toio(system_sram, timer_irq_rv_bin, timer_irq_rv_bin_len);
	// we're going to use SRAM later
	
	// write the high bits of the boot address to the boot address register
	grf_soc_con4 = reserve_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);
	iowrite32((0xffff0000) | (SYSTEM_SRAM_BASE >> 16), grf_soc_con4);
	release_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);

	udelay(10); // Rockchip BSP u-boot does this after writing the address
	
	// take MCU, mailbox, and INTMUX out of reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	// this register defaults to 0x400, so we can safely trample the whole thing
	iowrite32((0xffff0000), cru_softrst_con26); // clear all reset bits, enabling the MCU to run
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);

	mdelay(50); // let code run (idk what the clock speed is, though it's probably high)

	// read mailbox register
	mailbox_b2a_cmd_0 = reserve_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	mailbox_b2a_dat_0 = reserve_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);
	u32 mailbox_reg;
	// mailbox register should be about 0x1000000, that's what mtimecmp is set to.
	while ((mailbox_reg = ioread32(mailbox_b2a_cmd_0)) == 0x11111111); // wait for timer interrupt
	pr_info("timer_irq: MCU interrupt fired, MAILBOX_B2A_CMD_0 after MCU runs = 0x%x", ioread32(mailbox_b2a_cmd_0));
	release_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	release_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);


	release_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);

	return 0;
}

static void __exit timer_irq_exit(void) {
}

module_init(timer_irq_init);
module_exit(timer_irq_exit);

MODULE_LICENSE("GPL");

