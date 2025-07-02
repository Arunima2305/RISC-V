.data
n: .word 3
arr: .word 4 6 7 

.text
lui x5 65536
lw x6 0(x5)
addi x5 x5 4
addi x11 x11 0

loop : beq x6 x0 exit
    lw x8 0(x5)
    addi x9 x0 2
    rem x10 x11 x9
    beq x10 x0 case2
    addi x8 x8 -1
    jal x0 end
    case2: addi x8 x8 1
    end:
    sw x8 0(x5)
    addi x5 x5 4
    addi x6 x6 -1
    addi x11 x11 1
jal x0 loop
exit: