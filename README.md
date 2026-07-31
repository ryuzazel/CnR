<div align="center">

# CnR — Compile N Run

### A mathematical, concurrent, and high-performance programming language, powered by a native C++ runtime.

<br>

`Compile the file, and run, just as simples as that!`
</div>

---

## Overview

**CnR (Compile N Run)** is a custom programming language built to pair a small, readable syntax with the raw performance of a native C++ runtime underneath it.

The core idea is simple:

- You write code in **CnR** — clean, minimal, expressive.
- The **C++ runtime** does the heavy lifting — parsing, executing, and optimizing under the hood.
- CnR handles the plumbing in between, so you get a simple programming experience without giving up performance.

CnR is being designed with a clear focus on:

- Scientific & mathematical computing
- Artificial Intelligence
- Parallel & concurrent programming
- Networking
- Databases
- Workflow / DAG execution

> Want to see it in action? This repo includes real, runnable `.cnr` example files for every feature below — check the file mentioned in each section to see working code.

---

## Current Features

### Variables

CnR currently uses **double-precision numbers as its native numeric type**, along with strings, booleans, arrays, structs, and objects built on top of them. The language is designed around numerical computing, calculation, and data processing first.

> See `Hello.cnr` for a minimal first program.
> See `numeric_types.cnr` for all numerical variables.
> See `BigFloat.cnr` for BigFloat usage.

### Control Flow

CnR supports the structures you'd expect from any general-purpose language:

- `if` / `else`
- `while` loops
- `for` loops
- Standard comparison & logical operators (`==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`)

### Functions & Structs

CnR currently supports:

- Function declaration & invocation (`function`)
- Return values (`return`)
- Struct declarations with constructors and fields (`Struct`)

> See `linearRegression.cnr` for a full example combining structs, functions, arrays, and loops — a linear regression model trained with gradient descent, entirely in CnR.

> See `Xor_nn.cnr` for a full example of how matrices works and basic math.

**Planned improvements:** more advanced function systems, better function composition, and additional language abstractions.

### Native Database System

CnR currently supports:

- A custom `.cnrdb` database format
- Database syntax built directly into the language
- Tables, records, and persistence

### Arrays

CnR currently supports dynamic, numeric arrays with built-in methods:

- `push`, `pop`
- `sort`, `reverse`
- `contains`, `indexOf`
- `accumulate`
- `len(...)`

> See `Array.cnr` for array declaration and method usage.

**Planned improvements:** numerical transformations and more optimized array processing.

### Error Handling — `try` / `catch` / `Throw`

CnR supports structured exception handling:

- `try { ... } catch(var e) { ... }` — catches thrown errors **and** ordinary runtime errors (division by zero, out-of-bounds access, etc.), binding the message to `e` as a string.
- `Throw("message");` — raises your own catchable error.

> See `TryCatch.cnr` for division-by-zero, out-of-bounds, and custom `Throw()` examples.

---

## Native Threading & Parallelism

CnR currently supports:

- Spawning threads directly from CnR with `thread(...)`, and collecting results with `join()` / `joinAll()`
- Running multiple blocks concurrently with `Parallel { } { } ...`

> See `Thread.cnr` for `thread()`/`join()` examples, and `Parallel.cnr` for `Parallel{}` blocks.

**Planned capabilities:** locks, mutable/shared state primitives, thread lifecycle management, and runtime-controlled scheduling — abstracting complex C++ concurrency while keeping high-performance parallel code accessible.

---

## DAG Execution System — `Nodes`

CnR includes a **Directed Acyclic Graph workflow system** for orchestrating dependent tasks, expressed directly in the language via the `Nodes { }` statement.

**Currently supported:**

- Declaring named nodes with dependencies: `Step2(Step1) { ... }`
- Automatic execution ordering based on the dependency graph
- Nodes with satisfied dependencies at the same "depth" run **in parallel**, as threads
- Retry policies per node: `OnFail(Retry = N)`
- Explicit node failure via `Fail();`
- Automatic skipping of nodes whose dependencies failed, without aborting the rest of the workflow
- Cycle detection with clear error reporting

**Example execution concept:**

```
Node A runs
Node A completes successfully
Node B becomes available
Node B finishes
Nodes C and D execute in parallel
Node E runs after its dependencies complete
```

This enables:

- Parallel workflows
- Scientific pipelines
- AI processing chains
- Distributed execution possibilities

> See `Nodes.cnr` for retries, dependency chains, and failure/skip propagation.

---

## Networking / Web

CnR currently supports:

- Raw socket support
- An HTTP client: `Http.GET/POST/PUT/PATCH/DELETE(url) { header { ... } body { ... } }`
- A full HTTP server: `Server() { host = ...; port = ...; }`, route declarations (`server.GET("/path") { ... }`), and `server.start()`
- Rich `request` / `response` objects inside route handlers — `request.params`, `.query`, `.header`, `.cookie`, `.body`, `.json`, and `response.status`, `.header()`, `.cookie()`, `.redirect()`

> See `HttpClient.cnr` for making outbound HTTP requests, and `Http.cnr` for running an HTTP server with routes.

**Planned web ecosystem:** HTML/template integration, native backend development tooling, and an integrated web stack.

---

## Runtime in C++

CnR is powered by a C++ runtime responsible for actually executing the language. The runtime handles:

- Internal execution (lexing, parsing, interpreting)
- Performance-critical operations
- Native integrations
- System communication (sockets, threads, I/O)

The objective is to keep CnR's syntax simple on the surface while relying on C++ performance underneath.

---

## Future Features

The sections below are **planned, not yet implemented** — grouped by the same categories as the current features above, so it's easy to see where each area is headed.

### Variables & Language Core
- Bytecode generation
- JIT compilation
- Native optimization & an improved execution engine

### Native Threading
- Locks, mutable & shared state primitives
- Full thread lifecycle management
- Runtime-controlled scheduling

### Networking / Web
- HTML/template integration
- Native backend development tooling
- A fully integrated web stack


### MathLib Integration

The existing C++ MathLib will be integrated directly into CnR, planned to include:

| Area | Planned Capabilities |
|---|---|
| **Algebra** | Algebraic operations, expression manipulation, polynomial operations |
| **Linear Algebra** | Vectors, matrices, tensors |
| **Calculus** | Derivatives, integrals, limits |
| **Statistics** | Statistical functions, numerical analysis |
| **Optimization** | Optimization algorithms, numerical solvers |
| **Polynomial Mathematics** | Polynomial manipulation, root calculation, Durand-Kerner method, advanced numerical methods |
| **Symbolic Mathematics** | Symbolic expressions and transformations, built on the parser's AST |

### Native Neural Network System
- Tensor operations & matrix calculations
- Automatic differentiation
- Backpropagation & gradient computation
- Optimization algorithms
- Neural network models defined directly in native CnR syntax

### DAG Execution System
- Richer workflow tracking & observability
- Passing results between dependent nodes

---

## Vision

CnR aims to become a programming language where:

- Mathematics is native.
- AI development is integrated.
- Parallelism is simplified.
- Networking is built-in.
- Databases are part of the language.
- Performance comes from the C++ runtime.

<div align="center">

**CnR — Compile N Run**

</div>