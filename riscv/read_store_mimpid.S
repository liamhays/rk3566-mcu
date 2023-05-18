	.section .text
	.globl start
	/* risc-v comments are actually # */
	/* my RISC-V assembly skills leave a lot to be desired...some
	   of us are used to a nice legible 68k assembly syntax */

	.equ MAILBOX_BASE_ADDRESS,0xfe780000
	.equ MAILBOX_B2A_CMD_0,(MAILBOX_BASE_ADDRESS + 0x0030)
	.equ MAILBOX_B2A_DAT_0,(MAILBOX_BASE_ADDRESS + 0x0034)
	.equ MAGIC_NUMBER,0xDEADBEEF

	/* Command and data words don't actually have any significant meaning. */
	/* LA computes an effective address (for example, to put the address of a label in a register), it's not what we want */
start:
	# world's most basic code: write 0xDEADBEEF into a known SRAM address, 0xFDCC1000

	/* load MAILBOX_B2A_CMD_0 into x2 */
	li x2,0xfdcc1000
	li x3,0xdeadbeef
	/* store 32-bit x3 in address x2 */
	sw x3,0(x2)

loop:
	j loop
