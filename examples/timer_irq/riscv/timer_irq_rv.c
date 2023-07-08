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


volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;
volatile uint32_t* mailbox_b2a_dat_0 = (uint32_t*)MAILBOX_B2A_DAT_0;

volatile uint32_t* timer_ctrl = (uint32_t*)MCU_TIMER_CTRL;
volatile uint32_t* timer_div = (uint32_t*)MCU_TIMER_DIV;
volatile uint32_t* mtimecmp = (uint32_t*)MCU_MTIMECMP;
volatile uint32_t* mtimecmph = (uint32_t*)MCU_MTIMECMPH;
volatile uint32_t* mtime = (uint32_t*)MCU_MTIME;
volatile uint32_t* mtimeh = (uint32_t*)MCU_MTIMEH;

// Under GCC, interrupt functions must have the interrupt attribute
static void irq_entry() __attribute__ ((interrupt ("machine")));

// alignment for mtvec.BASE
#pragma GCC push_options
// align on a 64-byte boundary because mtvec.BASE is only upper 26
// bits (32-26 = 6, 2**6 = 64)
#pragma GCC optimize ("align-functions=64")
void irq_entry() {
  //*timer_ctrl = 0;
  uint32_t cause;

  // in direct mode, you have to read mcause
  __asm__ volatile ("csrr     %0,mcause"
		    : "=r" (cause) // output into cause
		    : // no input
		    : // no clobber
		    );
  // mip.mti will apparently be cleared by a write to mtimecmp
  // in this case, disable the comparator
  __asm__ volatile ("la t0,0xfe790010\nsw zero, 0(t0)"
		    : : :);

  // update mailbox register
  
  *mailbox_b2a_cmd_0 = *mtime;

}
#pragma GCC pop_options

int main() {
  // timer defaults to enabled

  // default value
  *mailbox_b2a_cmd_0 = *timer_ctrl;//0x11111111;
  
  *timer_ctrl = 0; // disable timer
  // set the divider to something that makes it a little slower
  *timer_div = 30;
  
  // writing all 1s prevent problems caused by intermediate values
  *mtimecmph = 0xffffffff;
  *mtimecmph = 0;
  *mtimecmp = 0x1000000;

  // reset mtime
  *mtimeh = 0;
  *mtime = 0;

  // now configure system interrupts
  // set vector address in mtvec and use direct mode
  uint32_t mtvec_value = (((uint32_t)&irq_entry >> 6) << 6);
  __asm__ volatile ("csrw     mtvec, %0"
		    : // no output
		    : "r" (mtvec_value) // write mtvec_value to %0
		    : // no clobber
		    );

  // enable timer interrupt in mie
  uint32_t mtie_enable = 0x80; // set bit 7 (0-indexed)
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
  
  // re-enable the timer
  *timer_ctrl = 1;

  // wait for interrupt (could also use `wfi`)
  while (1);

  return 0;
}
