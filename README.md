# CnR - Compile N Run

<p align="center">
  <b>A mathematical, concurrent and high-performance programming language powered by a C++ runtime.</b>
</p>

---

# Overview

**CnR (Compile N Run)** is a custom programming language designed to combine simple syntax with the performance and capabilities of a native C++ runtime.

The core idea:

- The user writes code in CnR.
- The C++ runtime executes optimized operations internally.
- CnR manages communication between systems, allowing a simpler programming experience while maintaining high performance.

CnR is focused on:

- Scientific computing
- Mathematics
- Artificial Intelligence
- Parallel programming
- Networking
- Databases
- Workflow execution

---

# Current Features

## Variables

CnR currently uses **only double variables** as its native variable type.

The language is designed around numerical computing, calculations and data processing.

---

## Control Flow

CnR currently supports programming logic structures:

- `if`
- `else`
- loops
- conditional execution
- iteration structures

---

## Functions

CnR currently supports:

NOTHING

Future improvements:
- Function declaration
- Function execution
- Return values
- More advanced function systems
- Better function composition
- Additional language abstractions

---

## Arrays

CnR currently supports arrays.

Planned improvements:

- Dynamic array operations
- `push_back`
- Accumulation functions
- Numerical transformations
- More optimized array processing

---

# Runtime in C++

CnR uses a C++ runtime responsible for executing the language.

The runtime handles:

- Internal execution
- Performance-critical operations
- Native integrations
- System communication

The objective is to keep the CnR syntax simple while using C++ performance underneath.

---

# Future Features

---

# Native Threading

CnR will support native multithreading.

Planned capabilities:

- Creating threads directly from CnR
- Thread lifecycle management
- Parallel execution
- Runtime-controlled scheduling

The goal is to abstract complex C++ concurrency while allowing high-performance parallel applications.

---

# Networking / Web

Future native networking system:

- Socket support
- HTTP requests
- HTTP servers
- Web communication

Future web ecosystem:

- HTML/template integration
- Native backend development
- Integrated web stack

---

# Native Database System

CnR will include a native database engine.

Planned features:

- Custom `.cnrdb` database format
- Database syntax inside the language
- Tables
- Records
- Persistence
- C++ database engine integration

The goal is to provide database functionality without requiring external systems for common use cases.

---

# MathLib Integration

The existing C++ MathLib will be integrated directly into CnR.

Planned available mathematical capabilities:

## Algebra

- Algebraic operations
- Expression manipulation
- Polynomial operations

## Linear Algebra

- Vectors
- Matrices
- Tensors

## Calculus

- Derivatives
- Integrals
- Limits

## Statistics

- Statistical functions
- Numerical analysis

## Optimization

- Optimization algorithms
- Numerical solvers

## Polynomial Mathematics

- Polynomial manipulation
- Root calculation
- Advanced numerical methods
- Durand-Kerner method

## Symbolic Mathematics

Using the parser AST:

- Symbolic expressions
- Mathematical transformations
- Expression analysis

---

# Native Neural Network System

CnR will support native neural network development.

Planned capabilities:

- Tensor operations
- Matrix calculations
- Automatic differentiation
- Backpropagation
- Gradient computation
- Optimization algorithms
- Neural network models defined directly in CnR

The objective is to allow AI systems to be created using native CnR syntax while the runtime executes optimized C++ operations.

---

# DAG Execution System

CnR will include a Directed Acyclic Graph execution system for task orchestration.

The system will allow programs to represent execution dependencies between nodes.

Features:

- Node states
- Dependency management
- Automatic execution ordering
- Parallel node execution
- Workflow tracking

Example execution concept:

- Node A runs
- Node A completes successfully
- Node B becomes available
- Node B finishes
- Nodes C and D execute in parallel
- Node E runs after dependencies complete

This enables:

- Parallel workflows
- Scientific pipelines
- AI processing chains
- Distributed execution possibilities

---

# Future Compiler Improvements

Planned:

- Bytecode generation
- JIT compilation
- Native optimization
- Improved execution engine

---

# Vision

CnR aims to become a programming language where:

- Mathematics is native.
- AI development is integrated.
- Parallelism is simplified.
- Networking is built-in.
- Databases are part of the language.
- Performance comes from the C++ runtime.

**CnR - Compile N Run**

