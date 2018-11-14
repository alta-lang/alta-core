# Changelog
All the changes for Alta's frontend/core functionality (parser, lexer, AST, DET, and module system) will be kept in this file.

This project follows [semantic versioning](https://semver.org).

## [Unreleased]
Nothing yet.

## [0.2.0] - 2018-11-13
### Added
#### Waterwheel (lexer)
  * New tokens:
    * Addition and subtraction signs (`+` and `-`)
    * Asterisks and forward slashes (`*` and `/`)
#### Palo (parser)
  * `Expectation`s can now be coerced to `bool`s
    * This allows easy validition (i.e. `if (someExpectation)` instead of `if (someExpectation.valid)`)
    * Also makes their use with `expect` much more natural (e.g. `if (expect(TokenType::Identifier))` instead of `if (expect(TokenType::Identifier).valid)`)
  * `expectKeyword` shortcut for keyword expectation in rules
  * New rules:
    * `ModuleOnlyStatement` = Exactly what it sounds like: statements that can only appear at the root of a module
    * `ImportStatement` = Only cherry-pick imports for now (`import foo, bar, foobar from "somewhere.alta"`)
    * `AdditionOrSubtraction` = Left-associative addition and subtraction
    * `MultiplicationOrDivision` = Left-associative multiplication and division
    * `BooleanLiteral` = `true` or `false`
#### AST
  * New nodes:
    * `BinaryOperation`
    * `BooleanLiteralNode`
    * `ImportStatement`
#### DET
  * Underlying type retrieval for boolean literals
  * Dedicated `exports` scope on module to keep track of only exports (instead of fishing for them in the normal scope)
  * `isCompatibleWith` method for types declared (*not* properly implemented yet)
    * Also, `%` operator for `Type`s is an operator for `isCompatibleWith`
  * `isExport` properties on functions and variables
#### Module System
  * `parseModule` method added to automatically read, lex, parse, and detail a module and return it
    * It's user overridable, so it can be changed to fit user needs

## [0.1.0] - 2018-11-11
### Added
#### General
  * Smart pointers galore! Nearly all raw pointers were replaced with smart pointers, thus providing easy, automatic, and effortless memory management (quite necessary due to the huge amount of objects dynamically allocated in a project like this)
#### AST
  * New node:
    * `AssignmentExpression`
#### DET
  * Certain classes now use factory-style construction (`create`) rather than straight-up constructors
    * Affected classes are:
      * `Function`
      * `Module`
    * Why? Because of smart pointers and the fact that `Scope`s contain `weak_ptr`s to their parents. Tried `enable_shared_from_this`, but that's a big no-no in constructors
#### Palo (parser)
  * Assignment expression parsing has been added

## [0.0.0] - 2018-11-08
### Added
#### Waterwheel (lexer)
  * Simple character tokenization support (e.g. `.`, `:`, `!`, etc.)
  * Complex character pattern tokenization support (e.g. identifiers, integers, strings, etc.)
  * Special character support (e.g. newlines, whitespace, etc.)
  * Full list of tokens added:
    * Identifiers (e.g. `foobar`, `function`, `bobby123`, etc.)
    * Integers (e.g. `123`, `456`, `0`, etc.)
    * Strings (e.g. `"hello"`, `"it's-a me`, etc.)
    * Opening and closing braces (`{` and `}`)
    * Opening and closing parentheses (`(` and `)`)
    * Colons and semicolons (`:` and `;`)
    * Commas and dots (`,` and `.`)
    * Equal signs (`=`)
#### Palo (parser)
  * Dynamic expectation-based parsing
    * Allows for complex rules that would be ambiguous with traditional parsers
    * Simple to extend with reusable code (as evidenced by `expectModifiers` and `expectParameters`)
  * Failure cache to avoid repitition and dramatically improve parse times and efficiency
#### AST
  * A bunch of new nodes (in alphabetical order, pretty self-explanatory):
    * `Accessor`
    * `BlockNode`
    * `ExpressionNode`
    * `ExpressionStatement`
    * `Fetch`
    * `FunctionDefinitionNode`
    * `IntegerLiteralNode`
    * `Node`
    * `Parameter`
    * `ReturnDirectiveNode`
    * `RootNode`
    * `StatementNode`
    * `Type`
    * `VariableDefinitionExpression`
#### DET
  * `Node` = Basic DET node that all DET nodes inherit from
  * `Module` = Every `RootNode` has a `Module`. Does two things:
    1. Provides a kind of namespace for all of the items it contains
    2. Facilitates importing and exporting module items
  * `Scope` = Contains `ScopeItem`s and provides an interface for finding or filtering items
  * `ScopeItem` = Basic DET node that all scope-bound DET nodes inherit from
  * `Function` = Scope item that describes a function. Complete with name, parameters, and return type
  * `Variable` = Scope item that describes a variable. Has a name and type
  * `Type` = Describes an Alta type accurately while being user-friendly
#### Module System
  * Module resolution support
  * Package information resolution and parsing support
    * So far, package information only contains a name, a version, the package's root directory, and the package's main source file
#### [Internal] Filesystem
  * Default platform separator determination (`\` on Windows, `/` on Unix-like platforms like Linux and macOS)
  * Current working directory retrieval
  * Path abstraction
    * Support for both rooted and floating (rootless) paths
    * Support for any path separator (not just `\` and `/`). Even multiple separators for a single path may given (useful when you don't know if paths are using Windows or Unix separators)
    * Absolute path resolution support (by default, resolves from the current working directory)
    * Relative path resolution support
    * Stringification support
    * `basename` support (e.g. `bazz.txt` in `/foo/bar/bazz.txt`)
    * `filename` support (e.g. `bazz` in `/foo/bar/bazz.txt`)
    * `extname` support (e.g. `.txt` in `/foo/bar/bazz.txt`)
    * `dirname` support (e.g. `/foo/bar` in `/foo/bar/bazz.txt`)
    * Many useful little methods (`isValid`, `isEmpty`, `isRoot`, `hasComponents`, `exists`, `isDirectory`, `isAbsolute`)
    * Operator support
  * Recursive directory creation (`mkdirp`, similar to the Unix command `mkdir -p`)
