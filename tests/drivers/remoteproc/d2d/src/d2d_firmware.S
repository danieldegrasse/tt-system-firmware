.global _start

_start:
	# Write 0xdeadbeef to D2D SRAM at 0x1000
	mv a0, zero
	mv a1, zero
	lui a0, 0x1
	li a1, 0xdeadbeef
	sw a1, 0x0(a0)
_spin:
	j _spin
