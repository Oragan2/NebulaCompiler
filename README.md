# NebulaCompiler
A C/C++ inspired language designed for kernel and low level development. 

## Target
It's currently made to target x86_64 architecture and maybe plans to support more.

## Status
Currently the language is still in the work.

## Usage
nbu <file.ne>

## Exemple files
There is an exemple file inside the source root that plans to showcase as much features as possible. 

## Features
- Strongly typed with kernel-specific types (vaddr, paddr)
- Inline assembly via asm { } blocks
- Direct memory read/write primitives
- Type safety and promotion

## Building
cmake . && make
