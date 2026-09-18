bits 32

mov ebx, eax
mov [ebx], eax
mov [ebx], ax
mov [ebx], al

mov eax, [ebx]
mov ax, [ebx]
mov al, [ebx]

; 0x89CI
mov ebx, eax ; ind 3 
mov ecx, eax ; ind 1
mov edx, eax ; ind 2

mov bl, al
mov cl, AL
mov dl, al

mov eax, ebx
mov eax, ecx
mov eax, edx

push eax
pop eax