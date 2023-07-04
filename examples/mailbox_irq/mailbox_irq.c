#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>

#include "riscv/build/mailbox_irq_rv_bin.h"

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
#define MAILBOX_A2B_CMD_1 (MAILBOX_BASE + 0x0010)
#define MAILBOX_A2B_DAT_1 (MAILBOX_BASE + 0x0014)

#define MAILBOX_B2A_INTEN (MAILBOX_BASE + 0x0028)
#define MAILBOX_B2A_STATUS (MAILBOX_BASE + 0x002C)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)

#define MAILBOX_CMD MAILBOX_A2B_CMD_1
#define MAILBOX_DAT MAILBOX_A2B_DAT_1

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
	void* __iomem grf_soc_con4;
	void* __iomem cru_gate_con32;
	void* __iomem cru_softrst_con26;
	void* __iomem mailbox_cmd;
	void* __iomem mailbox_dat;
	void* __iomem mailbox_a2b_inten;
	void* __iomem mailbox_b2a_inten;
	void* __iomem mcu_intc;
	
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
	memcpy_toio(system_sram, mailbox_irq_rv_bin, mailbox_irq_rv_bin_len);
	// we're going to use SRAM later so don't free it
	
	// write the high bits of the boot address to the boot address register
	grf_soc_con4 = reserve_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);
	iowrite32((0xffff0000) | (SYSTEM_SRAM_BASE >> 16), grf_soc_con4);
	release_iomem((phys_addr_t)GRF_SOC_CON4, GRF_SOC_CON4_LEN);

	udelay(10); // Rockchip BSP u-boot does this after writing the address
	
	// take INTMUX out of reset
	iowrite32(0xffff0000 | (1 << 12) | (1 << 10), cru_softrst_con26);

	// enable all the interrupt groups
	mcu_intc = reserve_iomem((phys_addr_t)MCU_INTC, MCU_INTC_LEN);
	
	iowrite32(0xff, mcu_intc + 0x00); // group 00
	iowrite32(0xff, mcu_intc + 0x04); // group 01
	// the `edp` interrupt (assuming my table is right) is in
	//group 2 and will always fire unless you disable it.
	//iowrite32(0xff, mcu_intc + 0x08); // group 02
	iowrite32(0xff, mcu_intc + 0x0C); // group 03
	iowrite32(0xff, mcu_intc + 0x10); // group 04
	iowrite32(0xff, mcu_intc + 0x14); // group 05
	iowrite32(0xff, mcu_intc + 0x18); // group 06
	iowrite32(0xff, mcu_intc + 0x1C); // group 07
	iowrite32(0xff, mcu_intc + 0x20); // group 08
	iowrite32(0xff, mcu_intc + 0x24); // group 09
	iowrite32(0xff, mcu_intc + 0x28); // group 10
	iowrite32(0xff, mcu_intc + 0x2C); // group 11
	iowrite32(0xff, mcu_intc + 0x30); // group 12
	iowrite32(0xff, mcu_intc + 0x34); // group 13
	iowrite32(0xff, mcu_intc + 0x38); // group 14
	iowrite32(0xff, mcu_intc + 0x3C); // group 15
	iowrite32(0xff, mcu_intc + 0x40); // group 16
	iowrite32(0xff, mcu_intc + 0x44); // group 17
	iowrite32(0xff, mcu_intc + 0x48); // group 18
	iowrite32(0xff, mcu_intc + 0x4C); // group 19
	iowrite32(0xff, mcu_intc + 0x50); // group 20
	iowrite32(0xff, mcu_intc + 0x54); // group 21
	iowrite32(0xff, mcu_intc + 0x58); // group 22
	iowrite32(0xff, mcu_intc + 0x5C); // group 23
	iowrite32(0xff, mcu_intc + 0x60); // group 24
	iowrite32(0xff, mcu_intc + 0x64); // group 25
	iowrite32(0xff, mcu_intc + 0x68); // group 26
	iowrite32(0xff, mcu_intc + 0x6C); // group 27
	iowrite32(0xff, mcu_intc + 0x70); // group 28
	iowrite32(0xff, mcu_intc + 0x74); // group 29
	iowrite32(0xff, mcu_intc + 0x78); // group 30
	iowrite32(0xff, mcu_intc + 0x7C); // group 31


	// now enable MCU and mailbox
	iowrite32(0xffff0000, cru_softrst_con26);
	release_iomem((phys_addr_t)CRU_SOFTRST_CON26, CRU_SOFTRST_CON26_LEN);

	// enable mailbox interrupts for A2B_CMD_0 and A2B_DAT_0
	mailbox_a2b_inten = reserve_iomem((phys_addr_t)MAILBOX_A2B_INTEN, MAILBOX_REG_LEN);
	iowrite32(0b10, mailbox_a2b_inten);
	release_iomem((phys_addr_t)MAILBOX_A2B_INTEN, MAILBOX_REG_LEN);

	mailbox_b2a_inten = reserve_iomem((phys_addr_t)MAILBOX_B2A_INTEN, MAILBOX_REG_LEN);
	//iowrite32(0b1111, mailbox_b2a_inten);
	release_iomem((phys_addr_t)MAILBOX_B2A_INTEN, MAILBOX_REG_LEN);
	
	mdelay(200); // let MCU run
	
	// write to CMD and DAT in that order to produce an interrupt
	mailbox_cmd = reserve_iomem((phys_addr_t)MAILBOX_CMD, MAILBOX_REG_LEN);
	iowrite32(0x12345678, mailbox_cmd);
	release_iomem((phys_addr_t)MAILBOX_CMD, MAILBOX_REG_LEN);
	
	mailbox_dat = reserve_iomem((phys_addr_t)MAILBOX_DAT, MAILBOX_REG_LEN);
	iowrite32(0x90abcdef, mailbox_dat);
	release_iomem((phys_addr_t)MAILBOX_DAT, MAILBOX_REG_LEN);
	
	mdelay(50); // let MCU process interrupt

	char str[1000] = {0};
	for (int i = 0x80; i <= 0x100; i += 4) {
	  sprintf(str, "%s%x, ", str, ioread32(mcu_intc + i));
	}
	pr_info("mailbox: INTMUX flag registers: 0x%s\n", str);
	// read mailbox register
	mailbox_cmd = reserve_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	mailbox_dat = reserve_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);
	pr_info("mailbox: MAILBOX_B2A_CMD_0 = 0x%x, MAILBOX_B2A_DAT_0 = 0x%x\n",
		ioread32(mailbox_cmd), ioread32(mailbox_dat));
	release_iomem((phys_addr_t)MAILBOX_B2A_CMD_0, MAILBOX_REG_LEN);
	release_iomem((phys_addr_t)MAILBOX_B2A_DAT_0, MAILBOX_REG_LEN);

	release_iomem((phys_addr_t)SYSTEM_SRAM_BASE, SYSTEM_SRAM_LEN);
	release_iomem((phys_addr_t)MCU_INTC, MCU_INTC_LEN);
	return 0;
}

static void __exit mailbox_exit(void) {
	//pr_info("ENTER: read_mimpid_exit()");
	//pr_info("EXIT: read_mimpid_exit()");
}

module_init(mailbox_init);
module_exit(mailbox_exit);

MODULE_LICENSE("GPL");

