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

int main() {
  __asm__ volatile ("wfi" : : :);
  
  while (1);

  return 0;
}
