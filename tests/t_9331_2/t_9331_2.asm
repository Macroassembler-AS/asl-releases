	,	,	,	,	,	,	,	,	,	,	,	,	,	,	;
	jc	,start	,gf0				,	,	,	,	,	,	,		;
	lda	,mod0	,15h		,					,	,	,			; Initialization
	lda	,mod1	,15h		,					,	,	,			; LACK of Low: Lch, High:
	lda	,df0	,03h		,					,	,	,			; Rch are assumed for the
	lda	,df1	,02h		,					,	,	,			; following comment relative
	lda	,dp0	,01h		,					,	,	,			; to channel
	lda	,dp1	,21h		,					,	,	,			; Stupid Toshiba assembler disregards
		,gfs	,gf0		,	,	,	,	,	,	,	,	,		; ';' in first row!
start:
	,	,	,	,	,	,	,cp+	,	,	,x	,si0	,	,	,	;
	mads,	,x	,y	,0	,	,	,cp+	,	,	,	,	,	,	,	;
	,	,	,	,	,wf0	,	,cp+	,dp1	,+	,so1	,w0	,	,	,	; data output (Lch)
wait_edge:
	jnc	,wait_edge(down),LRF		,	,	,	,	,	,	,			; down edge waiting
	,	,	,	,	,	,	,cp+	,	,	,x	,si0	,	,	,	;
	mads,	,x	,y	,0	,	,	,cp+	,	,	,	,	,	,	,	;
	,	,	,	,	,wf0	,	,	,	,	,so1	,w0	,	,	,	; data output (Rch)
wait_SYNC:
	jmp0	,wait_SYNC(up-edge)	,	,	,	,	,	,	,				; SYNC reset waiting
	end
