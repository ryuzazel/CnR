# CnR Language Support

A Visual Studio Code extension that adds first-class syntax support for **CnR (Compile N Run)** — a mathematical, concurrent, high-performance scripting language backed by a native C++ runtime.

This extension provides a TextMate grammar for `.cnr`/`.CnR` files, kept in sync with the actual lexer and parser in `CnR.cpp` and `math_bridge.inc` — not a hand-guessed approximation of the language.

## Features

### Syntax Highlighting

The grammar covers the real token/keyword surface of CnR, including:

* **Control flow**: `var`, `if`, `else`, `while`, `for`, `function`, `return`, `switch`, `case`, `default`, `break`, `continue`, `try`, `catch`, `Throw`
* **Types & casts**: `char`, `string`, `Int`, `long`, `Float`, `BigInt`, `BigFloat` (and its `bigDouble` alias), plus the `(Type)expr` cast forms — including `(BigFloat(N))expr` for a specific precision
* **Arrays, matrices & tensors**: `var[]`, `var[][]`, `var[][][]`, and their methods (`push`, `pop`, `sort`, `reverse`, `contains`, `indexOf`, `accumulate`)
* **Structs**: `Struct Name { ... }` declarations
* **Concurrency**: `thread`/`join`/`joinAll`, `Parallel { }` blocks, `mutable` variables, and `mutex()`/`.lock()`/`.unlock()`/`.tryLock()`
* **DAG workflows**: `Nodes { }`, dependency chains, `OnFail(Retry = N)`, `Fail()`
* **Networking**: `Http.GET/POST/PUT/PATCH/DELETE`, `header`/`body`, `Server()`, route declarations, and the `request`/`response` objects inside route handlers
* **Native database**: `Data`/`table` declarations and table methods (`.find`, `.findWhere`, `.findById`, `.insert`, `.updateWhere`, `.updateById`, `.delete`, `.deleteWhere`, `.deleteById`, `.orderBy`, `.count`, `.save`, `.load`, `.encode`)
* **File I/O**: `readFile`, `writeFile`, `appendFile`, `fileExists`
* **MathLib** — the full symbolic/numeric math bridge:
  * Function definition & evaluation: `mathDefine` (and its short alias `define`), `mathEval`, `mathToString`
  * Calculus: `mathDerivative`, `mathGradient` (multivariable), `mathIntegral`, `mathDefiniteIntegral`, `mathTaylorSeries`, `mathLimit`, `mathLimitAtInfinity`
  * Domain & structure: `mathDomain`, `mathDomainExact`, `mathImage`, `mathIsBijective`, `mathInverse`, `mathNumericInverseAt`
  * Roots, sums & products: `mathRoots`, `mathRootsImag`, `mathSum`, `mathProduct`, `divisors`
  * Complex numbers: `complexEval("...")` — string-expression-driven complex arithmetic, with `i` as the imaginary unit
  * Linear algebra on `var[][]` matrices: `matDeterminant`, `matInverse`, `matRank`, `matSolve`, `matEigenvalues`, `matEigenvaluesImag`, `matTranspose`, `matMultiply`
  * Constants: `pi`, `euler`, `phi`, `psi`
* **Core numeric builtins**: `sqrt`/`sqroot`, `pow`, `abs`, `floor`, `ceil`, `round`, `log`, `ln`, `log10`, `exp`, `sin`, `cos`, `tan`, `arcsin`, `arccos`, `arctan`, `isPrime`, `min`, `max`, `random`, `randomSeed`
* **Literals & basics**: numbers, strings (with escapes), booleans, comments

  * Single-line comments (the only comment form the interpreter itself understands):

    ```cnr
    // This is a comment
    ```

  * Block comments are also highlighted for editor convenience, but note that `/* ... */` is **not** recognized by the CnR lexer — it's purely a highlighting nicety, not a real language feature:

    ```cnr
    /*
       Highlighted, but not actually parsed as a comment by CnR itself
    */
    ```

### Example

```cnr
function fib(var n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for (var i = 0; i < 10; i++) {
    print(fib(i));
}
```

## Requirements

No additional dependencies are required for syntax highlighting.

To write and run CnR programs, you need the CnR interpreter (`./cnr program.cnr`) built and available — see the main CnR repository for build instructions.

## Extension Settings

This extension currently does not add any custom VS Code settings.

Future versions may include:

* Compiler path configuration
* Automatic compilation
* Run commands
* Debugging support
* Language server features

## Roadmap

Planned features:

* [ ] CnR snippets
* [ ] Autocomplete support
* [ ] Error diagnostics
* [ ] CnR Language Server Protocol (LSP)
* [ ] Integrated interpreter execution (run `.cnr` files from the editor)
* [ ] Debugger support
* [ ] Formatter

## Known Issues

Known limitations:

* No autocomplete yet
* No syntax error detection
* No compiler/interpreter integration
* Syntax highlighting depends entirely on the TextMate grammar, so it can't catch things a real parser would (e.g. it highlights `/* */` as a comment even though CnR itself doesn't treat it as one)

If you find a bug or have a suggestion, please report it.

## Release Notes

### 0.0.8

Grammar rewritten against the actual lexer/parser and math bridge, adding full coverage for previously-undocumented language features:

* `switch`/`case`/`default`/`break`/`continue`
* Compound assignment and increment/decrement (`+=`, `-=`, `*=`, `/=`, `++`, `--`)
* Mutexes and mutability (`mutable`, `mutex()`, `.lock()`/`.unlock()`/`.tryLock()`)
* File I/O builtins (`readFile`, `writeFile`, `appendFile`, `fileExists`)
* Full MathLib builtin set, including `mathGradient`, the `define` alias, and `complexEval` for complex-number arithmetic
* DAG workflows (`Nodes`, `OnFail`, `Retry`, `Fail`), native database methods, and the full HTTP client/server surface

### 0.0.1

Initial release.

Added:

* CnR language recognition
* `.cnr` file support
* Syntax highlighting
* Comments support
* Strings and numbers highlighting

---

## About CnR

CnR is a language where:

* Mathematics is native — real, complex, and multivariable, with symbolic calculus and arbitrary-precision arithmetic built in
* Parallelism is simplified — threads, mutexes, `Parallel{}` blocks, and DAG-based `Nodes{}` workflows
* Networking is built in — a full HTTP client and server, no framework required
* Databases are part of the language, with optional AES-256 encryption at rest
* Performance comes from a native C++ runtime, with no build step of your own to maintain

This extension is the first step toward a complete CnR development environment inside Visual Studio Code.

**Enjoy programming with CnR!**