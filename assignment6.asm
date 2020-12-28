TITLE Designing Low Level I/O procedures  (assignment6.asm)

; Author: Michael Chen
; Last Modified: 6/7/2020
; OSU email address: chenmich@oregonstate.edu
; Course number/section: 401
; Project Number: 6                 Due Date:June 7 2020
; Description:Using low level I/O procedures and macros

INCLUDE Irvine32.inc


;macro: get_string
;Description:gets a string from the user
;preconditions:gets a place to store the string and a place to store the number of characters counted
;postconditions:a string is returned
;registers changed:none
get_string MACRO storage, count
	push			edx
	push			ecx
	mov				edx, storage
	mov				ecx, 32
	call			ReadString
	mov				[count], eax
	pop				ecx
	pop				edx
ENDM


;macro:display_string
;Description:displays a string
;preconditions:gets a string
;postconditions:a string has been displayed
;registers changed:none
display_string MACRO string
	push			edx
	mov				edx, OFFSET string
	call			WriteString
	pop				edx
ENDM


AMOUNT = 10

.data
	;intro
	intro		BYTE	"Designing low-level I/O procedure by Michael Chen", 0
	ec_1		BYTE	"ec 1) Numbered lines", 0
	ec_2		BYTE	"ec 2) ReadVal and WriteVal using FPU", 0

	;get numbers
	info_1		BYTE	"Enter 10 signed integers, one at a time!", 0
	info_2		BYTE	"Numbers must be small enough to fit inside a 32-bit register", 0
	prompt		BYTE	") Enter a number: ", 0
	error		BYTE	"Invalid number", 0
	array		SDWORD	AMOUNT DUP(?)
	temp_num	BYTE	32 DUP(?)
	read_size	DWORD	?
	valid		DWORD	?


	;display
	disp_1		BYTE	"You entered the following numbers: ", 0
	comma		BYTE	", ", 0
	disp_2		BYTE	"The sum is: ", 0
	disp_3		BYTE	"The average is: ", 0

	;bye
	bye			BYTE	"I am hungry, so goodbye.", 0


.code
;Procedure: read_val
;Description:reads a value from the user and stores it
;preconditions:the program needs a value from the user
;postconditions:all needed ints are read and valid
;registers changed:ebx, eax, esi, edx, edi, ecx
read_val PROC
	;stack frame
	push			ebp
	mov				ebp, esp
	pushad
	mov				ebx, [ebp + 24]
	mov				eax, [ebp + 12]

	;get string
	push			[ebp + 12]
	push			[ebp + 16]
	call			write_val
	display_string	prompt
	get_string		[ebp + 16], [ebp + 28]
	
	;validate
	push			[ebp + 28]		;read_size
	push			[ebp + 8]		;valid bool
	push			[ebp + 16]		;temp_num
	call			verify
	mov				eax, [ebp + 8]
	mov				esi, 0
	cmp				[eax], esi
	je				skip

	;convert and store if valid
	cld
	mov				ebx, 0
	mov				edx, 1
	mov				esi, [ebp + 16]
	mov				ecx, [ebp + 28]
	mov				edi, [ebp + 20]
next_char:
	mov				eax, 0
	lodsb
	push			ecx

	;check for sign
	cmp				eax, 02Bh
	jne				check_neg
	mov				edx, 1
	jmp				not_digit

check_neg:
	cmp				eax, 02Dh
	jne				digit
	mov				edx, -1
	jmp				not_digit

	;add digit
digit:
	push			edx
	sub				eax, 48
	add				ebx, eax
	cmp				ecx, 1
	je				no_mult
	mov				eax, ebx
	mov				ecx, 10
	mul				ecx
	mov				ebx, eax
no_mult:
	pop				edx

not_digit:
	pop				ecx
	loop			next_char

	;save in array
	mov				eax, ebx
	mul				edx
	mov				[edi], eax
	jmp				skip

	;change to invalid int
invalid:
	mov				eax, 0
	mov				[ebp + 8], eax

skip:
	popad
	pop				ebp
	ret				24
read_val ENDP


;Procedure: verify
;Description:checks if an int is valid
;preconditions:gets an bool, read size, and string int
;postconditions:returns a bool
;registers changed:eax, edx, ebx, ecx
verify PROC
	;stack frame and setup
	push			ebp
	mov				ebp, esp
	mov				eax, [ebp + 8]
	mov				edx, 0
	mov				dl, [eax]
	mov				ebx, [ebp + 12]
	mov				ecx, [ebp + 16]
	mov				esi, 0

	;parse through first char
	cmp				dl, 02Bh
	je				good
	cmp				dl, 02Dh
	je				good
	;parse through each char
parse:
	mov				dl, [eax + esi]
	cmp				dl, 030h
	jl				invalid
	cmp				dl, 039h
	jg				invalid

	;valid input
good:
	inc				esi
	loop			parse
	mov				eax, 1
	mov				[ebx], eax
	jmp				finish

	;invalid input
invalid:
	mov				eax, 0
	mov				[ebx], eax

finish:
	pop				ebp
	ret				12
verify ENDP


;Procedure: write_val
;Description:converts an int into a string and writes it
;preconditions:gets an int and string container
;postconditions:the int has been printed
;registers changed:ebx, edi, eax, esi, ecx, edx
write_val PROC
	push			ebp
	mov				ebp, esp
	pushad
	mov				ebx, [ebp + 12]
	mov				edi, [ebp + 8]
	mov				eax, ebx
	mov				esi, 10
	mov				ecx, -1

	cmp				ebx, 0
	jge				positive
	mov				al, 45
	call			WriteChar
	mov				eax, ebx
	mov				ebx, -1
	mul				ebx
	mov				ebx, eax
positive:

	;get number of digits
divide:
	mov				edx, 0
	idiv			esi
	inc				ecx
	cmp				eax, 0
	jne				divide
	;get starting divisor
	mov				eax, 1
	cmp				ecx, 0
	je				done
multiply:
	mul				esi
	loop			multiply
done:
	xchg			eax, ebx

	;convert to string
	;write sign
	mov				ecx, 10

	;write digits
	mov				edi, [ebp + 8]
	cld
again:
	mov				edx, 0
	idiv			ebx
	add				eax, 48
	stosb
	;change divisor
	push			edx
	mov				eax, ebx
	mov				edx, 0
	div				ecx
	mov				ebx, eax
	pop				edx
	mov				eax, edx
	cmp				ebx, 0
	jne				again
	mov				eax, 0
	stosb

	display_string	OFFSET temp_num

	popad
	pop				ebp
	ret				8
write_val ENDP


;Procedure: introduction
;Description:says the introduction
;preconditions:none
;postconditions:prints the strings
;registers changed:none
introduction PROC
	display_string	intro
	call			Crlf
	display_string	ec_1
	call			Crlf
	call			Crlf

	ret
introduction ENDP


;Procedure: get_info
;Description:gets all the numbers
;preconditions:gets the read size, bool, string container, array, and its size
;postconditions:all 10 ints are taken in
;registers changed:ecx, ebx, esi, edx, eax
get_info PROC
	;stack frame and setup
	push			ebp
	mov				ebp, esp
	mov				ecx, [ebp + 8]
	mov				ebx, 1
	mov				esi, 0
	mov				edx, [ebp + 16]

	;displays info for this section
	display_string	info_1
	call			Crlf
	display_string	info_2
	call			Crlf
	jmp				another_number

error_message:
	display_string	error
	call			Crlf
another_number:
	push			ecx
	;read a string and validate it from the user
	push			[ebp + 24]		;read_size
	push			[ebp + 8]		;amount
	push			edx				;array
	push			[ebp + 12]      ;temp_num
	push			ebx				;current
	push			[ebp + 20]		;valid
	call			read_val
	mov				eax, 4
	add				edx, eax

	;if the number was valid, prepare for next, otherwise get a new one
	mov				eax, [ebp + 20]
	mov				esi, 0
	cmp				[eax], esi
	je				error_message
	add				ebx, 1
	pop				ecx
	loop			another_number

	pop				ebp
	ret				24
get_info ENDP


;Procedure: disp_info
;Description:displays the array, sum, and average
;preconditions:gets the string and its size
;postconditions:all info has been printed
;registers changed:ecx, esi, edi, eax, edx
disp_info PROC
	;stack frame
	push			ebp
	mov				ebp, esp

	;display array and setup
	display_string  disp_1
	call			Crlf
	mov				ecx, [ebp + 12]
	mov				esi, [ebp + 8]
	mov				edi, 0
	jmp				element
	
	;displays each comma after each element
add_comma:
	display_string	comma

	;displays each element
element:
	push			[esi + edi]
	push			[ebp + 16]
	call			write_val
	add				edi, 4
	loop			add_comma
	call			Crlf

	;sum
	display_string	disp_2
	mov				ecx, [ebp + 12]
	mov				eax, 0
	mov				edi, 0
addition:
	add				eax, [esi + edi]
	add				edi, 4
	loop			addition
	push			eax
	push			[ebp + 16]
	call			write_val
	call			Crlf

	;average
	display_string	disp_3
	mov				ecx, 10
	mov				edx, 0
	cdq
	idiv			ecx
	;call			WriteInt
	;call			Crlf
	push			eax
	push			[ebp + 16]
	call			write_val
	call			Crlf
	call			Crlf

	pop				ebp
	ret				12
disp_info ENDP


;Procedure: conclusion
;Description:says the conclusion
;preconditions:none
;postconditions:a string has been printed
;registers changed:none
conclusion PROC
	display_string	bye
	call			Crlf

	ret		
conclusion ENDP


main PROC
	call	introduction

	push	OFFSET read_size
	push	OFFSET valid
	push	OFFSET array
	push	OFFSET temp_num
	push	AMOUNT
	call	get_info

	push	OFFSET temp_num
	push	AMOUNT
	push	OFFSET array
	call	disp_info

	call	conclusion
	exit	; exit to operating system
main ENDP

END main