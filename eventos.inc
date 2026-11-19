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
    ; Lista explicita: no aceptar IDs inexistentes ni el antiguo igual.
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
    invoke GetWindowText,[hEditOp],buffer,128
    stdcall AgregarEntrada,buffer,dword [esi+4]
    test eax,eax
    jz .fin
.actualizar:
    invoke SetWindowText,[hEditOp],buffer
    invoke SendMessage,[hEditOp],EM_SETSEL,-1,-1
    invoke SendMessage,[hEditOp],EM_SCROLLCARET,0,0
    jmp .fin
.limpiar:
    invoke SetWindowText,[hEditOp],Vacio
    invoke SetWindowText,[hEditRes],Vacio
    jmp .fin
.borrar:
    invoke GetWindowText,[hEditOp],buffer,128
    test eax,eax
    jz .fin
    mov byte [buffer+eax-1],0
    jmp .actualizar
.salir:
    invoke SendMessage,[hwnd],WM_CLOSE,0,0
.fin:
    xor eax,eax
    ret
endp

include 'validacion.inc'
