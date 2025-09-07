section .text
global contextSwitch
contextSwitch:
	mov eax, [esp + 4] ; eax = from
	mov edx, [esp + 8] ; edx = to
	
	; these are the callee-saved registers on x86 according to cdecl
	; they will change once we go off and execute the other task
	; so we need to preserve them for when we get back
	
	; think of it from the perspective of the calling function,
	; it wont notice that we go off and execute some other code
	; but when we return to it, suddenly the registers it
	; expected to remain unchanged has changed
	push ebx
	push esi
	push edi
	push ebp

	; swap kernel stack pointer and store them
	mov [eax + 0], esp ; from->kesp = esp
	mov esp, [edx + 0] ; esp = to->kesp
	
	; NewTaskKernelStack will match the stack from here on out.

	pop ebp
	pop edi
	pop esi
	pop ebx

	ret ; new tasks hijack the return address to new_task_setup
    
; ... contextSwitch is correct ...

global newTaskSetup
newTaskSetup:
    ; This is where the 'ret' from contextSwitch lands for a new task.
    ; ESP now points to the 'data_selector' field in NewTaskKernelStack.
    pop ebx         ; Pop data_selector into ebx
    mov ds, bx      ; Load all data segments
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; ESP now points to the IRET frame (eip, cs, eflags, etc.)
    
    ; zero out registers so they dont leak
	xor eax, eax
	; ebx is already set, so we can skip it
	xor ecx, ecx
	xor edx, edx
	xor esi, esi
	xor edi, edi
	xor ebp, ebp

    iret            ; Start the task