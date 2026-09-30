.section entry, "ax"
.globl _start
.type _start, @function

_start:
  mv s0, zero
  li t0, (3 << 13)
  csrs mstatus, t0
  la sp, _stack_pointer
  jal call_main
