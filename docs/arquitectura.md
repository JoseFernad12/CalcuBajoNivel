# README.md — Calculadora en ASM (Win32, FASM/RADASM)

Te paso el README completo, organizado por secciones para que puedan estudiarlo por partes y defenderlo sin drama.

---

# Calculadora en Ensamblador x86 (Win32)

Calculadora de escritorio hecha 100% en ensamblador x86 de 32 bits, usando **FASM** como ensamblador y **RADASM** como IDE. Corre sobre la API de Windows de 32 bits (`kernel32`, `user32`, `gdi32`).

## Características

- Interfaz gráfica Win32 (ventana + botones + 2 edit controls)
- Entrada **exclusivamente por botones** (teclado bloqueado)
- Soporta: `+`, `-`, `X` (multiplicación), `/`, `%` (porcentaje), `.` (decimal)
- Soporta **un solo operador binario** por expresión
- Soporta números negativos (`-1-3`, `-1*-2`, `-1/-2`, `5+-3`)
- Resultados con **hasta 2 decimales**
- Manejo de errores (`Error` en pantalla para casos inválidos)
- **A prueba de crasheos** (todo overflow se detecta antes del `idiv`/`imul`)

## Limitaciones conocidas

- **Máximo de entrada**: 11 dígitos totales (por regla de validación)
- **Rango numérico efectivo**: ±21.474.836,00 (por usar `int32` escalado ×100)
- **Decimales**: 2 fijos, se trunca (no redondea)
- Números >8 dígitos suelen dar `Error` al apretar `=` (fuera de rango)
- Solo **un operador binario** por expresión (`5+3*2` no está soportado)

## Estructura del proyecto

```
/
├── Main.asm           ← Programa principal + WndProc principal
├── controles.inc      ← Definición de IDs de controles (constantes)
├── ventana.inc        ← Creación de la ventana y todos los controles
├── eventos.inc        ← Manejo de clics de botones + validación de entrada
└── operadores.inc     ← Lógica matemática (parser + cómputo + formateo)
```

---

## 1) `controles.inc`

Solo contiene **constantes numéricas** para identificar cada control (botón, edit) con un nombre simbólico. Así el resto del código no usa números mágicos.

```asm
IDC_EDIT_OP    = 1000   ; Edit de entrada (donde se tipea)
IDC_EDIT_RES   = 1001   ; Edit de resultado (solo lectura)

IDC_BTN_AC     = 1100   ; Botón AC (limpiar todo)
IDC_BTN_BACK   = 1101   ; Botón <- (borrar último)
IDC_BTN_PORC   = 1102   ; Botón %
IDC_BTN_DIV    = 1103   ; Botón /

IDC_BTN_7      = 1110   ; ... botones numéricos y operadores ...
IDC_BTN_8      = 1111
IDC_BTN_9      = 1112
IDC_BTN_MUL    = 1113   ; Botón X (multiplicación)

IDC_BTN_4      = 1120
IDC_BTN_5      = 1121
IDC_BTN_6      = 1122
IDC_BTN_SUB    = 1123   ; Botón -

IDC_BTN_1      = 1130
IDC_BTN_2      = 1131
IDC_BTN_3      = 1132
IDC_BTN_ADD    = 1133   ; Botón +

IDC_BTN_0      = 1140
IDC_BTN_IGUAL  = 1141   ; Botón =
IDC_BTN_PUNTO  = 1142   ; Botón .
IDC_BTN_SALIR  = 1200   ; Botón Salir
```

**Por qué importa**: cuando el usuario hace clic en un botón, Windows envía un mensaje `WM_COMMAND` con el ID del botón en `wParam`. `eventos.inc` compara ese ID contra estas constantes para saber qué botón se apretó.

---

## 2) `Main.asm` — programa principal

### `.data` — Datos globales

| Variable | Tipo | Para qué |
|---|---|---|
| `ClassName` | string | Nombre de la clase de ventana (`CalculadoraFASM`) |
| `AppName` | string | Título de la ventana (`Ventana 1`) |
| `EditClass`, `ButtonClass` | string | `EDIT` y `BUTTON` (clases estándar de Win32) |
| `txtAC`, `txtBack`, `txt7`... | string | Texto de cada botón |
| `hInstance` | dword | Handle del módulo (lo da `GetModuleHandle`) |
| `hWnd` | dword | Handle de la ventana principal |
| `hEditOp` | dword | Handle del edit de entrada |
| `hEditRes` | dword | Handle del edit de resultado |
| `hEditOpOldProc` | dword | WndProc original del edit (para subclaseo) |
| `wc` | `WNDCLASS` | Estructura para registrar la clase |
| `msg` | `MSG` | Estructura del bucle de mensajes |
| `buffer` | 128 bytes | Buffer temporal para leer el texto del edit |
| `btnText` | 16 bytes | Buffer temporal para leer el texto de un botón |

### `start:` — Punto de entrada

1. `GetModuleHandle,0` → obtiene `hInstance`
2. Rellena la estructura `WNDCLASS` con:
   - `lpfnWndProc = WndProc` (la función que maneja los mensajes)
   - `hIcon`, `hCursor`, `hbrBackground` (ícono, cursor, fondo gris)
   - `lpszClassName = ClassName`
3. `RegisterClass` → registra la clase en Windows
4. `CreateWindowEx` → crea la ventana con tamaño `250×340` (medidas modificadas para el diseño final)
5. `ShowWindow` + `UpdateWindow` → la muestra
6. **Bucle de mensajes**:
   ```asm
   mensaje:
       invoke GetMessage,msg,0,0,0
       cmp eax,1
       jb salir            ; si GetMessage devuelve 0 o -1, salir
       invoke TranslateMessage,msg
       invoke DispatchMessage,msg
       jmp mensaje
   ```
   Este bucle es **el corazón de cualquier app Win32**: pide mensajes a Windows (`GetMessage`), los traduce (`TranslateMessage`, teclas) y los despacha al WndProc (`DispatchMessage`).
7. `salir:` → `ExitProcess` con el código de salida

### `proc WndProc hwnd,wmsg,wparam,lparam`

Es **la función que Windows llama** cada vez que ocurre algo en la ventana (clic, redimensión, cierre, etc.).

Firma `stdcall` (los 4 parámetros están en la pila y se acceden con `[hwnd]`, `[wmsg]`, etc. porque FASM `proc` con `stdcall` + `uses` maneja el frame).

Flujo:

```asm
cmp [wmsg],WM_CREATE
je .crear           ; se dispara UNA VEZ al crear la ventana

cmp [wmsg],WM_COMMAND
je .comando         ; clic en un botón (o cambio en un edit)

cmp [wmsg],WM_DESTROY
je .destruir        ; la ventana se está cerrando
```

| Mensaje | Qué hace |
|---|---|
| `WM_CREATE` | Llama a `CrearControles` (crea los edits y botones) |
| `WM_COMMAND` | Llama a `ProcesarComando` (maneja clics) |
| `WM_DESTROY` | Llama a `PostQuitMessage` (rompe el bucle de mensajes) |
| **cualquier otro** | `DefWindowProc` (comportamiento por defecto de Windows) |

**Por qué importa**: no procesamos mensajes que no nos interesan — los delegamos a Windows.

Al final del archivo:

```asm
include 'ventana.inc'
include 'eventos.inc'
include 'operadores.inc'
.end start
```

Incluye los otros 3 archivos. `.end start` marca el final del archivo fuente y el punto de entrada.

---

## 3) `ventana.inc` — creación de controles

### `proc CrearControles hwnd`

Llamada desde `WM_CREATE`. Crea **todos** los controles hijos (edits y botones).

#### Edit de entrada (donde se tipea)

```asm
invoke CreateWindowEx,\
    WS_EX_CLIENTEDGE,       ; borde hundido
    EditClass,              ; clase "EDIT"
    Vacio,                  ; texto inicial (vacío)
    WS_CHILD or WS_VISIBLE or ES_AUTOHSCROLL,
    9,10,232,26,            ; posición y tamaño
    [hwnd],                 ; ventana padre
    IDC_EDIT_OP,            ; ID
    [hInstance],
    0
mov [hEditOp],eax
```

Luego:

```asm
invoke SetWindowLong,[hEditOp],GWL_WNDPROC,EditSinTeclado
mov [hEditOpOldProc],eax
```

**Esto es el "subclaseo"**: le cambia el WndProc al edit para que use nuestra función `EditSinTeclado` en lugar del default de Windows. El WndProc original se guarda en `hEditOpOldProc` para restaurarlo si hiciera falta.

**Efecto**: el edit de entrada ya no procesa teclado → entrada solo por botones.

#### Edit de resultado (solo lectura)

```asm
invoke CreateWindowEx,\
    WS_EX_CLIENTEDGE,\
    EditClass,\
    Vacio,\
    WS_CHILD or WS_VISIBLE or ES_RIGHT or ES_READONLY,\
    9,44,232,26,\
    [hwnd],IDC_EDIT_RES,[hInstance],0
mov [hEditRes],eax
```

- `ES_RIGHT` → texto alineado a la derecha (tipo calculadora)
- `ES_READONLY` → no se puede editar

#### Botones

Todos siguen el mismo patrón:

```asm
invoke CreateWindowEx,0,ButtonClass,txt7,WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON,9,124,52,32,[hwnd],IDC_BTN_7,[hInstance],0
```

- `ButtonClass` = `"BUTTON"`
- `txt7` = `"7"` (texto del botón)
- `WS_CHILD or WS_VISIBLE or BS_PUSHBUTTON` = estilo de botón hijo visible
- `9,124,52,32` = x, y, ancho, alto
- `IDC_BTN_7` = ID que identifica el botón

Layout (250×340):
```
[ AC ] [ <- ] [ % ] [ / ]     ← fila 1
[  7 ] [  8 ] [ 9 ] [ X ]     ← fila 2
[  4 ] [  5 ] [ 6 ] [ - ]     ← fila 3
[  1 ] [  2 ] [ 3 ] [ + ]     ← fila 4
[  0 ] [  = ] [ . ] [Salir]   ← fila 5
```

---

## 4) `eventos.inc` — validación y manejo de clics

Este archivo es **el más denso**. Contiene la validación de entrada (para que el usuario no pueda tipear cosas inválidas) y el WndProc que bloquea el teclado.

### Constantes

```asm
MAX_DIGITOS     = 11   ; máximo de dígitos totales en la entrada
MAX_CARACTERES  = 30   ; máximo de caracteres totales (dígitos + símbolos)
```

### `proc ProcesarComando uses ebx esi edi, hwnd,wparam,lparam`

Llamada desde `WM_COMMAND`. Decide qué hacer según el ID del botón.

#### Flujo general

```asm
mov eax,[wparam]
and eax,0FFFFh          ; el ID está en los 16 bits bajos
```

Luego chequea **casos especiales**:

| ID | Salta a | Qué hace |
|---|---|---|
| `IDC_BTN_SALIR` | `.salir` | `SendMessage WM_CLOSE` (cierra la ventana) |
| `IDC_BTN_AC` | `.limpiar` | Vacía los dos edits |
| `IDC_BTN_BACK` | `.borrar` | Borra el último carácter del edit de entrada |
| `IDC_BTN_IGUAL` | `.calcular` | Llama a `ResolverOperacion` (evaluá y muestra) |
| fuera del rango 1102..1142 | `.fin` | Ignorar (mensajes que no nos importan) |

Si el botón **agrega texto** (números, operadores, punto, %), ejecuta las **5 reglas de validación** en orden:

#### Regla 1: límite total de caracteres

```asm
mov eax,ebx          ; ebx = largo actual del buffer
add eax,edx          ; edx = largo del texto del botón
cmp eax,MAX_CARACTERES
ja .fin              ; si excede 30, ignorar el clic
```

Evita que el buffer de 127 se llene y que el edit se vea saturado.

#### Regla 2: límite de dígitos totales

```asm
call ContarDigitosBuffer  ; cuenta '0'-'9' en todo el buffer
mov ecx,eax
call ContarDigitosBoton   ; cuenta dígitos en el botón (0 o 1)
add ecx,eax
cmp ecx,MAX_DIGITOS       ; si > 11, bloquear
ja .fin
```

Los símbolos **no cuentan** para este límite.

#### Regla 3: punto decimal

```asm
call BotonEsPunto
jnz .no_es_punto
cmp ebx,0
je .fin                    ; no empezar con '.'
; el carácter anterior debe ser dígito
lea esi,[buffer]
add esi,ebx
dec esi
mov al,[esi]
cmp al,'0'
jb .fin
cmp al,'9'
ja .fin
; el número actual no debe tener ya un punto
call PuntoYaUsadoEnNumeroActual
jz .fin
jmp .agregar
.no_es_punto:
```

Impide:
- `3..1` (dos puntos)
- `.5` (punto al inicio)
- `3.+` (punto sin dígitos después)

#### Regla 4: operadores (`+ - X /`)

Esta es la más compleja. La idea central:

- **`+ X /`**: solo pueden aparecer **después de un dígito o `%`**, y **solo uno por expresión**.
- **`-`**: puede ser binario (resta) o unario (negativo). Es binario si va **después de un dígito o `%`**, y unario si va **al inicio o después de `+ X /`**.

```asm
call BotonEsOperador
jnz .no_operador            ; si no es operador, saltear

cmp ebx,0
je .operador_inicio         ; buffer vacío: solo '-' permitido

; ver último carácter
lea esi,[buffer]
add esi,ebx
dec esi
mov al,[esi]

call BotonEsMenos
jz .es_menos                ; el botón es '-'

; es + X /: debe ir después de dígito o '%'
; y no puede haber otro binario
; (si falla, jmp .fin)
```

**Caso especial `-` (`.es_menos`)**:
- Si el anterior es `-` → bloquear (no `5--3`)
- Si el anterior es `.` → bloquear (no `3.-5`)
- Si el anterior es `+ X /` → **es unario**, permitir
- Si el anterior es dígito o `%` → **es binario**, contar si ya hay otro, y si sí, bloquear

**Caso buffer vacío**: solo se permite `-` (negativo al inicio). `+5` se bloquea.

#### Regla 5: porcentaje

```asm
call BotonEsPorcentaje
jnz .no_porcentaje
cmp ebx,0
je .fin                    ; no empezar con '%'
; último debe ser dígito
; solo 1 % por expresión
call ContarPorcentajes
cmp eax,1
jae .fin
jmp .agregar
```

Y si el botón es **dígito** pero el anterior es `%` → bloquear (`5%5` inválido).

#### `.agregar` — copia el texto al buffer

Una vez que **todas** las reglas pasan, copia `btnText` al final de `buffer` y actualiza el edit:

```asm
invoke SetWindowText,[hEditOp],buffer
```

### `.borrar` — retroceso

```asm
invoke GetWindowText,[hEditOp],buffer,127
cmp eax,0
je .fin
lea edi,[buffer]
add edi,eax
dec edi                    ; apunta al último carácter
mov byte [edi],0           ; lo pone en 0 (fin de string)
invoke SetWindowText,[hEditOp],buffer
```

Truco clásico: como es C-string, "borrar" es poner el terminador nulo una posición antes.

### Helpers de `eventos.inc`

| Helper | Para qué |
|---|---|
| `ContarDigitosBuffer` | Cuenta caracteres `'0'-'9'` en `buffer` |
| `ContarDigitosBoton` | Cuenta dígitos en `btnText` |
| `BotonEsPunto` | Devuelve `ZF=1` si el botón es `.` |
| `BotonEsMenos` | Devuelve `ZF=1` si el botón es `-` |
| `BotonEsOperador` | `ZF=1` si botón ∈ `{+, -, X, x, *, /}` |
| `BotonEsPorcentaje` | `ZF=1` si botón es `%` |
| `CaracterEsOperador` | `ZF=1` si AL ∈ `{+, -, X, x, *, /}` |
| `PuntoYaUsadoEnNumeroActual` | `ZF=1` si el **número actual** ya tiene `.` |
| `ContarOperadoresBinarios` | Cuenta operadores binarios (excluye `-` unario) |
| `ContarPorcentajes` | Cuenta `%` en el buffer |

**Nota sobre `ContarOperadoresBinarios`**: recorre el buffer recordando si el carácter anterior era "operando" (dígito, punto o `%`). Si encuentra `+ X /` → cuenta +1. Si encuentra `-`, cuenta +1 **solo si el anterior era operando** (si no, es unario).

### `proc EditSinTeclado hwnd,wmsg,wparam,lparam`

WndProc que reemplaza al del edit de entrada (vía `SetWindowLong`).

```asm
cmp [wmsg],WM_CHAR
je .bloquear                ; caracteres tipeados
cmp [wmsg],WM_KEYDOWN
je .bloquear                ; teclas (Backspace, flechas, Delete...)
cmp [wmsg],WM_KEYUP
je .bloquear
cmp [wmsg],WM_PASTE
je .bloquear                ; Ctrl+V
cmp [wmsg],WM_CUT
je .bloquear                ; Ctrl+X
cmp [wmsg],WM_CLEAR
je .bloquear                ; Delete

; todo lo demás va al WndProc original
invoke CallWindowProc,[hEditOpOldProc],[hwnd],[wmsg],[wparam],[lparam]
ret

.bloquear:
xor eax,eax                 ; devolver 0 = "mensaje procesado"
ret
```

**Resultado**: el edit **no acepta entrada por teclado de ningún tipo**. Solo cambia por los botones.

---

## 5) `operadores.inc` — lógica matemática

### Enfoque: enteros escalados ×100

Todos los números se guardan internamente como **enteros ×100** para evitar punto flotante:

| Lo que tipea el usuario | Interno |
|---|---|
| `5` | `500` |
| `3.14` | `314` |
| `0.5` | `50` |
| `5%` | `5` (= 0.05 × 100) |
| `-2.75` | `-275` |

**Por qué**: `int32` es más rápido y predecible que la FPU para un proyecto escolar, y da exactitud con 2 decimales.

**Limitación**: máximo `21.474.836,47` (porque `21.474.836 × 100` ya está cerca del `INT32_MAX`).

### `.data` en `operadores.inc`

| Variable | Para qué |
|---|---|
| `strError` | `"Error"` (texto para mostrar cuando falla) |
| `bufferCalc` | Copia del texto del edit de entrada |
| `resultStr` | Buffer donde se arma el resultado en texto |
| `parseResult` | Último número parseado (o resultado final) |
| `numLeft` | Operando izquierdo (guardado antes de parsear el derecho) |
| `numSigno` | `0` = positivo, `1` = negativo (temporal del parser) |
| `theOp` | Carácter del operador actual (`+`, `-`, `X`, `/`) |

### `proc ResolverOperacion` — punto de entrada

Llamada desde `ProcesarComando` cuando se aprieta `=`.

1. Lee `hEditOp` a `bufferCalc`. Si está vacío → `Error`.
2. Si el buffer es solo `-` → `Error`.
3. Llama a `EvaluarExpresion`. Si retorna `CF=1` → `Error`.
4. Si no, formatea `parseResult` con `Int32ToStr` y escribe el string en `hEditRes`.

### `proc EvaluarExpresion` — el parser

Gracias a que solo hay **un operador binario**, el parser es simple:

```
1. Parsear número izquierdo → numLeft
2. Leer operador (si no hay, mostrar el número tal cual)
3. Parsear número derecho → parseResult
4. Verificar fin de buffer
5. Aplicar operación
```

#### Parseo del número izquierdo

```asm
call ParseNumero
jc .err
mov eax,[parseResult]
mov [numLeft],eax
```

#### Detección de operador

```asm
mov al,[esi]
cmp al,0
je .solo_numero     ; solo un número, no hay operación
; si es + - X x * / → .op_ok
; si no → .err
```

#### Parseo del número derecho + cómputo

Después de parsear el derecho, verifica que **no quede nada** en el buffer:

```asm
cmp byte [esi],0
jne .err                ; si queda algo (ej: 5+3+2), error
```

Luego aplica la operación según `theOp`.

### Cómputo de operaciones

#### Suma / resta

```asm
.op_add:
    add eax,ebx
    jo .err             ; si overflow, error
    jmp .done
```

Como ambos operandos están ×100, la suma/resta da el resultado ×100 directo.

#### Multiplicación

```asm
.op_mul:
    imul ebx            ; edx:eax = a*b (×100 × ×100 = ×10000)
    ; check: |edx| > 48 => /100 no entra en int32
    cmp edx,48
    jg .err
    cmp edx,-48
    jl .err
    mov ebx,100
    idiv ebx            ; eax = (a*b) / 100  → resultado ×100 ✓
    jmp .done
```

**Por qué el check**: `idiv` **crashea el proceso** si el cociente no cabe en `int32`. No podemos confiar solo en `jo` (que es para `imul`, no para `idiv`). El check `|edx| <= 48` garantiza que `edx:eax / 100` cabe.

#### División

```asm
.op_div:
    cmp ebx,0
    je .err             ; división por cero

    ; check: |eax| <= 21474836 * |ebx|
    ; si no se cumple, eax*100/ebx no entra en int32 => Error
    ...check con mul...
.div_ok:
    mov ecx,100
    imul ecx            ; edx:eax = a*100
    idiv ebx            ; eax = a*100 / b
    jmp .done
```

**Por qué el check**: igual que en `mul`, `idiv` crashea si el cociente no entra. El check es **matemáticamente correcto**: `|eax*100/ebx| ≤ INT32_MAX` ⟺ `|eax| * 100 ≤ INT32_MAX * |ebx|` ⟺ `|eax| ≤ (INT32_MAX/100) * |ebx|` = `21474836 * |ebx|`.

### `proc ParseNumero` — convertir texto a int32 escalado

Entrada: `esi` apunta al inicio del número. Salida: `parseResult` con el valor ×100, `esi` al final del número, `CF=1` si error.

**Pasos**:

1. **Signo `-`**: si el primer carácter es `-`, marca `numSigno=1` y avanza
2. **Parte entera**: lee dígitos y acumula en `ebx` (`ebx = ebx*10 + dígito`). Check `jo` en cada paso
3. **Parte fraccionaria** (si hay `.`):
   - Lee **solo 2 dígitos** (los extras se ignoran con `cmp ecx,2 / jae .frac_skip`)
   - Si leyó 1, multiplica por 10 (para tener 2 decimales)
   - Si leyó 0 (punto sin dígitos, ej: `5.`) → **error**
4. **Escalado**: si no hubo fracción, multiplica por 100
5. **Porcentaje**: si el siguiente es `%`, divide por 100 (o sea, `5%` → `5` internamente = 0.05 × 100)
6. **Signo**: si `numSigno=1`, niega `ebx`

**Ejemplo**: `-3.14%` → 
- signo: `-`
- entero: `3`
- fracción: `14`, ebx = `3*10+1 = 31`, `31*10+4 = 314`
- `%`: `314 / 100 = 3` (con `idiv` → 3 con resto 14, pero `idiv` toma el cociente = 3)
- signo: `-3`

### `proc Int32ToStr` — convertir int32 escalado a string

Entrada: `eax` (escalado ×100), `edi` destino.

**Pasos**:
1. Si `eax < 0`, escribe `'-'`, niega (`neg eax`)
2. Divide por 100: `eax` = parte entera, `edx` = fracción (0–99)
3. Llama `ImprimirEntero` con la parte entera
4. Si fracción ≠ 0:
   - Escribe `.`
   - Divide fracción por 10: `eax` = decena, `edx` = unidad
   - Si `edx=0` → escribe solo el primer dígito (`3.5` en vez de `3.50`)
   - Si `edx≠0` → escribe ambos dígitos (`3.14`)
5. Termina con `\0`

**Ejemplos**:
| `eax` interno | Salida |
|---|---|
| `800` | `"8"` |
| `250` | `"2.5"` |
| `564` | `"5.64"` |
| `-400` | `"-4"` |
| `5` | `"0.05"` |
| `0` | `"0"` |

### `proc ImprimirEntero` — convertir uint32 a decimal

Entrada: `eax`, `edi` destino.

**Algoritmo clásico con pila**:
1. Repetidamente divide `eax` por 10, apilando el **resto + '0'** (cada dígito se convierte en ASCII)
2. Cuando `eax=0`, desapila e imprime en orden → dígitos al revés (que es el orden correcto)

**Ejemplo**: `123`
- Iteración 1: `123/10 = 12`, resto `3` → push `'3'`
- Iteración 2: `12/10 = 1`, resto `2` → push `'2'`
- Iteración 3: `1/10 = 0`, resto `1` → push `'1'`
- Pop: `'1'`, `'2'`, `'3'` → `"123"` ✓

---

## 6) Flujo completo de una operación

Ejemplo: usuario tipea `3.14+2.5=`.

### 1. Clic en `3`
- Windows envía `WM_COMMAND` con `wparam = 1132` (`IDC_BTN_3`)
- `WndProc` → `.comando` → `ProcesarComando`
- Regla 1 (30 chars) OK. Regla 2 (11 dígitos) OK.
- No es punto, no es operador, no es `%`, es dígito.
- `.agregar` → `buffer = "3"`, `SetWindowText`
- Edit se ve: `3`

### 2. Clic en `.`
- Regla 3: es punto. Buffer no vacío. Último es `'3'` (dígito). Número actual no tiene punto.
- `.agregar` → `buffer = "3."`

### 3. Clic en `1`, `4`
- Se agregan como dígitos → `buffer = "3.14"`

### 4. Clic en `+`
- Regla 4: es operador `+`. Último es `'4'` (dígito). `ContarOperadoresBinarios` = 0. OK.
- `.agregar` → `buffer = "3.14+"`

### 5. Clic en `2`, `.`, `5`
- Igual que antes → `buffer = "3.14+2.5"`

### 6. Clic en `=` (IDC_BTN_IGUAL = 1141)
- `ProcesarComando` → `.calcular` → `ResolverOperacion`

### 7. `ResolverOperacion`
- Lee `bufferCalc = "3.14+2.5"` (no vacío, no es solo `-`)
- Llama a `EvaluarExpresion`

### 8. `EvaluarExpresion`
- `ParseNumero` con `esi` en `"3.14+2.5"`:
  - No hay `-`
  - Entero: `3` → ebx=3
  - `.` → fracción: `1`, `4` → ebx = 3*10+1 = 31, 31*10+4 = 314
  - ecx=2, listo (no multiplica por 10)
  - No hay `%`
  - No hay `-`
  - `parseResult = 314`
- `numLeft = 314`
- `esi` está en `'+'`, es operador válido → `theOp = '+'`, `esi++`
- `ParseNumero` con `esi` en `"2.5"`:
  - Entero: `2`
  - Fracción: `5` → ebx = 2*10+5 = 25, después `25*10 = 250`
  - `parseResult = 250`
- `esi` está en `\0` → OK
- `theOp = '+'` → `.op_add`: `314 + 250 = 564`
- `parseResult = 564`
- Retorna con `CF=0`

### 9. Formateo
- `Int32ToStr` con `eax=564`:
  - Positivo, no lleva `-`
  - `564 / 100 = 5` (entero), resto `64` (fracción)
  - `ImprimirEntero(5)` → `"5"`
  - Fracción ≠ 0 → `"."`, `64 / 10 = 6` (decena), resto `4` (unidad)
  - `edx ≠ 0` → escribe `'6'` y `'4'`
  - `resultStr = "5.64"`

### 10. Mostrar
- `SetWindowText [hEditRes], "5.64"`
- Usuario ve: `5.64`

---

## 7) Tabla de pruebas (para la defensa)

| Entrada + `=` | Resultado esperado |
|---|---|
| `5+3` | `8` |
| `5-3` | `2` |
| `5X3` | `15` |
| `10/4` | `2.5` |
| `1/3` | `0.33` (trunca) |
| `-1-3` | `-4` |
| `-1*-2` | `2` |
| `-1/-2` | `0.5` |
| `5*-2` | `-10` |
| `5+-3` | `2` |
| `3.14+2.5` | `5.64` |
| `3.14*2` | `6.28` |
| `5%` | `0.05` |
| `5%+3` | `3.05` |
| `50%*4` | `2` |
| `100%-25%` | `0.75` |
| `20/0` | `Error` |
| `-5` (solo) | `-5` |
| `5` (solo) | `5` |
| `-` (solo) | `Error` |
| vacío | `Error` |
| `99999X999999` | `Error` (overflow, sin crashear) |
| `9999999/0.1` | `Error` (overflow, sin crashear) |
| `21474836/0.47` | `Error` (overflow, sin crashear) |

### Validaciones de entrada

| Lo que intentás | Resultado |
|---|---|
| Tipear en el edit | Bloqueado |
| Ctrl+V / Ctrl+X | Bloqueado |
| Backspace físico | Bloqueado |
| Botón `<-` | Borra 1 carácter |
| Botón `AC` | Limpia ambos edits |
| `11.` + `5` en uno de 11 dígitos | Bloqueado (12 dígitos) |
| `1++2` | Bloqueado (dos símbolos) |
| `1+-2` | Permitido (unario) |
| `5--3` | Bloqueado (dos `-` seguidos) |
| `3.1.4` | Bloqueado (2 puntos) |
| `.5` | Bloqueado (punto al inicio) |
| `5%5` | Bloqueado (dígito después de `%`) |
| `5%%` | Bloqueado (2 `%`) |
| `+5` | Bloqueado (`+` al inicio) |
| `-5` | Permitido (negativo al inicio) |
| `5+3+2` | Bloqueado (2 binarios) |
| `5+3*2` | Bloqueado (2 binarios) |

---

## 8) Conceptos clave para la defensa

### ¿Por qué enteros escalados y no punto flotante?
- `int32` es determinista y predecible
- Sin problemas de redondeo binario
- Más rápido y más simple de debuggear
- **Trade-off**: rango limitado (~21M) y solo 2 decimales

### ¿Por qué `idiv` necesita check antes?
`idiv` **crashea el proceso** con `#DE` (divide error) si:
- El divisor es 0
- El cociente no entra en el tamaño del dividendo

`jo` (overflow flag) **no sirve para `idiv`**. Hay que chequear manualmente comparando el rango del dividendo contra el divisor.

### ¿Por qué subclaseo y no `ES_READONLY`?
- `ES_READONLY` pinta el fondo gris (feo)
- El subclaseo intercepta **solo lo que queremos** (teclado) y deja el control con apariencia normal
- Además bloquea `Ctrl+V`, que `ES_READONLY` también bloquea, pero el subclaseo es más explícito

### ¿Cómo evita crasheos?
- Toda operación aritmética riesgosa (`add`, `sub`, `imul`) tiene `jo .err` después
- Toda división (`idiv`) tiene un check **antes** que garantiza que no va a fallar
- Toda conversión texto→número tiene `jo .err` en cada `imul`/`add`
- Si algo falla, se muestra `Error` en el edit de resultado (nunca se cierra la app)

### ¿Por qué solo un operador binario?
- Simplifica el parser (no hace falta precedencia de operadores)
- El parser solo necesita `parseNumero op parseNumero`
- Si hubiera más, necesitaríamos un algoritmo de shunting-yard o parsers recursivos

---