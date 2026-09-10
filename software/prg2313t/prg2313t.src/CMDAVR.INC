;**************************************************
	EXTRN	clr_SCK,pas_RES,act_RES
	EXTRN	imp_SCK,wrser,rdser
;
	PUBLIC	set_prg,ld_lb,ld_hb,rd_mem,erase
	PUBLIC	wr_mem,rd_sgn,rd_fuse,wr_fusL
;
.z80
;**************************************************
; Задать режим программирования МК
set_prg:di			; Запретить прерывания
	call	pas_RES		; RESET=1
	ld	b,10
	call	delay		; 10 msec
	call	clr_SCK		; SCK=0
	ld	b,30
	call	delay		; 30 msec
	call	act_RES		; RESET=0
; Передать команду перехода в режим программирования
	ld	c,0ach
	call	wrser		; wrser(0xac) SPI write (byte 1)
	ld	c,053h
	call	wrser		; wrser(0x53) SPI write (byte 2)
	call	rdser		; 	      SPI read  (byte 3)
	ld	a,053h
	cp	c		; if (rdser == 0x53)
	push	af
	call	rdser		; wrser(0x00) SPI write (byte 4)
	pop	af		;Z=1 OK
	ret
;--------------------------------------------
; Пауза (B) милисекунд
delay:	ld	c,218		;
; цикл длительностью 1 мсек
del_c:	dec	c		;4
	jr	nz,del_c	;12
	djnz	delay		;
	ret
;=============================================
;====== Chip Erase ===========================
erase:	call	set_prg
erase2:	ld	c,0ach
	call	wrser		; wrser(0xac) SPI write (byte 1)
	ld	c,80h
	call	wrser		; wrser(0x80) SPI write (byte 2)
	ld	c,04h
	call	wrser		; wrser(0x04) SPI write (byte 3)
	call	rdser		; wrser(0x00) SPI write (byte 4)
	ld	b,40		; delay 40mS
	jr	delay
;=================================================
;====== Write Program Memory =====================
; Вход:
;   hl = address
wr_mem:	ld	c,4ch
	call	wrser
	ld	c,h
	call	wrser
	ld	c,l
	call	wrser
	call	rdser
	ld	b,10		; delay 40mS
	jr	delay
;=================================================
;====== Load Program Memory, Low Byte ===========
; Вход:
;   hl = address
;   e  = low byte
ld_lb: 	ld	c,40h
	jr	ld_bt
;=================================================
;====== Load Program Memory, High Byte ==========
; Вход:
;   hl = address
;   e  = high byte
ld_hb:	ld	c,48h
ld_bt:	call	wrser		; wrser(0x48)   SPI write (byte 1)
	ld	c,h	
	call	wrser		; wrser(addrh)  SPI write (byte 2)
	ld	c,l
	call	wrser		; wrser(addrl)  SPI write (byte 3)
	ld	c,e		; wrser(u_data) SPI write (byte 4)
	jp	wrser
;=================================================
;====== 'R' - Read Program Memory ================
; Вход:
;	hl = address
; Выход:
; 	de = (addr)
rd_mem:	ld	c,28h		; read high Byte (order is different from Block Read!)
	call	wrser		; wrser(0x28)  SPI write (byte 1)
	ld	c,h
	call	wrser		; wrser(addrh) SPI write (byte 2)
	ld	c,l	
	call	wrser		; wrser(addrl) SPI write (byte 3)
	call	rdser		;              SPI read  (byte 4)
	ld	d,c		; High byte
	ld	c,20h		; read Low Byte
	call	wrser		; wrser(0x20)  SPI write (byte 1)
	ld	c,h	
	call	wrser		; wrser(addrh) SPI write (byte 2)
	ld	c,l	
	call	wrser		; wrser(addrl) SPI write (byte 3)
	call	rdser		;              SPI read  (byte 4)
	ld	e,c		; Low byte
	ret
;============================================
;====== Write Fuses Bytes ===================
; LOW BYTE             DEF     SET
;--------------------------------------------
; 7 - CLKDIV8           0       1
; 6 - CKOUT             1       1
; 5 - SUT1              1       1
; 4 - SUT0              0       0
; 3 - CKSEL3   		0       0
; 2 - CKSEL2            1       1
; 1 - CKSEL1            0       0
; 0 - CKSEL0            0       0
;--------------------------------------------
wr_fusL:ld	d,0a0h
	jr	wr_fus1
;--------------------------------------------
; HIGH BYTE	       DEF     SET	
;--------------------------------------------
; 7 - DWEN              1       1
; 6 - EESAVE            1       1
; 5 - SPIEN             0       0
; 4 - WDTON             1       1
; 3 - BODLEVEL2		1       1
; 2 - BODLEVEL1         1       1
; 1 - BODLEVEL0         1       1
; 0 - RSTDISBL          1	1
;--------------------------------------------
wr_fusH:ld	d,0a8h
	jr	wr_fus1
;--------------------------------------------
; EXTENDED BYTE        DEF     SET
;--------------------------------------------
; 0 - SELFPRGEN         1	1
;--------------------------------------------
wr_fusE:ld	d,0a4h
; e - fuse byte
wr_fus1:ld	c,0ach		;
	call	wrser		; wrser(0xAC)   SPI write (byte 1)
	ld	c,d
	call	wrser		; wrser(  d )   SPI write (byte 2)
	call	rdser		; wrser(0x00)   SPI write (byte 3)
	ld	c,e
	call	wrser
	ld	b,10		;Пауза 10 мсек
	jp	delay		; для завершения записи
;============================================
;====== Read Fuses Bytes ====================
; Выход:
;	D,E,C - 3 байта FUSES
rd_fuse:ld	hl,5000h
	call	rd_fus1
	ld	d,c		;d = LOW FUSES
	ld	hl,5808h
	call	rd_fus1
	ld	e,c		;e = HIGH FUSES
	ld	hl,5008h
rd_fus1:ld	c,h
	call	wrser		; wrser(0x50)   SPI write (byte 1)
	ld	c,l
	call	wrser		; wrser(0x08)   SPI write (byte 2)
	call	rdser		; wrser(0x00)   SPI write (byte 3)
	jp	rdser		;               SPI read  (byte 4)
				;c = EXTENDED FUSES
;============================================
;====== Read Signature Bytes ================
; Выход:
;	D,E,C - 3 байта сигнатуры
rd_sgn:	ld	L,02h		;2
	call	ca17
	ld	d,c             ; D - sign 1	0Ah
	ld	L,01h		;1
	call	ca17
	ld	e,c		; E - sign 2	91h
	ld	L,0		;0
ca17:	ld	c,30h
	call	wrser		; wrser(0x30)   SPI write (byte 1)
	call	rdser		; wrser(0x00)   SPI write (byte 2)
	ld	c,L
	call	wrser		; wrser(param)  SPI write (byte 3)
	jp	rdser		;               SPI read  (byte 4)
				; C - sign 3	1Eh
;=============================================
	end
