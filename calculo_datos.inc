; x87 constants. Integer input digits are accumulated without integer overflow.
CalcDiez       dd 10
CalcCien       dd 100
CalcMedio      dq 0.5
; Compensate x87 representation noise at decimal half-cent boundaries.
; Tolerances are far below the input resolution of an 11-character expression.
CalcRoundAbs   dq 1.0e-12
CalcRoundRel   dq 1.0e-18
CalcLimite     dq 100000000000.0
CalcTruncar    dw 0F7Fh
ErrorDivision  db 'Error: /0',0
ErrorRango     db 'Error rango',0
ErrorExpresion db 'Error expr',0
ResultadoTexto rb 32
ResultadoValido dd 0
