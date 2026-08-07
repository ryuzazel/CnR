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
  - [File I/O](#file-io)
  - [Error Handling](#error-handling--try--catch--throw)
- [MathLib — Symbolic & Numeric Math](#mathlib--symbolic--numeric-math)
  - [Complex Numbers](#complex-numbers)
- [Native Threading & Parallelism](#native-threading--parallelism)
  - [Mutexes / Locks](#mutexes--locks)
- [DAG Execution System — `Nodes`](#dag-execution-system--nodes)
- [Native Database System](#native-database-system)
- [Networking / Web](#networking--web)
  - [JobForge — a real async job orchestrator in CnR](#jobforge--a-real-async-job-orchestrator-in-cnr)
- [Editor Support](#editor-support)
- [Runtime in C++](#runtime-in-c)
- [Full Examples Index](#full-examples-index)
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
| Scientific & mathematical computing | ✅ Implemented — full symbolic/numeric MathLib bridge, including complex numbers |
| Parallel & concurrent programming | ✅ Implemented — threads, mutexes/locks, `Parallel{}`, DAG `Nodes{}` workflows |
| Networking (HTTP client + server) | ✅ Implemented — built directly into the language, proven with a real async job orchestrator ([JobForge](#jobforge--a-real-async-job-orchestrator-in-cnr)) |
| Databases (native, optionally encrypted) | ✅ Implemented — `.cnrdb` format with AES-256-CBC at rest |
| File I/O | ✅ Implemented — `readFile`, `writeFile`, `appendFile`, `fileExists` |
| Bytecode / JIT execution | 🔜 Planned — see [Future Features](#future-features) |

Every example referenced below is a real, runnable file in this repo — check the [Full Examples Index](#full-examples-index) for the complete list, there's more here than a typical "getting started" section covers.

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

CnR is a unity build: `CnR.cpp` `#include`s `math_bridge.inc` and the flattened `mathlib_flat/` sources, so it compiles as a single translation unit — no extra include paths, no separate library to link, no Makefile of dependencies to track.

**Manual compile:**
```bash
g++ -std=c++17 -O3 -march=native -pthread CnR.cpp -o CnR
```

**Or install `CnR` onto your `PATH` with the provided CMake build:**
```bash
./scripts/build.sh              # build + install to /usr/local/bin (sudo)
./scripts/build.sh --user       # install to ~/.local/bin (no sudo)
```

For the zero-CMake alternative, Arch `PKGBUILD`, `.deb`/`.rpm` packages, and Windows builds (MSVC and MinGW cross-compile), see [`README-build.md`](README-build.md).

File extension doesn't matter to the interpreter; `.cnr` and `.CnR` are both used throughout this repo's examples.

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

### File I/O

Built-in filesystem access, no library import required:

- `readFile(path)`, `writeFile(path, content)`, `appendFile(path, content)`
- `fileExists(path)`

> See `examples/Database/file_io.cnr` for reading, writing, appending, and existence checks in one file — useful on its own, and also as the persistence layer underneath the native database system below.

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

> See `examples/Mathlib/rootsAndMultiply.cnr` for `mathRoots`/`mathRootsImag` and `mathProduct` worked through end-to-end, from raw coefficients through to named functions.

**Number theory:** `isPrime`, `divisors` — both exact on arbitrary-precision `BigInt` input, not just machine integers.

**Linear algebra**, operating directly on `var[][]` matrix values: `matDeterminant`, `matRank`, `matInverse`, `matSolve`, `matEigenvalues`/`matEigenvaluesImag`, `matTranspose`, `matMultiply`.

**Multivariable functions:** `mathDefine` and friends extend to functions of several variables (e.g. `f(x, y) = ...`), with evaluation, domain inference, and derivatives following the same builtin surface as the single-variable case.

**Constants:** `pi`, `euler` (`e`), `phi`, `psi` (golden ratio and its conjugate) are preloaded as `BigFloat` globals at 22-digit default precision.

> See `examples/Mathlib/mathlibExamples.cnr` for a full tour of every one of these builtins, including BigFloat-precision edge cases and error handling.

### Complex Numbers

Complex numbers are a native part of MathLib, not just something that shows up in a result — you can construct and operate on them directly, alongside real and multivariable math.

> See `examples/Mathlib/multivarAndComplex.cnr` for complex-number construction and arithmetic side by side with multivariable function definitions, and `examples/Mathlib/rootsAndMultiply.cnr` for `mathRootsImag` returning genuine complex roots from a polynomial with no real solutions.

---

## Native Threading & Parallelism

- Spawn threads directly with `thread(...)`, collect results with `join()` / `joinAll()`
- Run multiple blocks concurrently with `Parallel { } { } ...`

> See `examples/Threadding/Thread.cnr` for `thread()`/`join()`, and `examples/Threadding/Parallel.cnr` for `Parallel{}` blocks.

### Mutexes / Locks

Native mutex support for guarding shared state across threads — implemented and working today.

> See `examples/Threadding/mutex.CnR` for lock/unlock usage around shared state in a multi-threaded context.

**Planned:** fuller thread lifecycle management and runtime-controlled scheduling on top of the existing thread/mutex primitives.

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

### JobForge — a real async job orchestrator in CnR

The HTTP layer isn't just a toy example — it's the backbone of **JobForge**, a full-stack async job orchestrator written entirely in CnR, combining the HTTP server, the `Nodes` DAG system, and native threading into one working application.

> See `examples/Examples/jobforge.cnr` for the orchestrator itself and `examples/Examples/jobforgeTest.cnr` for exercising it end-to-end.

**Planned:** HTML/template integration, native backend tooling, a fully integrated web stack.

---

## Editor Support

A VS Code extension (`cnr-extesion/cnr/`) provides syntax highlighting for `.cnr`/`.CnR` files via a TextMate grammar, kept in sync with the actual lexer/parser (so keywords, casts, operators, and MathLib builtins highlighted match what the interpreter really accepts).

---

## Runtime in C++

CnR is powered by a single-translation-unit C++ runtime handling:

- Lexing, parsing, and tree-walking interpretation
- Performance-critical operations (BigInt/BigFloat arithmetic, matrix/eigenvalue routines, AES/SHA)
- Native integrations (raw sockets, threads, mutexes, file I/O)

The goal is to keep CnR's surface syntax simple while getting C++ performance underneath, without a separate compile-and-link step for the person writing CnR code.

---

## Full Examples Index

Every example in the repo, grouped by folder — a good map for exploring beyond what's linked inline above:

| Folder | File | Demonstrates |
|---|---|---|
| `Basics/` | `basics.cnr` | Full language tour: variables, every operator, all control flow |
| `Variables/` | `Array.cnr` | Dynamic array methods (`push`, `pop`, `sort`, `reverse`, `contains`, `indexOf`, `accumulate`) |
| `Variables/` | `BigFloat.cnr` | Arbitrary-precision decimal arithmetic |
| `Variables/` | `Matrix_Tensor.cnr` | `var[][]` / `var[][][]` matrix and tensor declarations |
| `Variables/` | `numeric_types.cnr` | The full numeric type set (`var`, `BigInt`, `BigFloat`, casts) |
| `ErrorHandling/` | `TryCatch.cnr` | `try`/`catch`, runtime errors, custom `Throw()` |
| `Mathlib/` | `mathlibExamples.cnr` | Full MathLib tour: calculus, roots, linear algebra, constants |
| `Mathlib/` | `multivarAndComplex.cnr` | Multivariable functions and native complex-number arithmetic |
| `Mathlib/` | `rootsAndMultiply.cnr` | `mathRoots`/`mathRootsImag`, `mathProduct` |
| `Threadding/` | `Thread.cnr` | `thread()` / `join()` |
| `Threadding/` | `Parallel.cnr` | `Parallel{}` concurrent blocks |
| `Threadding/` | `mutex.CnR` | Mutex lock/unlock around shared state |
| `Threadding/` | `Nodes.cnr` | DAG workflows: dependencies, retries, failure/skip propagation |
| `Database/` | `db_write.cnr`, `db_read.cnr` | Full `.cnrdb` table system, including encryption at rest |
| `Database/` | `file_io.cnr` | `readFile`, `writeFile`, `appendFile`, `fileExists` |
| `Http/` | `HttpClient.cnr` | Outbound `Http.GET/POST/PUT/PATCH/DELETE` requests |
| `Http/` | `Http.cnr` | HTTP server with routes, `request`/`response` objects |
| `Examples/` | `linearRegression.cnr` | Structs, functions, arrays, and loops in a gradient-descent model |
| `Examples/` | `Xor_nn.cnr` | Matrix-based math in a small neural net |
| `Examples/` | `jobforge.cnr`, `jobforgeTest.cnr` | JobForge: a real async job orchestrator combining HTTP, `Nodes`, and threading |

---

## Future Features

Planned, not yet implemented, grouped by area:

### Variables & Language Core
- Bytecode generation
- JIT compilation
- Native optimization & an improved execution engine

### Native Threading
- Full thread lifecycle management on top of the existing thread/mutex primitives
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

- Mathematics is native — real, complex, and multivariable, and for polynomial/rational calculus, exact to arbitrary precision.
- Parallelism is simplified.
- Networking is built-in, proven out by real applications like JobForge.
- Databases are part of the language, with encryption at rest available out of the box.
- Performance comes from the C++ runtime, with no build step of your own to maintain.

<div align="center">

**CnR — Compile N Run**

</div>