#
# interrupts and exceptions while in supervisor
# mode come here.
#
# the current stack is a kernel stack.
# push all registers, call kerneltrap().
# when kerneltrap() returns, restore registers, return.
#
.globl kerneltrap
.globl kernelvec
.align 4
kernelvec:
    # TODO: implement kernelvec
    sret

#
# machine-mode timer interrupt.
#
.globl timervec
.align 4
timervec:
    #TODO: implement timervec
    mret