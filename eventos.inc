; Constantes de validacion
MAX_DIGITOS     = 11
MAX_CARACTERES  = 30

proc ProcesarComando uses ebx esi edi, hwnd,wparam,lparam

    mov eax,[wparam]
    and eax,0FFFFh

    ; --------------------
    ; SALIR
    ; --------------------
    cmp eax,IDC_BTN_SALIR
    je .salir

    ; --------------------
    ; AC
    ; --------------------
    cmp eax,IDC_BTN_AC
    je .limpiar

    ; --------------------
    ; BORRAR
    ; --------------------
    cmp eax,IDC_BTN_BACK
    je .borrar

    ; --------------------
    ; = : no se escribe, reservado para la logica
    ; --------------------
    cmp eax,IDC_BTN_IGUAL
    je .fin

    ; --------------------
    ; ignorar mensajes que no son de botones de entrada
    ; --------------------
    cmp eax,IDC_BTN_PORC
    jb .fin
    cmp eax,IDC_BTN_PUNTO
    ja .fin

    ; =====================
    ; A partir de aca: el boton agrega texto al edit
    ; =====================

    invoke GetWindowText,[hEditOp],buffer,127
    mov ebx,eax                     ; ebx = longitud actual

    invoke GetWindowText,[lparam],btnText,15
    mov edx,eax                     ; edx = longitud del texto del boton

    ; --- Regla 1: limite total de caracteres ---
    mov eax,ebx
    add eax,edx
    cmp eax,MAX_CARACTERES
    ja .fin

    ; --- Regla 2: limite de 11 digitos ---
    call ContarDigitosBuffer        ; eax = digitos en buffer
    mov ecx,eax
    call ContarDigitosBoton         ; eax = digitos en btnText
    add ecx,eax
    cmp ecx,MAX_DIGITOS
    ja .fin

    ; --- Regla 3: el boton es simbolo y el ultimo del buffer tambien -> bloquear ---
    call BotonEsSimbolo             ; ZF=1 si el boton es simbolo
    jnz .agregar                    ; no es simbolo -> puede entrar
    ; el boton SI es simbolo
    cmp ebx,0
    je .chequear_primer_caracter    ; buffer vacio: validar primer caracter
    ; mirar ultimo caracter del buffer
    lea esi,[buffer]
    add esi,ebx
    dec esi
    mov al,[esi]
    call CaracterEsSimbolo          ; ZF=1 si el ultimo caracter es simbolo
    jz .fin                         ; ya hay simbolo -> bloquear

.agregar:

    ; Reservar espacio para el terminador
    mov eax,ebx
    add eax,edx
    cmp eax,127
    ja .fin

    lea edi,[buffer]
    add edi,ebx
    lea esi,[btnText]
    mov ecx,edx

.copiar:
    cmp ecx,0
    je .terminar
    mov al,[esi]
    mov [edi],al
    inc esi
    inc edi
    dec ecx
    jmp .copiar

.terminar:
    mov byte [edi],0
    invoke SetWindowText,[hEditOp],buffer
    jmp .fin

    ; Buffer vacio: el primer caracter solo puede ser digito o '-'
.chequear_primer_caracter:
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'-'
    je .agregar                     ; '-' al inicio permitido
    cmp al,'0'
    jb .fin
    cmp al,'9'
    ja .fin
    jmp .agregar                    ; digito al inicio permitido

.limpiar:
    invoke SetWindowText,[hEditOp],Vacio
    invoke SetWindowText,[hEditRes],Vacio
    jmp .fin

.borrar:
    invoke GetWindowText,[hEditOp],buffer,127
    cmp eax,0
    je .fin
    lea edi,[buffer]
    add edi,eax
    dec edi
    mov byte [edi],0
    invoke SetWindowText,[hEditOp],buffer
    jmp .fin

.salir:
    invoke SendMessage,[hwnd],WM_CLOSE,0,0

.fin:
    ret

endp


; -------------------- HELPERS --------------------

; Cuenta digitos (0..9) en buffer. Salida: eax
proc ContarDigitosBuffer
    push esi
    push ecx
    lea esi,[buffer]
    xor eax,eax
.ciclo:
    mov cl,[esi]
    cmp cl,0
    je .fin
    cmp cl,'0'
    jb .sig
    cmp cl,'9'
    ja .sig
    inc eax
.sig:
    inc esi
    jmp .ciclo
.fin:
    pop ecx
    pop esi
    ret
endp

; Cuenta digitos en btnText (largo en edx). Salida: eax
proc ContarDigitosBoton
    push esi
    push ecx
    push edx
    lea esi,[btnText]
    mov ecx,edx
    xor eax,eax
.ciclo:
    cmp ecx,0
    je .fin
    mov dl,[esi]
    cmp dl,'0'
    jb .sig
    cmp dl,'9'
    ja .sig
    inc eax
.sig:
    inc esi
    dec ecx
    jmp .ciclo
.fin:
    pop edx
    pop ecx
    pop esi
    ret
endp

; ZF=1 si el primer caracter de btnText es simbolo (+ - X x * / . %)
; NO considera digitos.
proc BotonEsSimbolo
    push eax
    push esi
    lea esi,[btnText]
    mov al,[esi]
    cmp al,'+'
    je .si
    cmp al,'-'
    je .si
    cmp al,'X'
    je .si
    cmp al,'x'
    je .si
    cmp al,'*'
    je .si
    cmp al,'/'
    je .si
    cmp al,'.'
    je .si
    cmp al,'%'
    je .si
    pop esi
    pop eax
    or  al,1        ; ZF=0
    ret
.si:
    pop esi
    pop eax
    xor al,al       ; ZF=1
    ret
endp

; ZF=1 si AL es simbolo (+ - X x * / . %)
proc CaracterEsSimbolo
    cmp al,'+'
    je .si
    cmp al,'-'
    je .si
    cmp al,'X'
    je .si
    cmp al,'x'
    je .si
    cmp al,'*'
    je .si
    cmp al,'/'
    je .si
    cmp al,'.'
    je .si
    cmp al,'%'
    je .si
    or  al,1
    ret
.si:
    xor al,al
    ret
endp


; WndProc del edit de operacion: bloquea TODA entrada por teclado.
proc EditSinTeclado hwnd,wmsg,wparam,lparam

    cmp [wmsg],WM_CHAR
    je .bloquear
    cmp [wmsg],WM_KEYDOWN
    je .bloquear
    cmp [wmsg],WM_KEYUP
    je .bloquear
    cmp [wmsg],WM_PASTE
    je .bloquear
    cmp [wmsg],WM_CUT
    je .bloquear
    cmp [wmsg],WM_CLEAR
    je .bloquear

    invoke CallWindowProc,[hEditOpOldProc],[hwnd],[wmsg],[wparam],[lparam]
    ret

.bloquear:
    xor eax,eax
    ret

endp