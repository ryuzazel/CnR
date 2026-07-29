# CnR Language Support

A Visual Studio Code extension that adds first-class support for the **CnR programming language**.

CnR (**Compile and Run**) is a lightweight programming language designed for simplicity, fast experimentation, and learning. This extension provides syntax highlighting and editor support to make writing CnR code easier and more enjoyable.

## Features

### Syntax Highlighting

The extension currently supports syntax highlighting for:

* Keywords:

  * `var`
  * `if`
  * `else`
  * `while`
  * `for`
  * `return`
  * `break`
  * `continue`
  * `function`

* Built-in functions:

  * `print`
  * `len`

* Data:

  * Numbers
  * Strings
  * Boolean values
  * Variables

* Comments:

  * Single-line comments:

    ```cnr
    // This is a comment
    ```

  * Block comments:

    ```cnr
    /*
       This is a block comment
    */
    ```

### Example

```cnr
// CnR example

var counter = 10;

while(counter > 0)
{
    print("Hello from CnR");

    counter = counter - 1;
}
```

## Requirements

No additional dependencies are required for syntax highlighting.

To write and run CnR programs, you need the CnR compiler/interpreter installed.

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
* [ ] Improved syntax highlighting
* [ ] Autocomplete support
* [ ] Error diagnostics
* [ ] CnR Language Server Protocol (LSP)
* [ ] Integrated compiler execution
* [ ] Debugger support
* [ ] Formatter

## Known Issues

This extension is currently in early development.

Known limitations:

* No autocomplete yet
* No syntax error detection
* No compiler integration
* Syntax highlighting depends on TextMate grammar

If you find a bug or have a suggestion, please report it.

## Release Notes

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

CnR is a programming language focused on:

* Simplicity
* Performance
* Easy experimentation
* Clean syntax

This extension is the first step toward a complete CnR development environment inside Visual Studio Code.

**Enjoy programming with CnR!**
