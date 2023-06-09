#include <stdint.h>

// This example currently doesn't actually do anything, because I need to find the timer base address!
#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)

// also the "INTC" base in Rockchip register space
#define MCU_BASE (0xFE790000)
// timers are memory mapped CSRs
#define MCU_TIMER_CTRL (MCU_BASE + 0x0000)
// the first six MCU registers are just memory locations, not CSRs.
#define MCU_MTIMECMP (MCU_BASE + 0x0010)
// timers are rewritable
#define MCU_MTIME (MCU_BASE + 0x0008)


volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;
volatile uint64_t* mtimecmp = (uint64_t*)MCU_MTIMECMP;
volatile uint64_t* mtime = (uint64_t*)MCU_MTIME;

// Under GCC, interrupt functions must have the interrupt attribute
static void irq_entry() __attribute__ ((interrupt ("machine")));

// alignment for mtvec.BASE
#pragma GCC push_options
// align on a 64-byte boundary because mtvec.BASE is only upper 26
// bits (32-26 = 6, 2**6 = 64)
#pragma GCC optimize ("align-functions=64")
void irq_entry() {
  // update mailbox register
  *mailbox_b2a_cmd_0 = 0xabababab;
  uint32_t cause;

  // in direct mode, you have to read mcause
  __asm__ volatile ("csrr     %0,mcause"
		    : "=r" (cause) // output into cause
		    : // no input
		    : // no clobber
		    );



}
#pragma GCC pop_options

int main() {
  // default value to make change visible
  *mailbox_b2a_cmd_0 = 0x11111111;
  
  // set vector address in mtvec and use direct mode
  uint32_t mtvec_value = (((uint32_t)&irq_entry >> 6) << 6);
  __asm__ volatile ("csrw     mtvec, %0"
		    : // no output
		    : "r" (mtvec_value) // write mtvec_value to %0
		    : // no clobber
		    );

  // enable software interrupt (msie) in mie
  uint32_t mtie_enable = 0b1000;
  __asm__ volatile ("csrrs    zero, mie, %0"
		    : // no output
		    : "r" (mtie_enable) // write mtie_enable to %0
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
