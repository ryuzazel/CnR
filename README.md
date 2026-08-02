<div align="center">

# CnR — Compile N Run

### A mathematical, concurrent, high-performance scripting language, powered by a native C++ runtime.

<br>

![status](https://img.shields.io/badge/status-active--development-orange)
![language](https://img.shields.io/badge/runtime-C%2B%2B17-blue)
![license](https://img.shields.io/badge/license-unlicensed-lightgrey)

<br>

`Compile the file, and run — just as simple as that.`

</div>

---

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Getting Started (Build)](#getting-started-build)
- [Language Basics](#language-basics)
  - [Variables](#variables)
  - [Operators](#operators)
  - [Control Flow](#control-flow)
  - [Functions & Structs](#functions--structs)
  - [Arrays](#arrays)
  - [Error Handling](#error-handling--try--catch--throw)
- [MathLib — Symbolic & Numeric Math](#mathlib--symbolic--numeric-math)
- [Native Threading & Parallelism](#native-threading--parallelism)
- [DAG Execution System — `Nodes`](#dag-execution-system--nodes)
- [Native Database System](#native-database-system)
- [Networking / Web](#networking--web)
- [Editor Support](#editor-support)
- [Runtime in C++](#runtime-in-c)
- [Future Features](#future-features)
- [Vision](#vision)

---

## Overview

**CnR (Compile N Run)** pairs a small, readable syntax with a native C++ runtime underneath it.

- You write code in **CnR** — clean, minimal, expressive.
- The **C++ runtime** does the heavy lifting — lexing, parsing, and interpreting directly, no separate build step to manage.
- One binary, one command: `./cnr program.cnr`.

CnR is being designed with a clear focus on:

| Focus area | Status |
|---|---|
| Scientific & mathematical computing | ✅ Implemented — full symbolic/numeric MathLib bridge |
| Parallel & concurrent programming | ✅ Implemented — threads, `Parallel{}`, DAG `Nodes{}` workflows |
| Networking (HTTP client + server) | ✅ Implemented — built directly into the language |
| Databases (native, optionally encrypted) | ✅ Implemented — `.cnrdb` format with AES-256-CBC at rest |
| Bytecode / JIT execution | 🔜 Planned — see [Future Features](#future-features) |

> Every feature below links to a real, runnable example file in this repo — open it to see working code, not just a syntax sketch.

---

## Quick Start

No installation beyond building the single binary (see [Getting Started](#getting-started-build)). Save this as `fib.cnr`:

```cnr
function fib(var n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for (var i = 0; i < 10; i++) {
    print(fib(i));
}
```

Then run it:

```bash
./cnr fib.cnr
```

```
0
1
1
2
3
5
8
13
21
34
```

For a broader tour of syntax — every operator, every loop form, `switch`, `break`/`continue`, and more — see [`examples/Basics/basics.cnr`](examples/Basics/basics.cnr), or jump straight to [Language Basics](#language-basics) below.

---

## Getting Started (Build)

```bash
Still making the cmake...
```

Everything — the interpreter and the entire MathLib bridge — is unity-built from `CnR.cpp` (which pulls in `math_bridge.inc` and the flattened `mathlib_flat/` sources). No extra include paths, no separate library to link. File extension doesn't matter to the interpreter; `.cnr` and `.CnR` are both used throughout this repo's examples.

---

## Language Basics

> A single file exercising everything in this section end-to-end lives at [`examples/Basics/basics.cnr`](examples/Basics/basics.cnr) — variables, every operator, and every control-flow construct, each with the expected printed output alongside it.

### Variables

CnR's native numeric type is a **double-precision number** (`var`). Layered on top of that:

- `string`, `char` (via cast — see below), booleans (`true`/`false`)
- Dynamic arrays (`var[]`), matrices (`var[][]`), and tensors (`var[][][]`, and so on for higher rank — same `var` keyword, just more bracket pairs)
- `BigInt` and `BigFloat` — arbitrary-precision integers and decimals (up to 1000 significant digits), reached via casts: `(BigInt)x`, `(BigFloat)x`, or `(BigFloat(N))x` for a specific precision
- Structs and struct-backed objects

There is currently no `'x'` char literal — chars are produced by casting a numeric code: `var c = (char)65;`.

```cnr
var n = 42;
var s = "hello";
var c = (char)65;
var flag = true;
var arr[] = {1, 2, 3};
```

> See `examples/Basics/basics.cnr` for all of the above in one place, `examples/Variables/numeric_types.cnr` for the numeric type set, `examples/Variables/BigFloat.cnr` for arbitrary-precision arithmetic, and `examples/Variables/Matrix_Tensor.cnr` for how `var[][]`/`var[][][]` declarations work.

### Operators

| Category | Operators |
|---|---|
| Arithmetic | `+` `-` `*` `/` `%`, unary `-` |
| Assignment | `=` |
| Compound assignment | `+=` `-=` `*=` `/=` |
| Increment / decrement | `++` `--` |
| Comparison | `==` `!=` `<` `<=` `>` `>=` |
| Logical | `&&` `\|\|` `!` |

`+` also concatenates strings (`"foo" + "bar"` → `"foobar"`), and `==`/`!=` compare strings by value. Compound-assignment and increment/decrement work on plain variables and on array elements (`arr[i] += 1;`, `arr[i]++;`).

```cnr
var x = 10;
x += 5;   // 15
x++;      // 16
print(x == 16 && x != 0);   // true
```

> See `examples/Basics/basics.cnr` for every operator exercised with its expected output.

### Control Flow

- `if` / `else`
- `while` loops
- `for` loops (C-style), with increments written any of three equivalent ways: `i = i + 1`, `i += 1`, or `i++`
- `switch` / `case` / `default` — the first matching case runs (no fallthrough); `break;` inside a case exits the switch early without affecting an enclosing loop
- `break` and `continue`, valid inside `while`, `for`, and `switch` — using either outside of one is a parse-time error

```cnr
for (var i = 0; i < 10; i++) {
    if (i == 3) { continue; }
    if (i == 7) { break; }
    switch (i % 2) {
        case 0 { print("even"); }
        default { print("odd"); }
    }
}
```

> See `examples/Basics/basics.cnr` for `if`/`else`, `while`, all three `for` increment styles, `break`, `continue`, nested loops, and `switch` (including the case/loop-break distinction) each demonstrated with the exact printed output.

### Functions & Structs

- Function declaration & invocation (`function`)
- Return values (`return`)
- `Struct` declarations with a constructor (a method sharing the struct's name) and fields

> See `examples/Examples/linearRegression.cnr` for structs, functions, arrays, and loops combined into a gradient-descent linear regression model, and `examples/Examples/Xor_nn.cnr` for matrix-based math in a small neural net.

**Planned improvements:** more advanced function systems, better composition, and additional language abstractions.

### Arrays

Dynamic, numeric arrays with built-in methods:

- `push`, `pop`
- `sort`, `reverse`
- `contains`, `indexOf`
- `accumulate`
- `len(...)`

> See `examples/Variables/Array.cnr` for declaration and method usage.

### Error Handling — `try` / `catch` / `Throw`

- `try { ... } catch(var e) { ... }` — catches thrown errors **and** ordinary runtime errors (division by zero, out-of-bounds access, etc.), binding the message to `e` as a string.
- `Throw("message");` — raises your own catchable error.

> See `examples/ErrorHandling/TryCatch.cnr` for division-by-zero, out-of-bounds, and custom `Throw()` examples.

---

## MathLib — Symbolic & Numeric Math

CnR ships with a full symbolic/numeric math bridge (`math_bridge.inc` on top of a standalone C++ `mathlib`), exposed as built-in functions. This is implemented and working today, not a future goal.

**Defining and inspecting functions:**

```cnr
mathDefine("f(x) = sqrt(x - 2)");
print(mathToString("f"));   // f(x) = sqrt(x - 2)
print(mathEval("f", 6));    // 2
print(mathDomain("f"));     // [2, +inf)
```

**Calculus** — `mathDerivative`, `mathIntegral` (symbolic, throws a clear error if no closed form exists), `mathDefiniteIntegral` (numeric; switches to exact BigDecimal integration automatically when given `BigFloat` bounds and a polynomial/rational integrand), `mathTaylorSeries`, `mathLimit`, `mathLimitAtInfinity`.

**Roots, sums, products, bijectivity:** `mathRoots`/`mathRootsImag` (either raw coefficients, `mathRoots(1, -6, 11, -6)`, or a `mathDefine`'d function by name, `mathRoots("cubic")`), `mathSum`, `mathProduct`, `mathIsBijective`, `mathInverse`, `mathNumericInverseAt`.

**Number theory:** `isPrime`, `divisors` — both exact on arbitrary-precision `BigInt` input, not just machine integers.

**Linear algebra**, operating directly on `var[][]` matrix values: `matDeterminant`, `matRank`, `matInverse`, `matSolve`, `matEigenvalues`/`matEigenvaluesImag`, `matTranspose`, `matMultiply`.

**Constants:** `pi`, `euler` (`e`), `phi`, `psi` (golden ratio and its conjugate) are preloaded as `BigFloat` globals at 22-digit default precision.

> See `examples/Mathlib/mathlibExamples.cnr` for a full tour of every one of these builtins, including BigFloat-precision edge cases and error handling.

---

## Native Threading & Parallelism

- Spawn threads directly with `thread(...)`, collect results with `join()` / `joinAll()`
- Run multiple blocks concurrently with `Parallel { } { } ...`

> See `examples/Threadding/Thread.cnr` for `thread()`/`join()`, and `examples/Threadding/Parallel.cnr` for `Parallel{}` blocks.

**Planned:** locks, mutable/shared state primitives, full thread lifecycle management, runtime-controlled scheduling.

---

## DAG Execution System — `Nodes`

A Directed Acyclic Graph workflow system expressed directly in the language via `Nodes { }`.

- Declare named nodes with dependencies: `Step2(Step1) { ... }`
- Automatic execution ordering from the dependency graph
- Nodes with satisfied dependencies at the same "depth" run **in parallel**, as threads
- Retry policies per node: `Step1() -> OnFail(Retry = N) { ... }`
- Explicit node failure via `Fail();`
- Dependents of a failed node are automatically skipped, without aborting the rest of the workflow
- Cycle detection with clear error reporting

> See `examples/Threadding/Nodes.cnr` for retries, dependency chains, and failure/skip propagation.

**Planned:** richer workflow tracking/observability, passing results between dependent nodes.

---

## Native Database System

A custom `.cnrdb` table format, with database syntax built directly into the language:

```cnr
Data LoginDB("logindb") {
    Struct Account {
        var id;
        var username[];
        var password[];
        ...
        Account(var i, var u, var p) { id = i; username = u; password = p; }
    }
}
```

- Tables, records, and disk persistence, with methods like `.insert`, `.find`, `.findWhere`, `.findById`, `.updateWhere`, `.updateById`, `.delete`, `.deleteWhere`, `.deleteById`, `.orderBy`, `.count`, `.save`, `.load`
- **Encryption at rest**: `.encode(key)` enables real AES-256-CBC encryption (SHA-256-derived key) for the backing `.cnrdb` file — this is implemented today, via a minimal self-contained AES/SHA implementation with no external crypto dependency.

> See `examples/Database/db_write.cnr` and `examples/Database/db_read.cnr` for a full login-system example exercising every table method.

---

## Networking / Web

- Raw socket support
- HTTP client: `Http.GET/POST/PUT/PATCH/DELETE(url) { header { ... } body { ... } }`
- HTTP server: `Server() { host = ...; port = ...; }`, route declarations (`server.GET("/path") { ... }`), and `server.start()`
- `request` / `response` objects inside route handlers — `request.params`, `.query`, `.header`, `.cookie`, `.body`, `.json`, and `response.status`, `.header()`, `.cookie()`, `.redirect()`

> See `examples/Http/HttpClient.cnr` for outbound requests, and `examples/Http/Http.cnr` for a running HTTP server with routes.

**Planned:** HTML/template integration, native backend tooling, a fully integrated web stack.

---

## Editor Support

A VS Code extension (`cnr-extesion/cnr/`) provides syntax highlighting for `.cnr`/`.CnR` files via a TextMate grammar, kept in sync with the actual lexer/parser (so keywords, casts, operators, and MathLib builtins highlighted match what the interpreter really accepts).

---

## Runtime in C++

CnR is powered by a single-translation-unit C++ runtime handling:

- Lexing, parsing, and tree-walking interpretation
- Performance-critical operations (BigInt/BigFloat arithmetic, matrix/eigenvalue routines, AES/SHA)
- Native integrations (raw sockets, threads, file I/O)

The goal is to keep CnR's surface syntax simple while getting C++ performance underneath, without a separate compile-and-link step for the person writing CnR code.

---

## Future Features

Planned, not yet implemented, grouped by area:

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

### Native Neural Network System
- Tensor operations & matrix calculations
- Automatic differentiation
- Backpropagation & gradient computation
- Optimization algorithms
- Neural network models defined directly in native CnR syntax

### DAG Execution System
- Richer workflow tracking & observability
- Passing results between dependent nodes

### MathLib
- Further symbolic simplification and a richer rule engine for function composition/algebra
- Exact BigDecimal evaluation for transcendental expressions (currently BigFloat-precision calculus is exact only for polynomial/rational bodies; `sin`/`cos`/`exp`/`sqrt` fall back to ~19-digit `long double` precision)

---

## Vision

CnR aims to become a language where:

- Mathematics is native — and, for polynomial/rational calculus, exact to arbitrary precision.
- Parallelism is simplified.
- Networking is built-in.
- Databases are part of the language, with encryption at rest available out of the box.
- Performance comes from the C++ runtime, with no build step of your own to maintain.

<div align="center">

**CnR — Compile N Run**

</div>