// This is a very simple kernel module that just reads certain registers.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>


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
// rockchip-read_registers driver.
static int __init read_registers_init(void) {
	void* __iomem system_sram;
	void* __iomem grf_soc_con3;
	void* __iomem grf_soc_con4;
	void* __iomem cru_gate_con32;
	void* __iomem cru_softrst_con26;
	void* __iomem read_registers_cmd;
	void* __iomem read_registers_dat;
	
	pr_info("read_registers: ENTER read_registers_init()");
	
	cru_gate_con32 = reserve_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	pr_info("read_registers: CRU_GATE_CON32 = %x", ioread32(cru_gate_con32));
	release_iomem((phys_addr_t)CRU_GATE_CON32, CRU_GATE_CON32_LEN);
	
	// put MCU, read_registers, and INTMUX in reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	pr_info("read_registers: CRU_SOFTRST_CON26 = %x", ioread32(cru_softrst_con26));
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
		
	system_sram = reserve_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);
	pr_info("read_registers: SYSTEM_SRAM address 0x8000 = %x", ioread32(system_sram + 0x8000));
	release_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);

	grf_soc_con3 = reserve_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	pr_info("read_registers: GRF_SOC_CON3 = %x", ioread32(grf_soc_con3));
	release_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	
	grf_soc_con4 = reserve_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);
	pr_info("read_registers: GRF_SOC_CON4 = %x\n", ioread32(grf_soc_con4));
	release_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);


	/*
	read_registers_cmd = reserve_iomem((phys_addr_t)READ_REGISTERS_B2A_CMD_0, READ_REGISTERS_REG_LEN);
	read_registers_dat = reserve_iomem((phys_addr_t)READ_REGISTERS_B2A_DAT_0, READ_REGISTERS_REG_LEN);
	pr_info("read_registers: READ_REGISTERS_B2A_CMD_0 = 0x%x, READ_REGISTERS_B2A_DAT_0 = 0x%x\n",
		ioread32(read_registers_cmd), ioread32(read_registers_dat));
	release_iomem((phys_addr_t)READ_REGISTERS_B2A_CMD_0, READ_REGISTERS_REG_LEN);
	release_iomem((phys_addr_t)READ_REGISTERS_B2A_DAT_0, READ_REGISTERS_REG_LEN);*/


	return 0;
}

static void __exit read_registers_exit(void) {
}

module_init(read_registers_init);
module_exit(read_registers_exit);

MODULE_LICENSE("GPL");

