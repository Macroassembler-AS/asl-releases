	cpu	8086
	page	0

	in	ax,dx
	in	al,dx
	in	ax,50h
	in	al,50h
	out	dx,ax
	out	dx,al
	out	50h,ax
	out	50h,al

	; Play a bit with the new callback-based address mode parser,
	; introduced with build 261:

	; Legal mod/rm combinations, in all permutations:

	mov	ax,[bx+si]		; 00xxx000
	mov	ax,[si+bx]
	mov	ax,[bx+di]		; 00xxx001
	mov	ax,[di+bx]
	mov	ax,[bp+si]		; 00xxx010
	mov	ax,[si+bp]
	mov	ax,[bp+di]		; 00xxx011
	mov	ax,[di+bp]
	mov	ax,[si]			; 00xxx100
	mov	ax,[di]			; 00xxx101
	mov	ax,[bp]			; 00xxx110 is [disp16], so converted to 01xxx110
	mov	ax,[bx]			; 00xxx111
	mov	ax,[1234h]		;
	mov	ax,[bx+si+12h]		; 01xxx000 
	mov	ax,[bx+12h+si]
	mov	ax,[si+12h+bx]
	mov	ax,[si+bx+12h]
	mov	ax,[12h+si+bx]
	mov	ax,[12h+bx+si]
	mov	ax,[bx+di+12h]		; 01xxx001
	mov	ax,[bx+12h+di]
	mov	ax,[di+12h+bx]
	mov	ax,[di+bx+12h]
	mov	ax,[12h+di+bx]
	mov	ax,[12h+bx+di]
	mov	ax,[bp+si+12h]		; 01xxx010
	mov	ax,[bp+12h+si]
	mov	ax,[si+12h+bp]
	mov	ax,[si+bp+12h]
	mov	ax,[12h+si+bp]
	mov	ax,[12h+bp+si]
	mov	ax,[bp+di+12h]		; 01xxx011
	mov	ax,[bp+12h+di]
	mov	ax,[di+12h+bp]
	mov	ax,[di+bp+12h]
	mov	ax,[12h+di+bp]
	mov	ax,[12h+bp+di]
	mov	ax,[si+12h]		; 01xxx100
	mov	ax,[12h+si]
	mov	ax,[di+12h]		; 01xxx101
	mov	ax,[12h+di]
	mov	ax,[bp+12h]		; 01xxx110
	mov	ax,[12h+bp]
	mov	ax,[bx+12h]		; 01xxx111
	mov	ax,[12h+bx]
	mov	ax,[bx+si+1234h]	; 10xxx000 
	mov	ax,[bx+1234h+si]
	mov	ax,[si+1234h+bx]
	mov	ax,[si+bx+1234h]
	mov	ax,[1234h+si+bx]
	mov	ax,[1234h+bx+si]
	mov	ax,[bx+di+1234h]	; 10xxx001
	mov	ax,[bx+1234h+di]
	mov	ax,[di+1234h+bx]
	mov	ax,[di+bx+1234h]
	mov	ax,[1234h+di+bx]
	mov	ax,[1234h+bx+di]
	mov	ax,[bp+si+1234h]	; 10xxx010
	mov	ax,[bp+1234h+si]
	mov	ax,[si+1234h+bp]
	mov	ax,[si+bp+1234h]
	mov	ax,[1234h+si+bp]
	mov	ax,[1234h+bp+si]
	mov	ax,[bp+di+1234h]	; 10xxx011
	mov	ax,[bp+1234h+di]
	mov	ax,[di+1234h+bp]
	mov	ax,[di+bp+1234h]
	mov	ax,[1234h+di+bp]
	mov	ax,[1234h+bp+di]
	mov	ax,[si+1234h]		; 10xxx100
	mov	ax,[1234h+si]
	mov	ax,[di+1234h]		; 10xxx101
	mov	ax,[1234h+di]
	mov	ax,[bp+1234h]		; 10xxx110
	mov	ax,[1234h+bp]
	mov	ax,[bx+1234h]		; 10xxx111
	mov	ax,[1234h+bx]

	; nesting

	mov	ax,[bx-(1234h-si)]	; equal to...
	mov	ax,[bx-1234h+si]
	mov	ax,[12h-(-bx)]		; equal to...
	mov	ax,[bx+12h]

	; that won't work, only adding of registers is possible...

	expect	1350
	mov	ax,[bp-si]
	endexpect

	; ...and registers may not be used as not as function arg...

	expect	1350
	mov	ax,[abs(bx)]
	endexpect

	; ...and each register may only be used once:

	expect	1350,1350
	mov	ax,[bp+bx]
	mov	ax,[si+di]
	endexpect

	; What made problems with the old "poor man's" parser:
        ; Local symbols -/--/---/+/++/+++ were accidentally
	; split, and the initial fix was rather a kludge:

-	nop
-	nop
-	nop
	mov	ax,cs:[-]
	mov	ax,cs:[--]
	mov	ax,cs:[---]

	; ...and due to parentheses being allowed, this now also works:

	mov	ax,cs:[(--)-5]
