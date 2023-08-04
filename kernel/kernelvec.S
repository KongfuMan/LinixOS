        #
        # machine-mode timer interrupt.
        #
.globl timervec
.align 4
timervec:
    mret