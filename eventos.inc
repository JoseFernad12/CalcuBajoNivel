; Entrada de botones exclusivamente: filtrar notificaciones y validar el control.
proc ProcesarComando uses ebx esi edi, hwnd,wparam,lparam
    mov eax,[wparam]
    shr eax,16
    cmp eax,BN_CLICKED
    jne .fin
    cmp [lparam],0
    je .fin
    mov ebx,[wparam]
    and ebx,0FFFFh
    invoke GetDlgItem,[hwnd],ebx
    cmp eax,[lparam]
    jne .fin
    test eax,eax
    jz .fin
    cmp ebx,IDC_BTN_SALIR
    je .salir
    cmp ebx,IDC_BTN_AC
    je .limpiar
    cmp ebx,IDC_BTN_BACK
    je .borrar
    cmp ebx,IDC_BTN_IGUAL
    je .igual
    ; Only known button IDs can add input characters.
    mov esi,EntradasBotones
.buscar:
    mov eax,[esi]
    test eax,eax
    jz .fin
    cmp eax,ebx
    je .entrada
    add esi,8
    jmp .buscar
.entrada:
    invoke GetWindowText,[hEditOp],buffer,MAX_ENTRADA+1
    stdcall AgregarEntrada,buffer,dword [esi+4]
    test eax,eax
    jz .fin
.actualizar:
    stdcall EvaluarExpresion,buffer,ResultadoTexto
    cmp eax,1
    sete al
    movzx eax,al
    mov [ResultadoValido],eax
    invoke SetWindowText,[hEditRes],ResultadoTexto
.mostrar_entrada:
    invoke SetWindowText,[hEditOp],buffer
    invoke SendMessage,[hEditOp],EM_SETSEL,-1,-1
    invoke SendMessage,[hEditOp],EM_SCROLLCARET,0,0
    jmp .fin
.limpiar:
    mov [ResultadoValido],0
    mov byte [ResultadoTexto],0
    invoke SetWindowText,[hEditOp],Vacio
    invoke SetWindowText,[hEditRes],Vacio
    jmp .fin
.borrar:
    invoke GetWindowText,[hEditOp],buffer,MAX_ENTRADA+1
    test eax,eax
    jz .fin
    mov byte [buffer+eax-1],0
    jmp .actualizar
.igual:
    cmp [ResultadoValido],1
    jne .fin
    ; Only complete, bounded numeric results can become the next operand.
    invoke lstrcpy,buffer,ResultadoTexto
    jmp .mostrar_entrada
.salir:
    invoke SendMessage,[hwnd],WM_CLOSE,0,0
.fin:
    xor eax,eax
    ret
endp

include 'validacion.inc'
