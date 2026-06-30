# NebulaCompiler

NebulaCompiler is a systems programming language inspired by C and C++, designed for operating systems, kernels, and other low-level software where direct hardware access and predictable behavior are important.

## Current Status

NebulaCompiler is under active development.

The frontend (lexer, parser, semantic analysis, and type checking) is largely implemented. The backend is currently being expanded to generate complete x86-64 assembly.

The language is **not yet production ready** and the syntax and features may change.

## Current Features

- Strong static type system
- Integer and floating-point arithmetic
- Functions
- Global and local variables
- User-defined structures
- Enumerations
- Nested structures
- Type promotion
- Inline assembly (`asm {}`)
- Direct memory read/write primitives
- Kernel-oriented address types (`vaddr`, `paddr`)

## Target Architecture

Current target:

- x86-64 (NASM)

Additional architectures may be supported in the future.

## Building

```bash
cmake .
make
```

## Usage

```bash
nbu program.ne
```

which generates NASM assembly that can be assembled and linked.

## Example

```ne
enum State {
    READY,
    RUNNING,
    FINISHED
}

struct Process {
    uint32 pid,
    State state
}

uint32 nextPid = 0;

void createProcess() {
    Process p;
    p.pid = nextPid;
    nextPid = nextPid + 1;
    p.state = State::READY;
}
```

## Project Goals

NebulaCompiler aims to provide a language suitable for:

- Operating system kernels
- Embedded software
- Hardware drivers
- Bare-metal applications

while remaining familiar to developers with experience in C and C++.

## Roadmap

- [x] Lexer
- [x] Parser
- [x] Abstract Syntax Tree
- [x] Semantic analysis
- [x] Type checking
- [x] Structs
- [x] Enums
- [x] Function declarations
- [x] Inline assembly
- [ ] Complete x86-64 code generation
- [ ] Stack frame generation
- [ ] Function parameters
- [ ] Control-flow code generation
- [ ] Optimizations
