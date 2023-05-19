#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>

#include "rsm.h"


#define GRF_CHIP_ID (0x800 + 0xFDC60000) // 32 bits, should be 0x3566
#define GRF_CHIP_ID_LEN (4)

#define SYS_GRF_BASE (0xFDC60000)

#define GRF_SOC_CON3 (SYS_GRF_BASE + 0x50C)
#define GRF_SOC_CON3_LEN (4)
#define GRF_SOC_CON4 (SYS_GRF_BASE + 0x510)
#define GRF_SOC_CON4_LEN (4)

#define CRU_BASE (0xFDD20000)

#define CRU_SOFTRST_CON26 (CRU_BASE + 0x0468)
#define CRU_SOFTRST_CON26_LEN (4)

#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)
#define MAILBOX_REG_LEN (4)

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

static int __init read_mimpid_init(void) {
	void* __iomem system_sram;
	void* __iomem grf_soc_con3;
	void* __iomem grf_soc_con4;
	void* __iomem cru_softrst_con26;
	void* __iomem mailbox_b2a_cmd_0;
	void* __iomem mailbox_b2a_dat_0;

	// put MCU in reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	if (cru_softrst_con26 == NULL) {
		pr_err("cru_softrst_con26 == NULL!");
		return 1;
	}
	// this convienent hex number from the rockchip bsp u-boot
	iowrite32(0x04000400, cru_softrst_con26);
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	
	pr_info("ENTER: read_mimpid_init()");
	
	// copy RISC-V program into SYSTEM_SRAM
	system_sram = reserve_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);
	if (system_sram == NULL) {
		pr_err("system_sram == NULL!");
		return 1;
	}
	pr_info("SRAM address 0xfdcc1000 before MCU runs: %x\n", ioread32(system_sram + 0x1000));
	pr_info("SRAM before write: %x\n", ioread32(system_sram));
	memcpy_toio(system_sram, read_store_mimpid_bin, read_store_mimpid_bin_len);
	pr_info("SRAM after write: %x\n", ioread32(system_sram));
	

	// write the high bits of the boot address to the boot address register
	grf_soc_con4 = reserve_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);
	if (grf_soc_con4 == NULL) {
		pr_err("grf_soc_con4 == NULL!");
		return 1;
	}
	pr_info("GRF_SOC_CON4 before write: %x\n", ioread32(grf_soc_con4));
	iowrite32((0xffff0000) | (SYSTEM_SRAM_BASE >> 16), grf_soc_con4);
	pr_info("GRF_SOC_CON4 after write: %x\n", ioread32(grf_soc_con4));
	release_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);

	udelay(10); // Rockchip BSP u-boot does this after writing the address

	// take MCU out of reset
	cru_softrst_con26 = reserve_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);
	if (cru_softrst_con26 == NULL) {
		pr_err("cru_softrst_con26 == NULL!");
		return 1;
	}	
	// this register defaults to 0x400, so we can safely trample the whole thing
	pr_info("CRU_SOFTRST_CON26 before write: %x\n", ioread32(cru_softrst_con26));
	iowrite32((0x04000000), cru_softrst_con26); // clear all reset bits, enabling the MCU to run
	pr_info("CRU_SOFTRST_CON26 after write: %x\n", ioread32(cru_softrst_con26));
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);

	mdelay(50); // let code run (idk what the clock speed is, though it's probably high)


	pr_info("SRAM address 0xfdcc1000 after MCU runs: %x\n", ioread32(system_sram + 0x1000));
	release_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);

	
	// read mailbox register
/*	mailbox_b2a_cmd_0 = reserve_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	if (mailbox_b2a_cmd_0 == NULL) {
		pr_err("mailbox_b2a_cmd_0 == NULL!");
		return 1;
	}

	pr_info("MAILBOX_B2A_CMD_0: %x\n", ioread32(mailbox_b2a_cmd_0));
	iowrite32(0xDEADBEEF, mailbox_b2a_cmd_0);
	pr_info("MAILBOX_B2A_CMD_0: %x\n", ioread32(mailbox_b2a_cmd_0));
	iowrite32(0x0, mailbox_b2a_cmd_0);
	release_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
*/	
	return 0;
}

static void __exit read_mimpid_exit(void) {
	//pr_info("ENTER: read_mimpid_exit()");
	//pr_info("EXIT: read_mimpid_exit()");
}

module_init(read_mimpid_init);
module_exit(read_mimpid_exit);

MODULE_LICENSE("GPL");

	// set MCU to use AXI bus (SYSTEM_SRAM sits on AXI)
	// ... but mailbox sits on APB...
	/*grf_soc_con3 = reserve_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	if (grf_soc_con3 == NULL) {
		pr_err("grf_soc_con3 == NULL!");
		return 1;
	}
	pr_info("GRF_SOC_CON3 before write: %x\n", ioread32(grf_soc_con3));
	iowrite32(0x20002000, grf_soc_con3);
	pr_info("GRF_SOC_CON3 after write: %x\n", ioread32(grf_soc_con3));
	release_iomem((phys_addr_t)GRF_SOC_CON3, GRF_SOC_CON3_LEN);
	*/
