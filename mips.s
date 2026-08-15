.text

    .globl f
f:
        # === prologue ===
    addiu $sp, $sp, -8
    move $t8, $fp
    move $fp, $sp
    sw $ra, 0($fp)
    sw $t8, 4($fp)
f.f.entry:
    li $v0, 42
        # === epilogue ===
    lw $ra, 0($fp)
    lw $t8, 4($fp)
    addiu $sp, $fp, 8
    move $fp, $t8
    jr $ra

    .globl main
main:
        # === prologue ===
    addiu $sp, $sp, -16
    move $t8, $fp
    move $fp, $sp
    sw $ra, 0($fp)
    sw $t8, 4($fp)
main.main.entry:
    jal f
    move $t0, $v0
    sw $t0, 8($fp)    # flush main.entry.call.1
    lw $v0, 8($fp)
        # === epilogue ===
    lw $ra, 0($fp)
    lw $t8, 4($fp)
    addiu $sp, $fp, 16
    move $fp, $t8
    jr $ra

