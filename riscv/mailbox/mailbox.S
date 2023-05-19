	.section .text
	.globl start

	.equ SYSTEM_SRAM_BASE,0xfdcc0000
	.equ MAILBOX_BASE_ADDRESS,0xfe780000
	.equ MAILBOX_B2A_CMD_0,(MAILBOX_BASE_ADDRESS + 0x0030)
	.equ MAILBOX_B2A_DAT_0,(MAILBOX_BASE_ADDRESS + 0x0034)

	/* Command and data words don't actually have any significant
	meaning, aside from the fact that you have to write to
	first the command and then the data register to generate an
	interrupt. */

start:
	# now we're writing DEADBEEF to the MAILBOX_B2A_CMD_0 

	li x2,MAILBOX_B2A_CMD_0
	li x3,0xcdcdcdcd
	sw x3,0(x2)


loop:
	j loop
 
