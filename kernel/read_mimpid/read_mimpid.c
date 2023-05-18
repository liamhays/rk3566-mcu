#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/ioport.h>
// Regmaps are the preferred way to deal with memory-mapped system
// registers.
/*static const struct regmap_config mcu_csr_mimpid_regmap_cfg = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4, // length between start of registers
	.name = "MCU_CSR_MIMPID",
};

static const struct resource rk3566_mcu_resources[] = {
	{
		.start = 0xFE790000,
		.end = 0xFE79FFFF,
		.flags = IORESOURCE_MEM,
		.name = "mcu-space"
	},
};

static const struct platform_device rk3566_mcu = {
	.name = "rk3566_mcu",
*/

#define GRF_CHIP_ID (0x800 + 0xFDC60000) // 32 bits, should be 0x3566
#define GRF_CHIP_ID_LEN (4)

// 99% chance that the reason this fails on read is because it's not aligned. The "offset" value lied!
#define MCU_CSR_MIMPID (0x0 + 0xFE790000)
#define MCU_CSR_MIMPID_LEN (4)
#define REG MCU_CSR_MIMPID
#define REG_LEN MCU_CSR_MIMPID_LEN

u32 safe_read_io_u32(u32 reg) {
	pr_info("ENTER: safe_read_io_u32()");
	if (!request_mem_region(reg, REG_LEN, "read_mimpid")) {
		pr_err("couldn't request memory region!\n");
		return 1;
	}
	void* __iomem base = ioremap(reg, 4);
	if (!base) {
		pr_err("couldn't remap region!\n");
		return 1;
	}
	u32 data = ioread32(base);
	iounmap(base);
	release_mem_region(reg, 4);
	return data;
}
static int __init read_mimpid_init(void) {
	pr_info("ENTER: read_mimpid_init()");
	//void* __iomem base_addr = devm_platform_ioremap_resource(
	if (!request_mem_region(REG, REG_LEN, "read_mimpid")) {
		pr_err("couldn't request memory region!\n");
		return 1;
	}
	pr_info("requested memory region\n");
	void* __iomem base = ioremap(REG, REG_LEN);
	if (!base) {
		pr_err("couldn't remap region!\n");
		return 1;
	}
	pr_info("remapped memory region\n");
	
	pr_info("data: %x\n", ioread32(base));

	iounmap(base);

	pr_info("unmapped memory region\n");
	
	release_mem_region(REG, REG_LEN);

	pr_info("released memory region\n");
	return 0;
}

static void __exit read_mimpid_exit(void) {
	pr_info("ENTER: read_mimpid_exit()");
	pr_info("EXIT: read_mimpid_exit()");
}

module_init(read_mimpid_init);
module_exit(read_mimpid_exit);

MODULE_LICENSE("GPL");

