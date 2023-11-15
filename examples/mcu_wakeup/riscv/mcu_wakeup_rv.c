#include <stdint.h>

// This example currently doesn't actually do anything, because I need to find the timer base address!
#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)


#define TIMER_BASE (0xFE7F0000)
#define MCU_TIMER_CTRL (TIMER_BASE + 0x0000)
#define MCU_TIMER_DIV (TIMER_BASE + 0x0004)
// timers are rewritable
#define MCU_MTIME (TIMER_BASE + 0x0008)
#define MCU_MTIMEH (TIMER_BASE + 0x000C)
// the first six MCU registers are just memory locations, not CSRs.
#define MCU_MTIMECMP (TIMER_BASE + 0x0010)
#define MCU_MTIMECMPH (TIMER_BASE + 0x0014)

#define MCU_BASE (0xFE790000)

#define GPIO0_BASE (0xFDD60000)
// DR is low/high, DDR is input/output
#define GPIO0_SWPORT_DR_L (GPIO0_BASE + 0x0000)

#define GPIO3_BASE (0xFE760000)
#define GPIO3_SWPORT_DR_H (GPIO3_BASE + 0x0004)
#define GPIO3_SWPORT_DDR_H (GPIO3_BASE + 0x000C)

#define PMU_BASE (0xFDD90000)
#define PMU_INT_MASK_CON (PMU_BASE + 0x000C)

volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;
volatile uint32_t* mailbox_b2a_dat_0 = (uint32_t*)MAILBOX_B2A_DAT_0;

volatile uint32_t* timer_ctrl = (uint32_t*)MCU_TIMER_CTRL;
volatile uint32_t* timer_div = (uint32_t*)MCU_TIMER_DIV;
volatile uint32_t* mtimecmp = (uint32_t*)MCU_MTIMECMP;
volatile uint32_t* mtimecmph = (uint32_t*)MCU_MTIMECMPH;
volatile uint32_t* mtime = (uint32_t*)MCU_MTIME;
volatile uint32_t* mtimeh = (uint32_t*)MCU_MTIMEH;

volatile uint32_t* gpio0_swport_dr_l = (uint32_t*)GPIO0_SWPORT_DR_L;
volatile uint32_t* gpio3_swport_dr_h = (uint32_t*)GPIO3_SWPORT_DR_H;
volatile uint32_t* gpio3_swport_ddr_h = (uint32_t*)GPIO3_SWPORT_DDR_H;

volatile uint32_t* pmu_int_mask_con = (uint32_t*)PMU_INT_MASK_CON;

// Under GCC, interrupt functions must have the interrupt attribute
static void irq_entry() __attribute__ ((interrupt ("machine")));

void toggle_led() {
  /*uint32_t reg_value = *gpio3_swport_dr_h;
  // external LED is GPIO3_C4, or 5th bit of DR_H
  if (reg_value & (1 << 4)) {
    *gpio3_swport_dr_h = (1 << (16+4)) | (0 << 4);
  } else {
    *gpio3_swport_dr_h = (1 << (16+4)) | (1 << 4);
    }*/

  uint32_t reg_value = *gpio0_swport_dr_l;
  // external LED is GPIO3_C4, or 5th bit of DR_H
  if (reg_value & 1) {
    *gpio0_swport_dr_l = (1 << 16) | 0;
  } else {
    *gpio0_swport_dr_l = (1 << 16) | 1;
  }
}

// It seems that writing to a GPIO pin, like to enable an LED, competes with Linux
// because the states conflict. 

// alignment for mtvec.BASE
#pragma GCC push_options
// align on a 64-byte boundary because mtvec.BASE is only upper 26
// bits (32-26 = 6, 2**6 = 64)
#pragma GCC optimize ("align-functions=64")
void irq_entry() {
  uint32_t cause;

  // in direct mode, you have to read mcause
  __asm__ volatile ("csrr     %0,mcause"
		    : "=r" (cause) // output into cause
		    : // no input
		    : // no clobber
		    );

  toggle_led();
  *mtime = 0;
  *mtimeh = 0;
  //*mtime += 0x10000;
  /*uint32_t mtie_enable = 0x80; // set bit 7 (0-indexed)
  __asm__ volatile ("csrrs    zero, mie, %0"
		    : // no output
		    : "r" (mtie_enable) // write mtie_enable to %0
		    : // no clobber
		    );
  
  uint32_t mie_enable = 0b1000;
  __asm__ volatile ("csrrs    zero, mstatus, %0"
		    : // no output
		    : "r" (mie_enable)
		    :
		    );*/
  /*
  // wakeup PMU
  *pmu_int_mask_con = (1 << (15+16)) | (1 << 15);
  */

  // GCC puts in an mret instruction for us
}
#pragma GCC pop_options

int main() {
  

  *mailbox_b2a_cmd_0 = 0xdeadbeef;
  // set external LED pin to output
  *gpio3_swport_ddr_h = (1 << (16+4)) | (1 << 4);
  
  // I can't get the timer to work, but this does.
  while (1) {
    toggle_led();
    for (uint32_t i = 0; i < 0xafff; i++);
    toggle_led();
    for (uint32_t i = 0; i < 0xafff; i++);
  }

  return 0;
}
