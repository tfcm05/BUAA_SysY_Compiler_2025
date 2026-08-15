.data

Global.Variable.1:
    .word  1, 2, 3, 4, 5, 6

.text

    .globl main
main:
        # === prologue ===
    addiu $sp, $sp, -24
    move $t8, $fp
    move $fp, $sp
    sw $ra, 0($fp)
    sw $t8, 4($fp)
main.main.entry:
    la $t0, Global.Variable.1
    addiu $t1, $t0, 12
    addiu $t2, $t1, 4
    lw $t3, 0($t2)
    sw $t3, 12($fp)    # flush main.entry.load.1
    sw $t2, 8($fp)    # flush main.entry.gep.1
    lw $a0, 12($fp)
    jal putint
    la $t0, Global.Variable.1
    addiu $t1, $t0, 8
    lw $t2, 0($t1)
    sw $t2, 20($fp)    # flush main.entry.load.2
    sw $t1, 16($fp)    # flush main.entry.gep.2
    lw $a0, 20($fp)
    jal putint
    li $v0, 0
        # === epilogue ===
    lw $ra, 0($fp)
    lw $t8, 4($fp)
    addiu $sp, $fp, 24
    move $fp, $t8
    jr $ra

