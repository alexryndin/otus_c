	bits 64
	extern malloc, puts, printf, fflush, abort, free
	global main

	section   .data
empty_str: db 0x0
int_format: db "%ld ", 0x0
;; dq -- 8 byte constant
data: dq 4, 8, 15, 16, 23, 42
;; data_length = 6
data_length: equ ($-data) / 8

	section   .text
;;; print_int proc
print_int:
	mov rsi, rdi
	mov rdi, int_format
	xor rax, rax
	call printf

	xor rdi, rdi
	call fflush

	ret

;;; p proc
p:
	mov rax, rdi
	and rax, 1
	ret

;;; add_element proc
add_element:
	push rbp
	push rbx

	mov rbp, rdi
	mov rbx, rsi

	mov rdi, 16
	call malloc
	test rax, rax
	jz abort

	mov [rax], rbp
	mov [rax + 8], rbx

	pop rbx
	pop rbp

	ret

;;; free_list proc
free_list:
	test rdi, rdi
	jz out_free_list

	push rbp
	push rbx

	mov rbx, [rdi + 8]

	call free

	mov rdi, rbx
	call free_list

	pop rbx
	pop rbp

out_free_list:
	ret

;;; m proc
m:

	push rbp
	push rbx

	mov rbx, rdi
	mov rbp, rsi
m_loop:
	test rbx, rbx
	jz outm

	mov rdi, [rbx]
	call rsi

	mov rbx, [rbx + 8]
	mov rsi, rbp
	jmp m_loop

outm:
	pop rbx
	pop rbp

	ret

;;; f proc
f:


	push rbx
	push r12
	push r13

	mov rbx, rdi
	mov r12, rsi
	mov r13, rdx

f_loop:
	test rbx, rbx
	jz outf

	mov rdi, [rbx]
	call r13
	test rax, rax
	jz ff

	mov rdi, [rbx]
	mov rsi, r12
	call add_element
	mov r12, rax
	jmp ff

ff:
	mov rbx, [rbx + 8]
	jmp f_loop

outf:
	mov rax, r12

	pop r13
	pop r12
	pop rbx

	ret

;;; main proc
main:
	push rbx
	push r12

	xor rax, rax
	mov rbx, data_length
adding_loop:
	mov rdi, [data - 8 + rbx * 8]
	mov rsi, rax
	call add_element
	dec rbx
	jnz adding_loop

	mov rbx, rax

	mov rdi, rax
	mov rsi, print_int
	call m

	mov rdi, empty_str
	call puts

	mov rdx, p
	xor rsi, rsi
	mov rdi, rbx
	call f
	mov r12, rax

	mov rdi, rax
	mov rsi, print_int
	call m

	mov rdi, empty_str
	call puts

	mov rdi, rbx
	call free_list
	mov rdi, r12
	call free_list

	pop r12
	pop rbx

	xor rax, rax
	ret
