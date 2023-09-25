#include <stdint.h>

#define MAILBOX_BASE (0xFE780000)
#define MAILBOX_A2B_CMD_0 (MAILBOX_BASE + 0x0008)
#define MAILBOX_B2A_CMD_0 (MAILBOX_BASE + 0x0030)
#define MAILBOX_B2A_DAT_0 (MAILBOX_BASE + 0x0034)

volatile uint32_t* mailbox_a2b_cmd_0 = (uint32_t*)MAILBOX_A2B_CMD_0;
volatile uint32_t* mailbox_b2a_cmd_0 = (uint32_t*)MAILBOX_B2A_CMD_0;


int main() {
    // get address from mailbox a2b_cmd_0
    uint32_t addr = *mailbox_a2b_cmd_0;
    uint32_t* p = (uint32_t*)addr;

    // now write the value at that address to b2a_cmd_0
    *p = 0x1e4f8a9b;
    *mailbox_b2a_cmd_0 = *p;

    while (1);
    
    return 0;
}
