.global _start

_start:
	# Write 0xdeadbeef to SMC scratch register at 0xC0010108 (SCRATCH[1])
	mv a0, zero
	mv a1, zero
	lui a0, 0xc0010
	li a1, 0xdeadbeef
	sw a1, 0x108(a0)
_spin:
	j _spin
