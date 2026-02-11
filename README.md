# Tiny BASIC Interpreter

A simple interpreter for a subset of the BASIC programming language, implemented in C. It supports integer arithmetic, variables, arrays, and basic control flow.

## Features

- **Variables**: Single-letter variables `A` through `Z` (integer only).
- **Arrays**: Single-letter arrays `A` through `Z`, declared using the `DIM` statement. Supports both `()` and `[]` for indexing.
- **Arithmetic**: Support for `+`, `-`, `*`, and `/`.
- **Control Flow**: `GOTO` for unconditional jumps and `IF` for conditional jumps.
- **Direct & Program Mode**: Execute statements immediately or enter them as part of a numbered program.

## Commands

- `NEW`: Clears the current program from memory.
- `LIST`: Displays the current program.
- `RUN`: Executes the current program.
- `LOAD <filename>`: Loads a program from a file.
- `SAVE <filename>`: Saves the current program to a file.
- `QUIT`: Exits the interpreter.

## Statements

- `PRINT <expression | string> [, ...]`: Prints values or strings.
- `LET <variable> = <expression>`: Assigns a value to a variable or array element (e.g., `LET A(1) = 10` or `LET A[1] = 10`).
- `DIM <array>(<size>)`: Declares an array of the specified size (supports `()` or `[]`).
- `GOTO <line_number>`: Jumps to the specified line number.
- `IF <expression> <operator> <expression> [THEN] <statement>`: Executes a statement if the condition is true. Supported operators: `=`, `<`, `>`, `<=`, `>=`, `<>`, `!=`.
- `END`: Terminates program execution.

## Building and Running

### Prerequisites

- A C compiler (e.g., `gcc`, `clang`).

### Compilation

To compile the interpreter, run:

```bash
gcc basic_interpreter.c -o basic_interpreter (or you can just call the output basic)
```

### Running

To start the interpreter:

```bash
./basic_interpreter
```

## Example Program

The following program calculates and prints the squares of numbers 0 through 4:

```basic
10 LET A = 0
20 DIM B[5]
30 LET B[A] = A * A
40 PRINT "SQUARE OF ", A, " IS ", B[A]
50 LET A = A + 1
60 IF A < 5 THEN GOTO 30
70 END
```

To run this in the interpreter:
1. Start `./basic_interpreter`.
2. Type each line and press Enter.
3. Type `RUN` and press Enter.
