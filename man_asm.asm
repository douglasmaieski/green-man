section .text

MAN_SAVED_REGISTERS: equ 0
MAN_NEXT_INSTRUCTION: equ 128
MAN_NEXT_W_NODE: equ 136
MAN_RINGS_PTR: equ 144
MAN_FUN_PTR: equ 288

MWORKER_PARENT: equ 0
MWORKER_NEXT_W_NODE: equ 8
MWORKER_NEXT_INSTRUCTION: equ 16
MWORKER_STACK_BASE_PTR: equ 24
MWORKER_STACK_PTR: equ 32


extern rings_submit
extern _man_add_timer_worker
extern _man_cond_wait

global man_do_return
global back_to_worker
global prepare_worker
global wake_up_worker
global man_write
global man_read
global man_send
global man_recv
global man_sendmsg
global man_recvmsg
global man_writev
global man_readv
global man_shutdown
global man_close
global man_openat
global man_fsync
global man_socket
global man_connect
global man_accept
global man_unlinkat
global man_sleep
global man_cond_wait


_save_regs:
  mov [rdi+MAN_SAVED_REGISTERS+0],rax
  mov [rdi+MAN_SAVED_REGISTERS+8],rbx
  mov [rdi+MAN_SAVED_REGISTERS+16],rcx
  mov [rdi+MAN_SAVED_REGISTERS+24],rdx

  ;mov [rdi+MAN_SAVED_REGISTERS+32],rdi

  mov [rdi+MAN_SAVED_REGISTERS+40],rsi
  mov [rdi+MAN_SAVED_REGISTERS+48],rbp
  mov [rdi+MAN_SAVED_REGISTERS+56],rsp
  mov [rdi+MAN_SAVED_REGISTERS+64],r8
  mov [rdi+MAN_SAVED_REGISTERS+72],r9
  mov [rdi+MAN_SAVED_REGISTERS+80],r10
  mov [rdi+MAN_SAVED_REGISTERS+88],r11
  mov [rdi+MAN_SAVED_REGISTERS+96],r12
  mov [rdi+MAN_SAVED_REGISTERS+104],r13
  mov [rdi+MAN_SAVED_REGISTERS+112],r14
  mov [rdi+MAN_SAVED_REGISTERS+120],r15

  jmp [rdi+MAN_NEXT_INSTRUCTION]


_restore_regs:
  mov rdi,[rdi+MWORKER_PARENT]

  mov rax,[rdi+MAN_SAVED_REGISTERS+0]
  mov rbx,[rdi+MAN_SAVED_REGISTERS+8]
  mov rcx,[rdi+MAN_SAVED_REGISTERS+16]
  mov rdx,[rdi+MAN_SAVED_REGISTERS+24]

  ;mov rdi,[rdi+MAN_SAVED_REGISTERS+32]

  mov rsi,[rdi+MAN_SAVED_REGISTERS+40]
  mov rbp,[rdi+MAN_SAVED_REGISTERS+48]
  mov rsp,[rdi+MAN_SAVED_REGISTERS+56]
  mov r8,[rdi+MAN_SAVED_REGISTERS+64]
  mov r9,[rdi+MAN_SAVED_REGISTERS+72]
  mov r10,[rdi+MAN_SAVED_REGISTERS+80]
  mov r11,[rdi+MAN_SAVED_REGISTERS+88]
  mov r12,[rdi+MAN_SAVED_REGISTERS+96]
  mov r13,[rdi+MAN_SAVED_REGISTERS+104]
  mov r14,[rdi+MAN_SAVED_REGISTERS+112]
  mov r15,[rdi+MAN_SAVED_REGISTERS+120]

  jmp [rdi+MAN_NEXT_INSTRUCTION]


back_to_worker:
  lea r8,[rel .back_to_worker]
  mov qword [rdi+MAN_NEXT_INSTRUCTION],r8
  jmp _save_regs

  .back_to_worker:
    lea r8,[rel .return]
    mov qword [rdi+MAN_NEXT_INSTRUCTION],r8
    mov rsp,[rsi+MWORKER_STACK_PTR]
    mov eax,edx
    jmp [rsi+MWORKER_NEXT_INSTRUCTION]
  
  .return:
    ret


prepare_worker:
  lea r8,[rel .prepare_worker]
  mov qword [rdi+MAN_NEXT_INSTRUCTION],r8
  mov r8,[rdi+MAN_FUN_PTR]
  jmp _save_regs

  .prepare_worker:
    lea r9,[rel .done]
    mov qword [rdi+MAN_NEXT_INSTRUCTION],r9
    mov rdi,rdx
    mov rsp,[rdx+MWORKER_STACK_BASE_PTR]
    mov [rdx+MWORKER_STACK_PTR],rsp
    call r8
  
  .done:
    ret

wake_up_worker:
  lea r8,[rel .wake_up_worker]
  mov qword [rdi+MAN_NEXT_INSTRUCTION],r8
  jmp _save_regs

.wake_up_worker:
  lea r8,[rel .done]
  mov qword [rdi+MAN_NEXT_INSTRUCTION],r8
  mov rsp,[rsi+MWORKER_STACK_PTR]
  jmp [rsi+MWORKER_NEXT_INSTRUCTION]

.done:
  ret

man_do_return:
  ; RDI -> worker
  mov rsi,[rdi+MWORKER_PARENT]

  mov rdx,[rsi+MAN_NEXT_W_NODE]
  mov [rdi+MWORKER_NEXT_W_NODE],rdx
  mov [rsi+MAN_NEXT_W_NODE],rdi

  jmp _restore_regs


_io_rw:
  ; RDI -> worker
  ; ESI -> fd
  ; RDX -> offset
  ; RCX -> buffer
  ; R8  -> length
  push rbx
  push rcx
  push rdx
  push rdi
  push rsi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  sub rsp,72

  mov r11,rsp

  pxor xmm0,xmm0
  movdqa [r11+32],xmm0
  movdqa [r11+48],xmm0

  ; opcode + fd
  shl rsi,32
  or rsi,r9
  mov [r11],rsi

  ; offset
  mov [r11+8],rdx

  ; address
  mov [r11+16],rcx

  ; length
  mov r8d,r8d
  mov [r11+24],r8

  ; user data
  mov [r11+32],rdi

  ; get parent
  mov rax,[rdi+MWORKER_PARENT]

  push rdi

  ; call submit
  lea rdi,[rax+MAN_RINGS_PTR]
  mov rsi,r11
  call rings_submit

  pop rdi
  add rsp,72

  test eax,eax
  jnz .return_error

  ; go to another process
  lea r8,[rel .ret_handler]
  mov qword [rdi+MWORKER_NEXT_INSTRUCTION],r8
  mov [rdi+MWORKER_STACK_PTR],rsp

  jmp _restore_regs

.ret_handler:
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rsi
  pop rdi
  pop rdx
  pop rcx
  pop rbx
  ret

.return_error:
  mov eax,-128000
  jmp .ret_handler


man_write:
  mov r10,4
  mov r9,23
  jmp _io_rw


man_read:
  mov r10,1
  mov r9,22
  jmp _io_rw


_make_call:
  ; receive data in:
  ; xmm0, xmm1, xmm2, xmm3
  push rbp
  push rbx
  push r12
  push r13
  push r14
  push r15
  push rdi
  sub rsp,64

  movdqa [rsp],xmm0
  movdqa [rsp+16],xmm1
  movdqa [rsp+32],xmm2
  movdqa [rsp+48],xmm3

  ; submit
  mov rax,[rdi+MWORKER_PARENT]
  lea rdi,[rax+MAN_RINGS_PTR]
  mov rsi,rsp
  call rings_submit

  add rsp,64
  pop rdi

  test eax,eax
  jnz .return_error

  ; go to another process
  lea r8,[rel .done]
  mov qword [rdi+MWORKER_NEXT_INSTRUCTION],r8
  mov [rdi+MWORKER_STACK_PTR],rsp
  jmp _restore_regs

.done:
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbx
  pop rbp
  ret

.return_error:
  mov rax,-128000
  jmp .done


man_close:
  shl rsi,32
  or rsi,19 ; close
  movq xmm0,rsi

  pxor xmm1,xmm1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_openat:
  ; rdi -> worker
  ; esi -> dirfd
  ; rdx -> pathname
  ; ecx -> flags
  ; r8  -> mode
  ; opcode + fd + offset

  shl rsi,32
  or rsi,18 ; openat
  movq xmm0,rsi

  shl rcx,32
  mov r8d,r8d
  or rcx,r8

  movq xmm1,rdx
  pinsrq xmm1,rcx,1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_fsync:
  ; rdi -> worker
  ; esi -> fd
  shl rsi,32
  or rsi,3 ; fsync
  movq xmm0,rsi

  pxor xmm1,xmm1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_socket:
  ; rdi -> worker
  ; esi -> domain
  ; edx -> type
  ; ecx -> protocol

  shl rsi,32
  or rsi,45 ; socket

  mov edx,edx
  movq xmm0,rsi
  pinsrq xmm0,rdx,1

  mov ecx,ecx
  pxor xmm1,xmm1
  pinsrq xmm1,rcx,1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_connect:
  ; rdi -> worker
  ; esi -> sockfd
  ; rdx -> addr
  ; rcx -> addrlen
  shl rsi,32
  or rsi,16 ; connect
  movq xmm0,rsi

  mov ecx,ecx
  pinsrq xmm0,rcx,1

  movq xmm1,rdx

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_accept:
  ; rdi -> worker
  ; esi -> sockfd
  ; rdx -> addr
  ; rcx -> addrlen
  ; r8d -> flags
  shl rsi,32
  or rsi,13 ; accept
  movq xmm0,rsi
  pinsrq xmm0,rcx,1

  movq xmm1,rdx
  mov r8d,r8d
  shl r8,32
  pinsrq xmm1,r8,1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_unlinkat:
  ; rdi -> worker
  ; esi -> dirfd
  ; rdx -> pathname
  ; ecx -> flags
  shl rsi,32
  or rsi,36 ; unlinkat
  movq xmm0,rsi

  movq xmm1,rdx
  shl rcx,32
  pinsrq xmm1,rcx,1

  movq xmm2,rdi

  pxor xmm3,xmm3

  jmp _make_call


man_send:
  shl rsi,32
  or  rsi,26 
  movq xmm0,rsi

  mov  r9,r8
  shl  r9,32
  mov  r10d,ecx
  or   r9,r10

  movq  xmm1,rdx
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_recv:
  shl rsi,32
  or  rsi,27
  movq xmm0, rsi

  mov  r9,r8
  shl  r9,32
  mov  r10d,ecx
  or   r9,r10

  movq  xmm1,rdx
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_sendmsg:
  ; rdi -> worker
  ; esi -> sockfd
  ; rdx -> msg (msghdr*)
  ; ecx -> flags

  shl rsi,32
  or  rsi,9
  movq xmm0,rsi

  mov   r9d,ecx
  shl   r9,32
  mov   r10d,1
  or    r9,r10

  movq  xmm1,rdx
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_recvmsg:
  ; rdi -> worker
  ; esi -> sockfd
  ; rdx -> msg (msghdr*)
  ; ecx -> flags

  shl rsi,32
  or  rsi,10 
  movq xmm0,rsi

  mov   r9d,ecx
  shl   r9,32
  mov   r10d,1
  or    r9,r10

  movq  xmm1,rdx
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_writev:
  shl rsi,32
  or  rsi,2
  movq xmm0,rsi
  pinsrq xmm0,rdx,1

  movq xmm1,rcx
  mov r9d,r8d
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_readv:
  shl rsi,32
  or rsi,1
  movq xmm0,rsi
  pinsrq xmm0,rdx,1

  movq xmm1,rcx
  mov r9d,r8d 
  pinsrq xmm1,r9,1

  movq xmm2, rdi
  pxor xmm3, xmm3
  jmp _make_call


man_shutdown:
  shl rsi,32
  or  rsi,34              
  movq xmm0,rsi

  pxor xmm1,xmm1
  mov  r9d,edx
  pinsrq xmm1,r9,1

  movq xmm2,rdi
  pxor xmm3,xmm3
  jmp _make_call


man_sleep:
  ; RDI -> worker
  ; RSI -> wakeup time
  push rbx
  push rcx
  push rdx
  push rdi
  push rsi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  push rdi

  mov rdx,rsi ; wakeup time 3rd arg
  mov rsi,rdi ; 2nd arg is worker
  mov rdi,[rdi+MWORKER_PARENT] ; 1st arg is parent wgm
  call _man_add_timer_worker

  pop rdi

  lea r8,[rel .resume]
  mov qword [rdi+MWORKER_NEXT_INSTRUCTION],r8
  mov [rdi+MWORKER_STACK_PTR],rsp
  jmp _restore_regs
 
.resume:
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rsi
  pop rdi
  pop rdx
  pop rcx
  pop rbx
  ret


man_cond_wait:
  ; RDI -> cond
  ; RSI -> wcond
  push rbx
  push rcx
  push rdx
  push rdi
  push rsi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  push rsi

  call _man_cond_wait

  pop rsi

  mov rdi,[rsi+8]

  lea r8,[rel .resume]
  mov qword [rdi+MWORKER_NEXT_INSTRUCTION],r8
  mov [rdi+MWORKER_STACK_PTR],rsp
  jmp _restore_regs

.resume:
  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rsi
  pop rdi
  pop rdx
  pop rcx
  pop rbx
  ret