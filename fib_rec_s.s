.global fib_rec_s


# fibrec - compute the nth fibonacci number

# a0 - int n

fib_rec_s:
    addi sp, sp, -16
    sd ra, (sp)

    mv t0, a0
    li t1, 1

    #Base case
    blt t1, t0, recstep
    mv a0, t0
    j done

#Recursive Step
recstep:
    # call fibrec on t0 - 1 and t0 - 2, add those values and return
    addi t0, t0, -1
    sd t0, 8(sp)

    mv a0, t0
    call fib_rec_s
    ld t0, 8(sp)
    addi t0, t0, -1

    sd a0, 8(sp)
    mv a0, t0
    call fib_rec_s
    ld t0, 8(sp)
    add a0, a0, t0
    
done:
    ld ra, (sp)
    addi sp, sp, 16    
    ret
