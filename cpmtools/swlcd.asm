;	Switch to next LCD status display tool
;
;	Copyright (C) 2024 by Thomas Eberhardt
;
	title	'Switch to next LCD status display tool'

	.8080
	aseg
	org	100h

hwctl	equ	0a0h
hwunlk	equ	0aah
lcdnext	equ	8

	in	hwctl		; check if hardware control port is unlocked
	ora	a
	jz	unlckd
	mvi	a,hwunlk	; unlock hardware control port
	out	hwctl
unlckd:	mvi	a,lcdnext	; switch to next LCD status display
	out	hwctl
	ret

	end
