	cpu	68000

	; single line with very long comment - should be truncated upon reading

	org	0	; this is a very long line wefeilurfuewvbdfwvwmhgedvwmhtdvwhdtwmhdtwdenhtgwdfrgwdfgwrdcngdrscdwm jdzwgeurg2 jezdgwdezgwr364rjdfgjdwejtzdfwjzre34jzehtdfwejtdwjzerfwjztfasdgvyxcmycvsmdfwejtdfwzetr7i34ti2wjkdgweszgfwejfg3i76r4t34ifjhsvfshvfshefgewurftge4r6t4i37gfmjbvfmsdvhcsmdfvgwefgw67herf7g34cd7wegfwe7wuefgrufzgegerjfgejwfgerfgjtzwfdjzswtewtzjtw 346r

	; again, a 'normal' line

	nop

	; line split over several lines

	move.l \
	d0, \
	d1

	; line that becomes too long by continuation, immediately followed by a normal one

	moveq	#-1,d0	; fugekrzfgwudzwjfuwjzftwfjdtwfdhwdfhw2fd2df2jetfhetf2edf2jztdf23 \
		  	  dfbvebfrgejkzfbkjzfdjbwerfwefzwjfgejwhrftwjedfjtfdjwtdfwjtdfwejtdfwedfwjte \
			  fvbkezrfgjweztgwjmhcvwnmcthwvfenctwevndwvdnwtfwnmhtcvwnhedvwjetdvfwtjdvfwt \
			  jfbvejz,rfbejzfemfzbs,cbas,djcfzbekfzgwebrfkzgjhtwvedfwnmgedvwdetvwtedvwtew
	nop

	; something actually useful

prayer	equ	"Our Father in heaven, Hallowed be your name, Your kingdom comes, Your will be done, As in heaven, even so on earth, Give us today our daily bread, And forgive us our guilt, Just as we also forgive [those who are guilty upon us], And lead us not in temptation, But save us from evil, [For yours is the kingdom, and the power, and the glory, for eternity, Amen.]"
	dc.b	prayer
