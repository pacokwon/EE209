### --------------------------------------------------------------------
### mydc.s
###
### Desk Calculator (dc)
### --------------------------------------------------------------------
    .equ   ARRAYSIZE, 20
    .equ   EOF, -1
    .equ   INUM, -4

        .section ".rodata"
scanfFormat:
    .asciz "%s"

digitFormat:
    .asciz "Number %d\n"

threeDigitsFormat:
    .asciz "Number %d %d %c\n"

pCommandNotEmptyFormat:
    .asciz "%d\n"

charFormat:
    .asciz "%c\n"

stackEmptyFormat:
    .asciz "dc: stack empty\n"
### --------------------------------------------------------------------
        .section ".data"

### --------------------------------------------------------------------

        .section ".bss"
buffer:
        .skip  ARRAYSIZE

### --------------------------------------------------------------------

        .section ".text"
    ## -------------------------------------------------------------
    ## int main(void)
    ## Runs desk calculator program.  Returns 0.
    ## -------------------------------------------------------------
    .globl  main
    .type   main,@function

## (char op)
## is op one of (+, -, *, /, |) ?
isOp:
    ## return (op == '+' || op == '-' || op == '*' || op == '/' || op == '|')
    movl    0x4(%esp), %eax
    cmpl    $45, %eax
    je      __isOp_true

    cmpl    $43, %eax
    je      __isOp_true

    cmpl    $42, %eax
    je      __isOp_true

    cmpl    $47, %eax
    je      __isOp_true

    cmpl    $124, %eax
    je      __isOp_true

__isOp_false:
    movl    $0, %eax
    jmp     __isOp_done
__isOp_true:
    movl    $1, %eax
__isOp_done:
    ret

## (int a)
abs:
    movl    0x4(%esp), %eax
    cmpl    $0, %eax
    jg      __abs_done
    neg     %eax
__abs_done:
    ret

min:
    movl    0x4(%esp), %eax
    cmpl    %eax, 0x8(%esp)
    jge     __min_done
    movl    0x8(%esp), %eax
__min_done:
    ret

max:
    movl    0x4(%esp), %eax
    cmpl    %eax, 0x8(%esp)
    jl      __max_done
    movl    0x8(%esp), %eax
__max_done:
    ret


# int abssum (int a -> 0x8, int b -> 0xc) {
#     int i, s = 0;
#     for (i = min (a, b); i <= max (a, b); i++)
#         s += |i|;
#     return s;
# }
abssum:
    pushl   %ebp
    movl    %esp, %ebp

    # -4 is s
    pushl   $0

    # -8 is i
    pushl   0xc(%ebp)
    pushl   0x8(%ebp)
    call    min
    addl    $8, %esp
    pushl   %eax

    # -12 is max
    pushl   0xc(%ebp)
    pushl   0x8(%ebp)
    call    max
    addl    $8, %esp
    pushl   %eax

__abssum_loop:
    movl    -0x8(%ebp), %eax
    cmpl    -0xc(%ebp), %eax
    jg      __abssum_loop_done

    pushl   %eax
    call    abs
    addl    $4, %esp

    add     %eax, -0x4(%ebp)
    incl    -0x8(%ebp)

    jmp     __abssum_loop
__abssum_loop_done:
    movl    -0x4(%ebp), %eax
    movl    %ebp, %esp
    popl    %ebp
    ret

## (int a -> 0x8, int b -> 0xc, char op -> 0x10)
computeOp:
    pushl   %ebp
    movl    %esp, %ebp

    movsbl  0x10(%ebp), %eax

    # movl    0xc(%ebp), %ebx
    # movl    0x8(%ebp), %ecx
    # pushl   %eax
    # pushl   %ebx
    # pushl   %ecx
    # pushl   $threeDigitsFormat
    # call    printf
    # addl    $16, %esp

    cmpl    $43, %eax
    jne     computeOpSub

    # add operation (a + b)
    movl    0x8(%ebp), %eax
    addl    0xc(%ebp), %eax
    jmp     computeOpDone

computeOpSub:
    cmpl    $45, %eax
    jne     computeOpMul

    # sub operation (a - b)
    movl    0x8(%ebp), %eax
    subl    0xc(%ebp), %eax
    jmp     computeOpDone

computeOpMul:
    cmpl    $42, %eax
    jne     computeOpDiv

    # mul operation (a * b)
    movl    0x8(%ebp), %eax
    imull   0xc(%ebp), %eax
    jmp     computeOpDone

computeOpDiv:
    cmpl    $47, %eax
    jne     computeOpAbsSum

    # div operation (a / b)
    movl    0x8(%ebp), %eax
    cltd    # sign extend eax into edx:eax
    idivl   0xc(%ebp)
    jmp     computeOpDone

computeOpAbsSum:
    pushl   0x8(%ebp)
    pushl   0xc(%ebp)
    call    abssum
    addl    $8, %esp

computeOpDone:
    movl    %ebp, %esp
    popl    %ebp
    ret

main:
    pushl   %ebp
    movl    %esp, %ebp

input:
    ## dc number stack initialized. %esp = %ebp

    ## scanf("%s", buffer)
    pushl   $buffer
    pushl   $scanfFormat
    call    scanf
    addl    $8, %esp

    ## check if user input EOF
    cmp     $EOF, %eax
    je      quit

    ##     if (isdigit(buffer[0]) || buffer[0] == '_') {
    ## check first condition
    movsbl  (buffer), %eax

    pushl   %eax
    call    isdigit
    addl    $4, %esp

    ## if first condition is satisfied, then jump
    cmpl    $0, %eax
    jne     isNumber

    ## check second condition
    ## if not satisfied, jump to end of if statement
    movsbl  (buffer), %eax
    cmpl    $95, %eax
    jne     pCommand

isNumber:
    cmpl    $95, %eax
    jne     convert
    movb    $45, (buffer) # $45 -> '-'

convert:
    ##        num = atoi(buffer);
    pushl   $buffer
    call    atoi
    addl    $4, %esp
    ##        stack.push(num);    /* pushl num */
    pushl   %eax

    # # Number Debug
    # pushl   %eax
    # pushl   $digitFormat
    # call    printf
    # addl    $8, %esp

    ##        continue;
    jmp     input

pCommand:
    movsbl  (buffer), %eax
    cmpl    $112, %eax # $112 -> 'p'
    jne     qCommand

    cmpl    %esp, %ebp # is stack pointer at the bottom?
    je      pCommandEmpty

    ##           printf("%d\n", (int)stack.top());
    pushl   (%esp) # stack.top()
    pushl   $pCommandNotEmptyFormat
    call    printf
    addl    $8, %esp
    ##        continue;
    jmp     input

pCommandEmpty:
    ##           printf("dc: stack empty\n");
    pushl   $stackEmptyFormat
    call    printf
    addl    $4, %esp
    jmp     input

qCommand:
    movsbl  (buffer), %eax
    cmpl    $113, %eax # $113 -> 'q'
    je      quit

opCommand:
    movsbl  (buffer), %eax

    pushl   %eax
    call    isOp
    addl    $4, %esp

    cmpl    $0, %eax
    je      quit

    movsbl  (buffer), %eax
    popl    %ebx # top element
    popl    %ecx # bottom element

    pushl   %eax
    pushl   %ebx
    pushl   %ecx
    call    computeOp
    addl    $12, %esp

    pushl   %eax
    jmp     input

    ## PSEUDO-CODE
    ## /*
    ##  * In this pseudo-code we assume that you do not use no local variables
    ##  * in the _main_ process stack. In case you want to allocate space for local
    ##  * variables, please remember to update logic for 'empty dc stack' condition
    ##  * (stack.peek() == NULL).
    ##  */
    ##
    ##  while (1) {
    ##     /* read the current line into buffer */
    ##     if (scanf("%s", buffer) == EOF)
    ##         return 0;
    ##
    ##     /* is this line a number? */
    ##     if (isdigit(buffer[0]) || buffer[0] == '_') {
    ##        int num;
    ##        if (buffer[0] == '_') buffer[0] = '-';
    ##        num = atoi(buffer);
    ##        stack.push(num);    /* pushl num */
    ##        continue;
    ##     }
    ##
    ##     /* p command */
    ##     if (buffer[0] == 'p') {
    ##        if (stack.peek() == NULL) { /* is %esp == %ebp? */
    ##           printf("dc: stack empty\n");
    ##        } else {
    ##           /* value is already pushed in the stack */
    ##           printf("%d\n", (int)stack.top());
    ##        }
    ##        continue;
    ##     }
    ##
    ##     /* q command */
    ##     if (buffer[0] == 'q') {
    ##        goto quit;
    ##     }
    ##
    ##     /* + operation */
    ##     if (buffer[0] == '+') {
    ##        int a, b;
    ##        if (stack.peek() == NULL) {
    ##           printf("dc: stack empty\n");
    ##           continue;
    ##         }
    ##         a = (int)stack.pop();
    ##         if (stack.peek() == NULL) {
    ##            printf("dc: stack empty\n");
    ##            stack.push(a); /* pushl some register value */
    ##            continue;
    ##         }
    ##         b = (int)stack.pop(); /* popl to some register */
    ##         res = a + b;
    ##         stack.push(res);
    ##         continue;
    ##     }
    ##
    ##     /* - operation */
    ##     if (buffer[0] == '-') {
    ##        /* ... */
    ##     }
    ##
    ##     /* | operation */
    ##     if (buffer[0] == '|') {
    ##        /* pop two values & call abssum() */
    ##     }
    ##
    ##     /* other operations and commands */
    ##     if (/* others */) {
    ##        /* ... and so on ... */
    ##     }
    ##
    ##   } /* end of while */
    ##

quit:
    ## return 0
    movl    $0, %eax
    movl    %ebp, %esp
    popl    %ebp
    ret
