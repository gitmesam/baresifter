  ; -*- Mode: nasm -*-

bits 32
section .text
extern irq_entry
global irq_entry_start, irq_entry_end, irq_exit

  ; Generate an interrupt entry function that takes care of normalizing the
  ; stack frame, i.e. pushes a dummy error code if the processor did not.
%macro gen_entry 1-2 0          ; number has-error-code
irq_entry_%1:
  %if %2
  ; Two-byte NOP
  xchg ax, ax
  %else
  ; Push dummy error code
  push 0
  %endif
  push %1
  jmp near save_context
%endmacro

  ; Save the general-purpose registers and branch to the interrupt entry in C++.
save_context:
  cld
  clts                          ; enable FPU
  pusha

  mov eax, 0x2b                 ; Userspace might have destroyed these
  mov ds, eax
  mov es, eax
  mov fs, eax
  mov gs, eax

  mov eax, esp                  ; exception_frame
  call irq_entry
irq_exit:
  mov eax, cr0
  or eax, (1 << 3)              ; disable FPU
  mov cr0, eax
  popa
  add esp, 8                    ; error code / vector
  iret

irq_entry_start:
  gen_entry 0
  gen_entry 1
  gen_entry 2
  gen_entry 3
  gen_entry 4
  gen_entry 5
  gen_entry 6
  gen_entry 7
  gen_entry 8, 1
  gen_entry 9
  gen_entry 10, 1
  gen_entry 11, 1
  gen_entry 12, 1
  gen_entry 13, 1
  gen_entry 14, 1
  gen_entry 15

  gen_entry 16
  gen_entry 17, 1
  gen_entry 18
  gen_entry 19
  gen_entry 20
  gen_entry 21
  gen_entry 22
  gen_entry 23
  gen_entry 24
  gen_entry 25
  gen_entry 26
  gen_entry 27
  gen_entry 28
  gen_entry 29, 1
  gen_entry 30, 1
  gen_entry 31
irq_entry_end:

%if (irq_entry_end - irq_entry_start) != (irq_entry_2 - irq_entry_1)*32
%error "Interrupt entry function size is inconsistent."
%endif
