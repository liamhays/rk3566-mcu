#include <stdint.h>

// This example receives a mailbox interrupt generated by the
// companion kernel module. It then updates MAILBOX_B2A_CMD_0 when the
// interrupt is fired.
#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_A2B_CMD_0 (MAILBOX_BASE + 0x0008)
#define MAILBOX_A2B_DAT_0 (MAILBOX_BASE + 0x000C)
#define MAILBOX_A2B_STATUS (MAILBOX_BASE + 0x0004)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)
#define MAILBOX_B2A_STATUS (MAILBOX_BASE + 0x002C)


// also the "INTC" base in Rockchip register space
#define MCU_BASE (0xFE790000)
#define MCU_CSR_IPIC_CISV (0xBF0)
// CICSR is for the interrupt currently in service
#define MCU_CSR_IPIC_CICSR (0xBF1)
#define MCU_CSR_IPIC_IPR (0xBF2)
#define MCU_CSR_IPIC_ISVR (0xBF3)
#define MCU_CSR_IPIC_EOI (0xBF4)
#define MCU_CSR_IPIC_SOI (0xBF5)
#define MCU_CSR_IPIC_IDX (0xBF6)
// ICSR configures the interrupt specified by IPIC_IDX
#define MCU_CSR_IPIC_ICSR (0xBF7)

// kernel module will have to control INTMUX clock and reset
#define INTMUX_BASE (?) // what is the base address?
#define INTMUX_INT_MASK_GROUP25 (INTMUX_BASE + 0x0064)
#define INTMUX_INT_FLAG_LEVEL2 (INTMUX_BASE + 0x0100)

volatile uint32_t* intmux_int_mask_group25 = (uint32_t*)INTMUX_INT_MASK_GROUP25;
volatile uint32_t* intmux_int_flag_level2 = (uint32_t*)INTMUX_INT_MASK_LEVEL2;

volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;
volatile uint32_t* mailbox_b2a_dat_0 = (uint32_t*)MAILBOX_B2A_DAT_0;
volatile uint32_t* mailbox_a2b_status = (uint32_t*)MAILBOX_A2B_STATUS;

// Under GCC, interrupt functions must have the interrupt attribute
static void irq_entry() __attribute__ ((interrupt ("machine")));

// alignment for mtvec.BASE
#pragma GCC push_options
// align on a 64-byte boundary because mtvec.BASE is only upper 26
// bits (32-26 = 6, 2**6 = 64)
#pragma GCC optimize ("align-functions=64")
void irq_entry() {
  // tell the IPIC that we're entering interrupt handling
  __asm__ volatile ("csrw    %[MCU_CSR_IPIC_SOI],%0"
		    : // no input
		    : "r" (0xf) // write any value to start interrupt processing
		    : // no clobber
		    );

  // clear IPIC interrupt pending flag
  __asm__ volatile ("csrw    %[MCU_CSR_IPIC_CICSR],%0"
		    : // no input
		    : "r" (0) // clear pending and disable interrupt
		    : // no clobber
		    );
  uint32_t cause;

  // in direct mode, you have to read mcause
  __asm__ volatile ("csrr     %0,mcause"
		    : "=r" (cause) // output into cause
		    : // no input
		    : // no clobber
		    );
  
  // clear the mailbox interrupt by writing 1 to bit 0 of MAILBOX_A2B_STATUS  
  *mailbox_a2b_status = 1;

  // update B2A_CMD_0 to notify kernel module
  *mailbox_b2a_cmd_0 = 0xabababab;
  
  __asm__ volatile ("csrw    %[MCU_CSR_IPIC_EOI],%0"
		    : // no input
		    : "r" (0xf) // write any value to end interrupt processing
		    :
		    );
}
#pragma GCC pop_options

int main() {
  // set vector address in mtvec and use direct mode (INTMUX
  // connection to IPIC lines is unclear)
  uint32_t mtvec_value = (((uint32_t)&irq_entry >> 6) << 6);
  __asm__ volatile ("csrw     mtvec, %0"
		    : // no output
		    : "r" (mtvec_value) // write mtvec_value to %0
		    : // no clobber
		    );
  
		      
  // enable external interrupts in mie
  uint32_t meie_enable = (1 << 11); // set bit 11 (0-indexed)
  __asm__ volatile ("csrrs    zero, mie, %0"
		    : // no output
		    : "r" (mtie_enable) // write mtie_enable to %0
		    : // no clobber
		    );

  // mailbox A2B interrupts are enabled by kernel module
  
  // A2B interrupt is *probably* `mailbox_ca55[0]`, which is GIC
  // interrupt 215. This maps to INTMUX_INT_MASK_GROUP25 bit 6 (0-indexed).
  *intmux_int_mask_group25 = (1 << 6);

  // we are using IPIC interrupt 0. Is this correct? No idea!
  // IPIC_IDX defaults to 0.
  // interrupt enable is bit 1 (0-indexed)
  uint32_t cicsr_enable_interrupt = 1 << 1;
  __asm__ volatile ("csrw    %[MCU_CSR_IPIC_CICSR], %1"
		    : // no output
		    : "r" (cicsr_enable_interrupt)
		    : // no clobber
		    );

  
  // enable global interrupts
  uint32_t mie_enable = 0b1000;
  __asm__ volatile ("csrrs    zero, mstatus, %0"
		    : // no output
		    : "r" (mie_enable)
		    :
		    );
  

  while (1);

  return 0;
}
