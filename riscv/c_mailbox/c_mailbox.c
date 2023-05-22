// Bare metal example for the RK3566 RISC-V MCU that communicates with
// the RK3566 mailbox, which can be read by Linux.

#include <stdint.h>

#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)

void main() {
  volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;
  *mailbox_b2a_cmd_0 = 0xDEADBEEF;

  while (1);
}
