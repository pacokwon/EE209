# --------------------------------------------------------------------
# Author: Haechan Kwon (권해찬)
# Assignment: Assembly (Assignment 4)
# Filename: dc.s
# Desk Calculator (dc)
# --------------------------------------------------------------------
        .section ".rodata"
scanfFormat:
    .asciz "%s"

digitFormat:
    .asciz "%d\n"

charFormat:
    .asciz "%c\n"

stackEmptyFormat:
    .asciz "dc: stack empty\n"
# --------------------------------------------------------------------
        .section ".data"

# --------------------------------------------------------------------

        .section ".bss"
buffer:
        .skip  ARRAYSIZE

# --------------------------------------------------------------------

        .section ".text"
    ## -------------------------------------------------------------
    ## int main(void)
    ## Runs desk calculator program.  Returns 0.
    ## -------------------------------------------------------------
    .globl  main
    .type   main,@function

# is op one of (+, -, *, /, |) ?
# int isOp (char op -> 0x4) {
#     return (op == '+' || op == '-' || op == '*' || op == '/' || op == '|');
# }
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

# int abs (int a -> 0x4) {
#     if (a > 0) return a;
#     else return -a;
# }
abs:
    movl    0x4(%esp), %eax
    cmpl    $0, %eax
    jg      __abs_done
    neg     %eax
__abs_done:
    ret

# int min (int a -> 0x4, int b -> 0x8) {
#     if (b >= a) return a;
#     else return b;
# }
min:
    movl    0x4(%esp), %eax
    cmpl    %eax, 0x8(%esp)
    jge     __min_done
    movl    0x8(%esp), %eax
__min_done:
    ret

# int max (int a -> 0x4, int b -> 0x8) {
#     if (b < a) return a;
#     else return b;
# }
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
    .equ    SUM, -0x4
    .equ    I, -0x8
    .equ    MAX, -0xc

    pushl   %ebp
    movl    %esp, %ebp

    # push SUM
    pushl   $0

    # push I
    pushl   0xc(%ebp)
    pushl   0x8(%ebp)
    call    min
    addl    $8, %esp
    pushl   %eax

    # push MAX
    pushl   0xc(%ebp)
    pushl   0x8(%ebp)
    call    max
    addl    $8, %esp
    pushl   %eax

__abssum_loop:
    movl    I(%ebp), %eax
    cmpl    MAX(%ebp), %eax
    jg      __abssum_loop_done

    pushl   %eax
    call    abs
    addl    $4, %esp

    add     %eax, SUM(%ebp)
    incl    I(%ebp)

    jmp     __abssum_loop
__abssum_loop_done:
    movl    SUM(%ebp), %eax
    movl    %ebp, %esp
    popl    %ebp
    ret

# int computeOP(int a -> 0x8, int b -> 0xc, char op -> 0x10) {
#     if (op == '+') return a + b;
#     else if (op == '-') return a - b;
#     else if (op == '*') return a * b;
#     else if (op == '/') return a / b;
#     else return abssum(a, b);
# }
computeOp:
    .equ    OPERAND_A, 0x8
    .equ    OPERAND_B, 0xc
    .equ    OPERATOR, 0x10

    pushl   %ebp
    movl    %esp, %ebp

    movsbl  OPERATOR(%ebp), %eax

    cmpl    $43, %eax
    jne     computeOpSub

    # add operation (a + b)
    movl    OPERAND_A(%ebp), %eax
    addl    OPERAND_B(%ebp), %eax
    jmp     computeOpDone

computeOpSub:
    cmpl    $45, %eax
    jne     computeOpMul

    # sub operation (a - b)
    movl    OPERAND_A(%ebp), %eax
    subl    OPERAND_B(%ebp), %eax
    jmp     computeOpDone

computeOpMul:
    cmpl    $42, %eax
    jne     computeOpDiv

    # mul operation (a * b)
    movl    OPERAND_A(%ebp), %eax
    imull   OPERAND_B(%ebp), %eax
    jmp     computeOpDone

computeOpDiv:
    cmpl    $47, %eax
    jne     computeOpAbsSum

    # div operation (a / b)
    movl    OPERAND_A(%ebp), %eax
    cltd    # sign extend eax into edx:eax
    idivl   OPERAND_B(%ebp)
    jmp     computeOpDone

computeOpAbsSum:
    pushl   OPERAND_A(%ebp)
    pushl   OPERAND_B(%ebp)
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
    .equ   ARRAYSIZE, 20
    .equ   EOF, -1

    # dc number stack initialized. %esp = %ebp

    # scanf("%s", buffer)
    pushl   $buffer
    pushl   $scanfFormat
    call    scanf
    addl    $8, %esp

    # check if user input EOF
    cmp     $EOF, %eax
    je      quit

    #     if (isdigit(buffer[0]) || buffer[0] == '_') {
    # check first condition
    movsbl  (buffer), %eax

    pushl   %eax
    call    isdigit
    addl    $4, %esp

    # if first condition is satisfied, then jump
    cmpl    $0, %eax
    jne     isNumber

    # check second condition
    # if not satisfied, jump to end of if statement
    movsbl  (buffer), %eax
    cmpl    $95, %eax
    jne     pCommand

isNumber:
    cmpl    $95, %eax
    jne     convert
    movb    $45, (buffer) # $45 -> '-'

convert:
    #        num = atoi(buffer);
    pushl   $buffer
    call    atoi
    addl    $4, %esp
    #        stack.push(num);    /* pushl num */
    pushl   %eax

    #        continue;
    jmp     input

pCommand:
    # if (buffer[0] == 'p')
    movsbl  (buffer), %eax
    cmpl    $112, %eax # $112 -> 'p'
    jne     qCommand

    cmpl    %esp, %ebp # is stack pointer at the bottom?
    je      stackEmpty

    #           printf("%d\n", (int)stack.top());
    pushl   (%esp) # stack.top()
    pushl   $digitFormat
    call    printf
    addl    $8, %esp
    #        continue;
    jmp     input

stackEmpty:
    #           printf("dc: stack empty\n");
    pushl   $stackEmptyFormat
    call    printf
    addl    $4, %esp
    jmp     input

qCommand:
    # if (buffer[0] == 'q')
    movsbl  (buffer), %eax
    cmpl    $113, %eax # $113 -> 'q'
    je      quit

opCommand:
    # if (isOp(buffer[0]))

    movsbl  (buffer), %eax

    pushl   %eax
    call    isOp
    addl    $4, %esp

    cmpl    $0, %eax
    je      fCommand

    movl    %esp, %eax
    addl    $8, %eax
    cmpl    %eax, %ebp
    # must satisfy %esp + 8 <= %ebp (jge)
    jl      stackEmpty

    # stack.pop() twice
    movsbl  (buffer), %eax
    popl    %ebx # top element
    popl    %ecx # bottom element

    # pass two popped values and
    # the operator to computeOp
    pushl   %eax
    pushl   %ebx
    pushl   %ecx
    call    computeOp
    addl    $12, %esp

    pushl   %eax
    jmp     input

fCommand:
    # if (buffer[0] == 'f')
    movsbl  (buffer), %eax
    cmpl    $102, %eax # $102 -> 'f'
    jne     cCommand

    pushl   %esp
__fCommand_loop:
    # if cursor has reached the bottom, break out
    cmpl    (%esp), %ebp
    jle     __fCommand_loop_done

    movl    (%esp), %eax
    pushl   (%eax)
    pushl   $digitFormat
    call    printf
    addl    $8, %esp

    # move down the stack by 4
    addl    $4, (%esp)
    jmp     __fCommand_loop
__fCommand_loop_done:
    addl    $4, %esp
    jmp     input

# move the stack pointer to the bottom
cCommand:
    # if (buffer[0] == 'c')
    movsbl  (buffer), %eax
    cmpl    $99, %eax # $99 -> 'c'
    jne     dCommand

    movl    %ebp, %esp
    jmp     input

dCommand:
    # if (buffer[0] == 'd')
    movsbl  (buffer), %eax
    cmpl    $100, %eax # $100 -> 'd'
    jne     rCommand

    # if stack is empty, print message
    cmpl    %esp, %ebp
    je      stackEmpty
    pushl   (%esp)
    jmp     input

rCommand:
    # if (buffer[0] == 'r')
    movsbl  (buffer), %eax
    cmpl    $114, %eax # $114 -> 'r'
    jne     input

    movl    %esp, %eax
    addl    $8, %eax
    cmpl    %eax, %ebp
    # must satisfy %ebp >= %esp + 8 (jge)
    jl      stackEmpty

    popl    %eax
    popl    %ebx

    pushl   %eax
    pushl   %ebx

    jmp    input

    # PSEUDO-CODE
     /*
    #  * In this pseudo-code we assume that you do not use no local variables
    #  * in the _main_ process stack. In case you want to allocate space for local
    #  * variables, please remember to update logic for 'empty dc stack' condition
    #  * (stack.peek() == NULL).
    #  */
    #
    #  while (1) {
    #     /* read the current line into buffer */
    #     if (scanf("%s", buffer) == EOF)
    #         return 0;
    #
    #     /* is this line a number? */
    #     if (isdigit(buffer[0]) || buffer[0] == '_') {
    #        int num;
    #        if (buffer[0] == '_') buffer[0] = '-';
    #        num = atoi(buffer);
    #        stack.push(num);    /* pushl num */
    #        continue;
    #     }
    #
    #     /* p command */
    #     if (buffer[0] == 'p') {
    #        if (stack.peek() == NULL) { /* is %esp == %ebp? */
    #           printf("dc: stack empty\n");
    #        } else {
    #           /* value is already pushed in the stack */
    #           printf("%d\n", (int)stack.top());
    #        }
    #        continue;
    #     }
    #
    #     /* q command */
    #     if (buffer[0] == 'q') {
    #        goto quit;
    #     }
    #
    #     /* + operation */
    #     if (buffer[0] == '+') {
    #        int a, b;
    #        if (stack.peek() == NULL) {
    #           printf("dc: stack empty\n");
    #           continue;
    #         }
    #         a = (int)stack.pop();
    #         if (stack.peek() == NULL) {
    #            printf("dc: stack empty\n");
    #            stack.push(a); /* pushl some register value */
    #            continue;
    #         }
    #         b = (int)stack.pop(); /* popl to some register */
    #         res = a + b;
    #         stack.push(res);
    #         continue;
    #     }
    #
    #     /* - operation */
    #     if (buffer[0] == '-') {
    #        /* ... */
    #     }
    #
    #     /* | operation */
    #     if (buffer[0] == '|') {
    #        /* pop two values & call abssum() */
    #     }
    #
    #     /* other operations and commands */
    #     if (/* others */) {
    #        /* ... and so on ... */
    #     }
    #
    #   } /* end of while */
    #

quit:
    # return 0
    movl    $0, %eax
    movl    %ebp, %esp
    popl    %ebp
    ret
