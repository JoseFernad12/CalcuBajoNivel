# FASM calculator — iteration 2

Open `calculadora.rap` in RadASM and use **Make > Assemble**, then **Make > Run**.
Alternatively, close any running `calculadora.exe` and run `compilar.bat`.
The build uses the existing FASM installation at `C:\Pack RadASM\fasm`.

## Behavior

- Click buttons to enter `a op b`, with exactly one binary operator: `+`, `-`,
  `X` or `/`. Each operand may be negative and may contain one decimal point.
- Examples: `3.14+2` gives `5.14`; `5--2` gives `7`; `5/-2` gives `-2.5`.
- Results update after each accepted edit. Empty input, a lone first operand,
  `7+`, `7+-` and `7+0.` are incomplete and leave the result empty.
- Results use at most two decimal places, rounding halves away from zero and
  omitting trailing zeros. `1/8` gives `0.13`; small rounded negatives show `0`,
  not `-0`.
- `=` copies the displayed rounded result into the input. For example,
  `2+3`, `=`, `X4` gives `20`. No hidden precision is carried across `=`.
- Another binary operator after a completed expression is ignored; press `=`
  to use that result in a new operation. A pending operator can be replaced;
  a following minus instead starts a negative second operand.
- `AC` clears both fields. Backspace removes the last character and recalculates.
- The existing percent button divides its operand by 100. Thus `50%X2` gives
  `1` and `2+50%` gives `2.5`; it is not a context-dependent commercial percentage.
- Keyboard editing and pasting remain blocked. Comma is not supported.
- The complete input is limited to 11 characters. The final position is
  reserved for a digit; automatic `0.` insertion reserves a following digit too.

## Errors

`Error: /0` reports division by zero, including negative zero. Incomplete
decimals take precedence: `5/0.` stays blank until another digit completes it.
`Error rango` reports results that cannot fit the 11-character display, such
as `999999999/7`. `Error expr` reports malformed expressions to the core parser;
the normal button flow prevents those expressions. `=` does nothing for errors
or incomplete expressions. Backspace and AC remain usable after an error.

## Implementation walkthrough

`validacion.inc` handles button input and the one-operator rule. `eventos.inc`
calls `EvaluarExpresion` after every accepted edit and manages result reuse.
`operadores.inc` contains the parser, x87 arithmetic and decimal formatting.
Constants and output state are in `calculo_datos.inc`.

`LeerNumero` accumulates digits with `FIMUL` and `FIADD`, divides by a power of
ten for the fractional part, and applies `FCHS` for a negative sign. Operands
are retained as 80-bit values. `FADDP`, `FSUBP`, `FMULP` and `FDIVP` perform the
four operations. Zero is checked before division. The whole x87 environment
is saved and restored, so repeated evaluation neither fills the FPU stack nor
changes the caller's rounding mode.

Formatting rounds the magnitude to cents, restores the sign, and prints the
digits using integer division. Tiny absolute and relative tolerances compensate
for x87 representation noise at decimal half-cent boundaries (for example,
`0.265` rounds to `0.27`). This is binary floating-point arithmetic, not an
arbitrary-precision decimal implementation. The tolerances are below the input
resolution within this calculator's 11-character domain.

See `pruebas/README.md` for reproducible tests and what each suite checks.
