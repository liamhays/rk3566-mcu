#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>

#include "riscv/build/soft_irq_rv_bin.h"

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
#define MAILBOX_A2B_INTEN (MAILBOX_BASE + 0x0000)
#define MAILBOX_A2B_STATUS (MAILBOX_BASE + 0x0004)
#define MAILBOX_A2B_CMD_0 (MAILBOX_BASE + 0x0008)
#define MAILBOX_A2B_DAT_0 (MAILBOX_BASE + 0x000C)

#define MAILBOX_B2A_INTEN (MAILBOX_BASE + 0x0028)
#define MAILBOX_B2A_STATUS (MAILBOX_BASE + 0x002C)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)
#define MAILBOX_REG_LEN (4)

#define MCU_INTC (0xFE798000)
#define MCU_INTC_LEN (0xFFFF)

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
static int __init mailbox_init(void) {
	void* __iomem system_sram;
	void* __iomem grf_soc_con3;
	void* __iomem grf_soc_con4;
	void* __iomem cru_gate_con32;
	void* __iomem cru_softrst_con26;
	void* __iomem mailbox_cmd;
	void* __iomem mailbox_dat;
	
	pr_info("mailbox: ENTER mailbox_init()");
	
	// Disable clock gate on mailbox and INTMUX
	cru_gate_con32 = reserve_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	// write 0 to bits 14 and 15 and enable write to those bits
	iowrite32((1 << (15+16)) | (1 << (14+16)), cru_gate_con32);
	release_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	
	// put MCU, mailbox, and INTMUX in reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	iowrite32(0xffff0000 | (1 << 12) | (1 << 11) | (1 << 10), cru_softrst_con26);
	
	pr_info("mailbox: MCU, mailbox, INTMUX reset");
	
	// copy RISC-V program into SYSTEM_SRAM
	system_sram = reserve_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);
	// zero out SRAM
	for (int i = 0; i < 0xffff; i += 4) {
	  iowrite32(0, system_sram + i);
	}
	memcpy_toio(system_sram, soft_irq_rv_bin, soft_irq_rv_bin_len);
	release_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);

	// set the MCU to use the AXI bus (just for fun in this example)
	grf_soc_con3 = reserve_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	iowrite32((1 << (13+16)) | (1 << 13), grf_soc_con3);
	release_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	
	// write the high bits of the boot address to the boot address register
	grf_soc_con4 = reserve_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);
	iowrite32((0xffff0000) | (SYSTEM_SRAM_BASE >> 16), grf_soc_con4);
	release_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);

	udelay(10); // Rockchip BSP u-boot does this after writing the address
	
	// now enable MCU, INTMUX, and mailbox
	iowrite32(0xffff0000, cru_softrst_con26);
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);

	mdelay(200); // let MCU run

	// assert interrupt request
	grf_soc_con3 = reserve_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	iowrite32((1 << (12+16)) | (1 << 12), grf_soc_con3);
	release_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	
	mdelay(100); // let MCU process interrupt

	// read mailbox register
	mailbox_cmd = reserve_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	mailbox_dat = reserve_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);
	pr_info("mailbox: MAILBOX_B2A_CMD_0 = 0x%x, MAILBOX_B2A_DAT_0 = 0x%x\n",
		ioread32(mailbox_cmd), ioread32(mailbox_dat));
	release_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	release_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);


	return 0;
}

static void __exit mailbox_exit(void) {
	//pr_info("ENTER: read_mimpid_exit()");
	//pr_info("EXIT: read_mimpid_exit()");
}

module_init(mailbox_init);
module_exit(mailbox_exit);

MODULE_LICENSE("GPL");

