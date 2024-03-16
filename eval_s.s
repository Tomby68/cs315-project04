.global eval_s

# a0 - char *expr_str
# a1 - int pos

eval_s:
    addi sp, sp, -16
    sd ra, (sp)
    
    addi a1, sp, 8

    sw zero, (a1)

    call expression_s

    ld ra, (sp)
    addi sp, sp, 16
    ret

expression_s:               # a0 - char *expr_str, a1 - int pos
    addi sp, sp, -64
    sd ra, (sp)

    sd a0, 8(sp)
    sd a1, 16(sp)

    call term_s

    mv t0, a0               # t0 - int value
    ld a0, 8(sp)
    ld a1, 16(sp)

expression_s_while_check1:
    lw t2, (a1)              # t2 - int pos
    add t3, a0, t2          
    lb t1, (t3)              # t1 - char token
    
    li t3, '+'
    bne t1, t3, expression_s_while_check2
    j expression_s_while

expression_s_while_check2:
    li t3, '-'
    bne t1, t3, expression_s_done

expression_s_while:
    addi t2, t2, 1            # update int pos in memory
    sw t2, (a1)

expression_s_while_add:
    li t3, '+'
    bne t1, t3, expression_s_while_subtract
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)

    call term_s

    ld t0, 24(sp)
    add t0, t0, a0
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    
    j expression_s_while_check1

expression_s_while_subtract:
    li t3, '-'
    bne t1, t3, expression_s_while_check1
    
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)

    call term_s

    ld t0, 24(sp)
    sub t0, t0, a0
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    
    j expression_s_while_check1

expression_s_done:
    mv a0, t0
    ld ra, (sp)
    addi sp, sp, 64
    ret

term_s:                     # a0 - char *expr_str, a1 - int pos
    addi sp, sp, -64
    sd ra, (sp)

    sd a0, 8(sp)
    sd a1, 16(sp)

    call factor_s

    mv t0, a0               # t0 - int value
    ld a0, 8(sp)
    ld a1, 16(sp)

term_s_while_check1:
    lw t2, (a1)             # t2 - int pos
    add t3, a0, t2
    lb t1, (t3)             # t1 - char token

    li t3, '*'
    bne t1, t3, term_s_while_check2        # if t1 != '*', check if t1 == '/'
    j term_s_while
    
term_s_while_check2:
    li t3, '/'
    bne t1, t3, term_s_done        # if t1 != '/' end loop

term_s_while:
    addi t2, t2, 1
    sw t2, (a1)                

term_s_while_multiply:
    li t3, '*'
    bne t1, t3, term_s_while_divide
    
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)

    call factor_s

    ld t0, 24(sp)
    mul t0, t0, a0                  # t0 *= factor_s(...)
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    
    j term_s_while_check1

term_s_while_divide:
    li t3, '/'
    bne t1, t3, term_s_while_check1
    
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)

    call factor_s

    ld t0, 24(sp)
    div t0, t0, a0
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    
    j term_s_while_check1

term_s_done:
    mv a0, t0
    ld ra, (sp)
    addi sp, sp, 64
    ret

factor_s:                   # a0 - char *expr_str, a1 - int pos
    addi sp, sp, -64
    sd ra, (sp)

    lw t2, (a1)             # t2 - int pos
    add t3, a0, t2
    lb t1, (t3)             # t1 - char token

    li t3, '('
    bne t1, t3, factor_s_number     # if t0 != '(', check if it is a number
    addi t2, t2, 1                  # update int pos
    sw t2, (a1)
    
    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t1, 24(sp)
    sd t2, 32(sp)
    sd t3, 40(sp)
    
    call expression_s
    
    mv t0, a0                       # t0 - int value
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t1, 24(sp)
    ld t2, 32(sp)
    ld t3, 40(sp)

    lw t2, (a1)
    add t3, a0, t2
    lb t1, (t3)
    
    li t3, ')'
    bne t1, t3, error
    addi t2, t2, 1
    sd t2, (a1)                
    mv a0, t0
    j factor_s_done

factor_s_number:
    call number_s

factor_s_done:
    ld ra, (sp)
    addi sp, sp, 64
    ret
    
isdigit_s:                  # a0 - char c
    li t0, 48
    sub a0, a0, t0
    blt a0, zero, isdigit_false
    li t0, 10
    bge a0, t0, isdigit_false
    
isdigit_done:
    ret

isdigit_false:
    li a0, -1
    j isdigit_done
    
number_s:                   # a0 - char *expr_str, a1 - int pos
    addi sp, sp, -64
    sd ra, (sp)

    lw t2, (a1)             # t2 - int pos
    add t3, a0, t2
    lb t0, (t3)             # t0 - char token

    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t2, 32(sp)
    sd t3, 40(sp)
    mv a0, t0

    call isdigit_s

    blt a0, zero, error

    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t0, 24(sp)
    ld t2, 32(sp)
    ld t3, 40(sp)
    
    li t1, 0                # t1 - int value
    addi t3, t0, -48        # t3 = t0 - '0' -> t3 = token - '0'
    add t1, t1, t3          # t1 = t1 + t2 -> value = value * 10 + (token - '0')
    addi t2, t2, 1
    sw t2, (a1)            
    add t3, a0, t2
    lb t0, (t3)    

number_s_while:

    sd a0, 8(sp)
    sd a1, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)
    mv a0, t0

    call isdigit_s

    blt a0, zero, number_s_done

    ld t1, 32(sp)
    ld t3, 48(sp)

    li t3, 10
    mul t1, t1, t3          # t1 (value) *= 10 
    add t1, t1, a0          # t1 (value) += a0 (token - '0')
    
    ld a0, 8(sp)
    ld a1, 16(sp)
    ld t0, 24(sp)
    ld t2, 40(sp)

    addi t2, t2, 1
    sw t2, (a1)
    add t3, a0, t2
    lb t0, (t3)
    j number_s_while
    
    
number_s_done:
    mv a0, t1
    ld ra, (sp)
    addi sp, sp, 64
    ret

error:
    mv a0, zero
    ld ra, (sp)
    addi sp, sp, 64
    ret
