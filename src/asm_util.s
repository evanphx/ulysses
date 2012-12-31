[GLOBAL read_flags]
read_flags:
    pushfd
    pop eax
    ret

[GLOBAL read_eip]
read_eip:
    pop eax                     ; Get the return address
    jmp eax                     ; Return. Can't use RET because return
                                ; address popped off the stack. 

[GLOBAL read_ebp]
read_ebp:
    mov eax, ebp
    ret

[GLOBAL second_return]
second_return:   ; A dummy symbol that should not be executed.
    ret          ; It's address is just used to determine if save
                 ; registers is returning for the 1st or 2nd time

[GLOBAL save_registers]
save_registers:
    mov edx, [esp+4]
    pop ecx             ; where to return to
    mov [edx+0],  ecx   ; eip
    mov [edx+4],  esp
    mov [edx+8],  ebp
    mov [edx+12], edi
    mov [edx+16], esi
    mov [edx+20], ebx
    mov eax, 0
    jmp ecx

[GLOBAL restore_registers]
restore_registers:
  mov eax, [esp+8]
  mov edx, [esp+4]

  mov ecx, [edx+0]
  mov esp, [edx+4]
  mov ebp, [edx+8]
  mov edi, [edx+12]
  mov esi, [edx+16]
  mov ebx, [edx+20]

  mov cr3, eax
  mov eax, 1
  jmp ecx

extern start_new_thread

[GLOBAL new_thread_tramp]
new_thread_tramp:
  mov eax, ebp
  xor ebp, ebp
  push ebx
  push eax
  push ebp
  jmp start_new_thread

[GLOBAL copy_page_physical]
copy_page_physical:
    push ebx              ; According to __cdecl, we must preserve the contents of EBX.
    pushf                 ; push EFLAGS, so we can pop it and reenable interrupts
                          ; later, if they were enabled anyway.
    cli                   ; Disable interrupts, so we aren't interrupted.
                          ; Load these in BEFORE we disable paging!
    mov ebx, [esp+12]     ; Source address
    mov ecx, [esp+16]     ; Destination address
  
    mov edx, cr0          ; Get the control register...
    and edx, 0x7fffffff   ; and...
    mov cr0, edx          ; Disable paging.
  
    mov edx, 1024         ; 1024*4bytes = 4096 bytes
  
.loop:
    mov eax, [ebx]        ; Get the word at the source address
    mov [ecx], eax        ; Store it at the dest address
    add ebx, 4            ; Source address += sizeof(word)
    add ecx, 4            ; Dest address += sizeof(word)
    dec edx               ; One less word to do
    jnz .loop             
  
    mov edx, cr0          ; Get the control register again
    or  edx, 0x80000000   ; and...
    mov cr0, edx          ; Enable paging.
  
    popf                  ; Pop EFLAGS back.
    pop ebx               ; Get the original value of EBX back.
    ret

