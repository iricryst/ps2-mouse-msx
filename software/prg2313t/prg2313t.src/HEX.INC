	PUBLIC	rd_hex,wr_hex
;
CR	equ	13
LF	equ	10
EOF	equ	1Ah
;
.z80
	CSEG
;********************************************************
; Чтение в HEX-формате
; DE-адрес загрузки в память
; BC-Дескриптор файла
;/----------------------------------------------------
rd_hex:	ld	(des_fl),bc	;
	ld	hl,0
	ld	(cn_rdbt),hl	;обнулить счетчик чтения
	jr	wt_dvt		;ждем ':'
;/--- ЦИКЛ ЧТЕНИЯ ФАЙЛА ---------
CIKLRH:
wt_cr:	call	rd_byte		;чтение текущего байта
	ret	c		;конец файла
	cp	CR		;ждем CR
	jr	nz,wt_cr	; пока нет
wt_lf:	call	rd_byte		;принять след.символ
	ret	c		;
	cp	LF		;должен быть перевод строки
	jr	nz,wt_cr	; иначе снова ждать CR
wt_dvt:	call	rd_byte		;принять след.символ
	ret	c
	cp	':'		;должно быть ':'
	jr	nz,wt_cr	;иначе начать с начала
; принять строку в HEX-формате
	ld	c,0		;0=> контр.сумма
	call	RD_AHEX		;чтение AKK в HEX-формате
	ret	c		;ошибка формата
	ld	b,a		;число байт в строке
; прочитать два байта адреса текущего блока (строки)
	call	RD_AHEX
	ret	c		;ошибка формата
	ld	h,a		;ст.байт адреса блока
	call	RD_AHEX
	ret	c		;ошибка формата
	ld	l,a		;мл.байт адреса блока
; !!!!!
	call	RD_AHEX		;прочитать байт MODE
	ret	c		;ошибка формата
	or	a
	jr	z,mode_0	;прием данных
; обычно байт MODE равен 0, но не всегда это надо учесть
	dec	a
	jr	z,mode_1	;=1 - конец данных
	dec	a
	jr	z,mode_2	;=2 - адрес блока (=0000h)
	scf
	ret			;ошибка формата
mode_1:
; MODE=1 - признак конца данных
	ld	a,b		;обьем данных
	or	a		;должно быть = 0
	jr	z,EXITRH	;
	scf
	ret			;  ошибка формата
mode_2:
; MODE=2 - два следующих байта определяют адрес блока
	ld	a,b		;байт должно быть два
	sub	2		; иначе
	jr	nz,ex_err	;  ошибка формата
; адрес должен = 0000h
	call	RD_AHEX		;старший байт адреса
	ret	c
	or	a
	jr	nz,ex_err
	call	RD_AHEX		;младший байт адреса
	ret	c
	or	a
	jr	nz,ex_err	;  ошибка формата
; проверить байт КС
	call	RD_AHEX		;чтение байта контр.суммы
	ret	c		; выход по ошибке формата
	jr	nz,ex_err	; выход по ошибке контр.суммы
	jr	CIKLRH		;продолжить прием файла
;---------------------------------
; проверить контрольную сумму
mode_0:	add	hl,de		;текущий адрес => HL
;/-------------------------------
; прочитать байты данных и записать в буфер
CKLRB:	call	RD_AHEX		;чтение байта
	ret	c		;ошибка формата
	ld	(hl),a		;байт в буфер
	inc	hl
	djnz	CKLRB		;продолжить прием строки
;\-------------------------------
; закончились байты данных
	call	RD_AHEX		;чтение байта контр.суммы
	ret	c		; выход по ошибке формата
	jr	nz,ex_err	; выход по ошибке контр.суммы
	jr	CIKLRH		;продолжить прием файла
;\-------------------------------
EXITRH:	call	RD_AHEX		;чтение байта контр.суммы
	ret	c		;ошибка формата
	ret	z		;CY=0 Выход без ошибки
ex_err:	scf
	ret
;===================================================
; чтение байта в HEX-формате по ниблам
RD_AHEX:
	call	RD_AHC		;чтение старшего нибла
	ret	c		;возврат с ошибкой CY=1
	rlca
	rlca
	rlca
	rlca
	push	de
	ld	e,a
	call	RD_AHC		;чтение младшего нибла
	jr	c,ex_ahex	;возврат с ошибкой CY=1
	or	e
	ld	e,a
	add	a,c
	ld	c,a		;контрольная сумма
	or	a		;CY=0 и проверка KS
	ld	a,e		;результат в AKK
ex_ahex:pop	de
	ret			;нормальный возврат
; прием нибла в ASCII-коде
RD_AHC:	call	rd_byte		;чтение текущего байта
	ret	c		;конец файла
	and	7Fh		;маска D7
	sub	'0'
	ret	c		;ошибка < '0'
	add	a,0E9h
	ret	c		;ошибка > 'F'
	add	a,06h
	jp	p,METKA		;< 'A'
	add	a,07h
	ret	c		;ошибка <'a',>'9'
METKA:	add	a,0Ah
	or	a		;CY=0
	ret			;возврат без ошибки
;********************************************************
; Записать блок с HL(adr1) до DE(adr2)
wr_hex:	call	wr_dvt		;':'
	jr	CIKLWH+3	;начать цикл передачи
;/----------------------------------------------
CIKLWH:	call	wr_met		;CR,LF,':'
	push	HL		;запомнить adr1
	ld	BC,0010H
; подсчитать число байт в текущей строке
CIKLCB:	INC	B		;счетчик байт в строке
	DEC	C
	jr	Z,ENDWB		;строка длиной 16 байт
	call	ICMXD		;
	jr	NC,CIKLCB	;
; строка длиной B (меньше 16 байт)
ENDWB:	pop	HL		;HL=adr1
	push	DE		;запомнить adr2
	ld	D,0		;0=>контр.сумма
	ld	A,B		;a=числу байт в строке
	call	WAHEX		;запись числа байт в строке в HEX
	call	WHLHEX		;запись адреса в HEX
	XOR	A		;
	call	WAHEX		;запись 00-го байта
;/--------------------------------
CIKLWB:	ld	A,(HL)		;текущий байт данных
	call	WAHEX
	INC	HL
	DEC	B		;если байт не последний
	JP	NZ,CIKLWB	; то продолжить передачу
;\--------------------------------
	XOR	A
	SUB	D		;a=контрольной сумме(NOT)
	call	WAHEX		;записать KS
	pop	DE		;DE=adr2
	DEC	HL
	call	ICMXD		;если строка не завершена
	jr	NC,CIKLWH	;то продолжить передачу
;\-------------------------------
	call	wr_met		;CR,LF,':'
	XOR	A
	ld	D,A		;0=>KS
	call	WAHEX		;число байт=0
	ld	hl,0
	call	WHLHEX		;
	xor	A		;
	call	WAHEX		;
	XOR	A
	SUB	D		;инверсия KS
	call	WAHEX		;запись KS
	ld	a,CR
	call	wr_byte
	ld	a,LF
	call	wr_byte
	ld	C,1AH		;признак EOF
	jp	wr_byte		; в конец файла
;--------------------------------
; записать HL в HEX-формате
WHLHEX:	ld	A,H
	call	WAHEX
	ld	A,L
WAHEX:	push	AF		;zapomnitx bajt
	rrca
	rrca
	rrca
	rrca
	and	0FH
	call	HEXASC		;HEX=>ASCII(reg C)
	call	wr_byte		;запись старшего нибла
	pop	AF
	push	AF
	and	0FH
	call	HEXASC		;
	call	wr_byte		;запись младшего нибла
	pop	AF
	add	A,D
	ld	D,A		;контрольная сумма
	or	a		;возврат
	ret			; без ошибки (CY=0)
; перевод AKK в HEX-формат
HEXASC:	add	a,90H
	daa
	adc	a,40H
	daa
	ld	C,A
	ret
;--------------------------------
; Записать метку начала блока
wr_met:	ld	C,CR
	call	wr_byte
	ld	C,LF
	call	wr_byte
wr_dvt:	ld	C,':'
	JP	wr_byte
;----------------------------------------
; инкремент указателя (HL) и сравнение
; его с концом блока (DE)
; CY=1 если HL > DE или HL=0
ICMXD:	inc	HL
	ld	A,H
	or	L
	scf
	ret	Z		;Z=1:CY=1 (HL=0)
;	HL-DE
	ld	A,E
	sub	L
	ld	A,D
	sbc	A,H
	ret
;==============================================
; чтение одного байта из файла
rd_byte:push	hl
	push	de
	push	bc
	ld	bc,(cn_rdbt)	;счетчик байт
	ld	a,c
	or	b
	jr	nz,rd_tbt	;буфер не пуст
; буфер данных пуст, прочитать сектор
	ld	de,bf_rdbt	;адрес буфера
	ld	hl,512		;размер буфера
	ld	bc,(des_fl)	;дескриптор файла
	call	_read##		;читаем след.блок
	ld	a,l
	or	h
	jr	nz,yes_rd	;что-то прочиталось
; ничего не прочитано - конец файла
	scf			;CY=1, нет больше данных
	jr	ex_rdbt+1	; выход с ошибкой
;--------------------------------
yes_rd:	ld	(cn_rdbt),hl	;текущий счетчик
	ld	c,l
	ld	b,h		; -> BC
	ld	hl,bf_rdbt	;адрес
	ld	(ad_rdbt),hl	; текущего байта
; читаем текущий байт из буфера
rd_tbt:	dec	bc
	ld	(cn_rdbt),bc
	ld	hl,(ad_rdbt)
	ld	a,(hl)		;текущий байт
	inc	hl
	ld	(ad_rdbt),hl
ex_rdbt:or	a		;CY=0 OK
	pop	bc
	pop	de
	pop	hl
	ret
;=======================================
; Запись одного байта в текущий файл
wr_byte:push	hl
	push	de
	push	bc
	ld	bc,(cn_wrbt)	;счетчик
	ld	hl,(ad_wrbt)	;адрес
        ld      (hl),a		;текущий байт в буфер
	inc	hl
	cp	EOF		;символ конца файла ?
	jr	z,yes_eof
; еще не конец файла
	ld	(ad_wrbt),hl	;новый адрес
	dec	bc
	ld	(cn_wrbt),bc
	ld	a,c
	or	b
	jr	nz,ex_wrbt	;
; буфер еще не заполнен
end_buf:
; записать буфер
	ld	de,bf_rdbt	;адрес буфера
	ld	hl,512		;размер буфера
	ld	bc,(des_fl)	;дескриптор файла
	call	_write##
	ld	hl,512
	ld	(cn_wrbt),hl	;счетчик = 512
	ld	hl,bf_rdbt
	ld	(ad_wrbt),hl	;адрес в начало
	jr	ex_wrbt		;выход ОК
; если поступил символ EOF, то закрыть файл
yes_eof:ld	a,c		;
	or	b
	jr	z,no_wrbt
; записать остаток из буфера
	ld	de,bf_rdbt	;адрес буфера
	ld	hl,512		;размер буфера
	ld	bc,(des_fl)	;дескриптор файла
	call	_write##	;
no_wrbt:call	_close##
ex_wrbt:or	a		;CY=0 OK
	pop	bc
	pop	de
	pop	hl
	ret
;****************************************
	DSEG
des_fl:		ds	2	;дескриптор файла
cn_rdbt:	ds	2	;счетчик буфера чтения
ad_rdbt:	ds	2	;адрес в буфере чтения
cn_wrbt:	ds	2	;счетчик буфера записи
ad_wrbt:	ds	2	;адрес в буфере записи
bf_rdbt:	ds	512	;буфер чтения
;****************************************
	end
