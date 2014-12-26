PROC dst:DWORD, src:DWORD, dpitch:DWORD, spitch:DWORD, width:DWORD, height:DWORD, mask:DWORD
	local diff1:DWORD
	local diff2:DWORD

	mov edi, dst
	mov esi, src

	mov ebx, width
	shl ebx, 2				; ebx = width * 4

	mov eax, dpitch		
	sub eax, ebx
	mov diff1, eax			; diff1 = dpitch - width * 4

	mov eax, spitch
	sub eax, ebx
	mov diff2, eax			; diff2 = spitch - width * 4

	mov ebx, mask
	mov edx, height

ALIGN
loop_line:
	mov ecx, width
ALIGN
loop_pixel:
	mov eax, [esi]
	cmp eax, ebx			; same to color key ??
	jz @f
	mov [edi], eax
@@:
	add esi, 4
	add edi, 4
	dec ecx
	jnz loop_pixel

	add edi, diff1
	add esi, diff2
	dec height
	jnz loop_line

	ret
ENDP


