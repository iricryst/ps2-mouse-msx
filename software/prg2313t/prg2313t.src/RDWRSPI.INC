;****************************************
; Программирование контроллера через	*
;       разьем джойстика MSX		*
;****************************************
; MISO  ->: Up     (Joy-1pin) PA/0
; VDD   <-: +5V    (Joy-5pin)
; MOSI  <-: TrgA   (Joy-6pin) PB/0 | PB/2
; SCK   <-: TrgB   (Joy-7pin) PB/1 | PB/3
; RESET <-: STROBE (Joy-8pin) PB/4 | PB/5
; GND     : GND    (Joy-9pin)
;----------
; SEL 1|2 :                  PB/6=0|PB/6=1
;----------------------------------------
	PUBLIC	set_j1,set_j2,res_joy
	PUBLIC	imp_SCK,clr_SCK
	PUBLIC	pas_RES,act_RES
	PUBLIC	rdser,wrser
.z80
;****************************************
PSG_A	equ	0a0h	;reg address
PSG_W	equ	0a1h	;reg data WR
PSG_R	equ	0a2h	;reg data RD
;========================================
; Инициализация работы через порт джойстика 1
set_j1:	ld	a,15		;address PORT B
	out	(PSG_A),a
	in	a,(PSG_R)	;RD port B
	ld	(JOY_S0),a	; save
	and	08fh		;SEL=0 	 (PB/6=0)
				;RESET=0 (PB/4,5=0)
	or	00fh		;MOSI=1,SCK=1
	jr	ini_set
;========================================
; Инициализация работы через порт джойстика 2
set_j2:	ld	a,15		;address PORT B
	out	(PSG_A),a
	in	a,(PSG_R)
	ld	(JOY_S0),a
	and	0cfh		;RESET=0 (PB/4,5=0)
	or	04fh		;SEL=1,MOSI=1,SCK=1
ini_set:out	(PSG_W),a	;запись в порт B
	ld	(JOY_S1),a	; сохранить значение
	ret
;------------------------------------------------
; Восстановить состояние порта B
res_joy:ld	a,15		;порт B
	out	(PSG_A),a	;
	ld	a,(JOY_S0)
	out	(PSG_W),a
	ret
;--------------------------------------------------
JOY_S0:	db	0
JOY_S1:	db	0
;==================================================
;*** SCK = 0 (PB/3,1=0)
clr_sck:ld	a,15		;port B
	out	(PSG_A),a
	ld	a,(JOY_S1)
	jr	ini2
;--------------------------------------------------
; Формирование импульса SCK _/~\_ (PB/3,1)
imp_sck:ld	a,15		;port B
	out	(PSG_A),a
	ld	a,(JOY_S1)
	or	08h OR 02h	;SCK = 1 (TRGB2=H)
;				;	 (TRGB1=H)	
	out	(PSG_W),a
ini2:	and	0f7h AND 0fdh	;SCK = 0 (TRGB2=L)
;				;	 (TRGB1=L)
	out	(PSG_W),a
	or	04h OR 01h	;MOSI =1 (TRGA2=H)
;					 (TRGA1=H)
	ld	(JOY_S1),a
	ret
;--------------------------------------------------
;*** RESET control (PB/4,5)
pas_res:ld	a,15		;port B
	out	(PSG_A),a
	ld	a,(JOY_S1)
	or	30h		;RESET=1 (STB=1)
	jr	pa_res
;--------------------------------------------------
act_res:ld	a,15		;port B
	out	(PSG_A),a
	ld	a,(JOY_S1)
	and	0cfh		;RESET=0(STB=0)
pa_res:	out	(PSG_W),a
	ld	(JOY_S1),a
	ret
;==================================================
; принять байт (результат в регистре C)
rdser:	ld	c,0		;байт передачи = 0
; передать содержимое регистра C
wrser:	ld	b,8		;счетчик бит
	ld	a,15		;port B
	out	(PSG_A),a
;/---- цикл приема/передачи байта по SPI ----\
wrckl:
;---- передать бит (и принять одновременно)
	ld	a,(JOY_S1)	;MOSI = H
	rl	c		;CY <- data_out(7)
	jr	c,wr_cy
	and	0fbh AND 0feh	;MOSI = L (TRGA2=L)
;					  (TRGA1=L)
wr_cy:	out	(PSG_W),a	;
	ld	(JOY_S1),a
;---- принять бит
	ld	a,14		;port A
	out	(PSG_A),a
	in	a,(PSG_R)	;RD port A
	push	af
	call	imp_SCK		; импульс по SCK _/~\_
	pop	af
	rra			;data_in -> CY
	djnz	wrckl		; повторить 8 раз
; последний бит сохранить в результирующем регистре
	rl	c		;принятый байт
	ret
;=================================
	end
