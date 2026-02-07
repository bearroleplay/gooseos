bits 32

section .multiboot
align 4
    dd 0x1BADB002          ; magic
    dd 0x00000003          ; flags
    dd -(0x1BADB002 + 3)   ; checksum

section .bss
align 16
stack_bottom:
    resb 16384             ; 16KB стека
stack_top:

section .text
global _start
extern kernel_main         ; void kernel_main(uint32_t magic, void* mb_info)

_start:
    ; Настройка стека
    mov esp, stack_top
    
    ; ПЕРЕДАЧА АРГУМЕНТОВ В ПРАВИЛЬНОМ ПОРЯДКЕ:
    ; Сигнатура: kernel_main(uint32_t magic, void* mb_info)
    ; В eax - magic (от Multiboot)
    ; В ebx - mb_info (указатель на структуру)
    
    push ebx              ; ВТОРОЙ аргумент: mb_info (void*)
    push eax              ; ПЕРВЫЙ аргумент: magic (uint32_t)
    
    ; Вызов ядра
    call kernel_main
    
    ; Ядро никогда не должно вернуться сюда
    ; Но на всякий случай:
    cli                    ; Запрещаем прерывания
.hang:
    hlt                    ; Останавливаем процессор
    jmp .hang             ; Бесконечный цикл
