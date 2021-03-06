# Changelog
All the changes for Alta's frontend/core functionality (parser, lexer, AST, DET, and module system) will be kept in this file.

This project follows [semantic versioning](https://semver.org).

## [Unreleased]
  * Many, MANY changes; still TBD (to-be-documented)
### Added
#### DET
  * Check parent classes for possible `from` or `to` casts
### Fixed
#### Palo (parser)
  * Fix `super` parsing in classes
    * I don't know what I was thinking when I originally limited `super` to only methods. Anyways, it can be used anywhere inside a class definition now
    * It'll still be a regular identifier anywhere else (although its usage as a regular identifier is not recommended)
#### DET
  * Don't treat `nullptr` like a regular `any` type
  * Don't allow manual casts to cast freely
    * e.g. They can't coerce a class to a pointer or a boolean
    * Note that *conversion* is not *coercion*
      * Alta types can use conversion (`from` and `to` are the main methods of conversion), but coercion is only allowed between native types and pointers
#### AST
  * Add ranged-loop iterator details in the detailing method
    * I don't know why the details were being added in the validation method in the first place
    * This way, validating a module (or really, even a single RangedForLoopStatement node) twice won't trigger errors due to the information being set twice

## [0.10.0] - 2018-12-20
### Added
#### [Project]
  * CI integration via [Azure Pipelines](https://facekapow.visualstudio.com/alta/_build?definitionId=2)
    * This gets us closer to automated testing of every commit
      * All that's left to add are tests :smile:
#### Attributes
  * Attributes now have their full ID path on them
  * The ability to clear attributes has been added
#### AST
  * AST nodes now have their own UUIDs (universally unique identifiers)
#### DET, AST
  * The `void` type is now supported
#### Palo (parser), Waterwheel (lexer), AST
  * Add support for more binary operators; Specifically:
    * Equality (`==`)
    * Inequality (`!=`)
    * Greater than (`>`)
    * Less than (`<`)
    * Greater than or equal to (`>=`)
    * Less than or equal to (`<=`)
#### Palo (parser), AST
  * Support for conditional expressions and statements
#### Palo (parser), DET, AST
  * Support for named parameters added
  * Arbitrary type (`any`) support for function declarations
    * Mainly inteded for declarations for C vararg functions (like `printf`)
  * Variable parameter support added
    * Fully supported for external function declarations (like for `printf`)
    * *Theoretically* supported in function definitions
      * But not really, since it would require at least pointer dereferencing support
      * Preferably, it would be properly implemented as a generic container class once user-types (**and** generic classes) are added
#### Module System
  * Support for package targets added
### Fixed
#### [Internal] Filesystem
  * Invalid filesystem `Path`s do not exist
### Changed
#### Palo (parser)
  * The generic parser interface has been completely changed
    * Now we use a coroutine-like interface that prevents stack overflows when parsing
    * Though it leads to much more boilerplate to write for new rules, it is a much more stable parser
#### [Internal] Filesystem
  * Many `Path` methods have been made `const` methods
    * Hopefully, this will allow compilers to perform more optimizations on `Path`s

## [0.9.0] - 2018-12-03
### Changed
  * Standard library renamed from "STL" to "stdlib"
    * Because I learned that "STL" actually stands for "standard template library"
  * Build artificats are now put into their own folders in the build directory: `bin` for executables and `lib` for libraries
### Fixed
  * macOS builds have been fixed
    * Via conditional inclusion of a polyfill for `std::optional`: [TartanLlama/optional](https://github.com/TartanLlama/optional/)
  * Linux builds have been fixed
    * By removing attempts to initialize reference variables with temporary values in `src/preprocessor.cpp`

## [0.8.0] - 2018-12-01
### Added
#### Attributes
  * Brand new core component
    * Welp. AltaCore is getting pretty big (in terms of the functionality it implements)
  * Attributes can be registered per-file or globally
  * They can have an optional callback that it suppossed to be run by the backend when compiling code
  * They can also be narrowed to only apply to certain statement types
    * By default, attributes are general, meaning they can apply to any statement or even to no statement at all
  * Attribute callbacks can also take arguments, and these are allowed to be 3 different kinds of concrete values: string, integer, or boolean literals
#### Palo (parser)
  * General attribute parsing
#### AST
  * New nodes:
    * `AttributeNode`
    * `AttributeStatement` = Statement node for general attributes that aren't applied to any other nodes
#### Util
  * Added `escape` method for escaping special characters in string literals
### Changed
#### AST
  * All literal value nodes now inherit from a common `LiteralNode` class
    * So far, this includes `StringLiteralNode`s, `IntegerLiteralNode`s, and `BooleanLiteralNode`s
#### Preprocessor
  * Implemented substitution delimiter change
    * Alta's preprocessor substition delimiters changed from `@foobar@` to `@[foobar]`

## [0.7.0] - 2018-11-28
### Added
#### DET, AST, Palo (parser)
  * String literal support
  * Function declaration support
### Fixed
#### Preprocessor
  * Fixed escaped character support in string literals
    * Before, we were accessing the wrong vector (`results` instead of `input`)

## [0.6.0] - 2018-11-27
### Added
#### DET
  * Aliases have been implemented
    * Basically, these are needed in order to implement aliased cherry-pick imports
    * They're basically item pointers (or more like references, since they will be recursively followed until a non-`Alias` is found), and they're resolved during item lookup in `Scope`s
  * Namespaces have been implemented
    * They've only been tested with alias imports, but they've been (theoretically) fully implemented and should work for custom namespace definitions, if they're ever added
#### DET, AST, Palo (parser)
  * Aliased cherry-pick import support has been added
    * Note: this is *not* the same as alias imports
    * Basically, allows you to import an item under a different name
      * e.g. `import foo as bar from "some-module.alta"` would import an item named `foo` from "some-module.alta" and make it available under the name `bar` in your module
  * Alias import support has been added
    * Allows you to import all of a module's exports as a namespace
      * e.g. `import "some-module.alta" as someModule` would import a module called "some-module.alta" and put all of its exports into a namespace named `someModule`
  * Accessors have been properly implemented
    * They work just like you'd expect them to work: `foo.bar` would try to find a member named `bar` in the scope item named `foo`
### Fixed
#### Preprocessor
  * Multi-line import recognition has been fixed
    * Fixed by saving the current line to the line cache if we were left expecting some tokens for the import. Once we get more input, try to parse it again
    * The implementation used to fix this problem also implicitly means that the `done` method is now more important (although, until now, it was still always necessary)
  * Type compatiblity checking has been fixed
    * We were erroneously comparing each of the host type's modifiers with the length of the guest type's modifier array
    * i.e. Before: `ourModifiers[i] == theirModifiers.size()`; Now: `ourModifiers[i] == theirModifiers[i]` :+1:

## [0.5.1] - 2018-11-22
### Added
  * semver.c
    * For some reason, it hadn't been added in the last few commits (even though it should've been)

## [0.5.0] - 2018-11-22
Oh boy, this one's a big one.
### Changed
#### Module System
  * The standard library structure has been changed up a bit
    * Previously, stdlib packages were expected to be created as single modules
    * Now, stdlib packages are expected to be set up just like regular packages, each with their own folder, `package.alta.yaml`, and main module.
### Added
#### Waterwheel (lexer), Palo (parser), AST, DET
  * **Function calls** have been added (!)
#### Module System, AST, DET
  * Modules now include their versions as part of their full name (to prevent module version conflicts)
    * The full name is what's used when mangling names, it basically makes it easy to single out a specific item
#### DET
  * `Module`s now carry around their package information with them
#### Module System
  * We now properly implement semver version parsing for packages (by adding a new dependency: [semver.c](https://github.com/h2non/semver.c))
#### Palo (parser)
  * Wrapped expressions (expressions surrounded by parentheses) are now a thing (e.g. `(5)`, `(foobar)`, `5 * (aThing + 4)`)
### Fixed
#### Palo (parser)
  * Parenthesized types should now properly have any additional modifiers appended
    * i.e. Before, `ref (ptr byte)` would only be interpreted as `ptr byte`. Now it's correctly recognized as `ref ptr byte`
#### DET
  * Function parameters can now *actually be used* (😮)
    * They weren't being added into the function's scope; now they are
  * Scopes now properly search for scope items
    * Previously, all items found with the given name would be returned
    * Now, scope items that aren't functions can only return one result, and that's the first one found when traveling up the scope tree. Scope items that are functions will return all overloaded results, but any overloads from parent scopes with the same signature as overloads from the current scope will be ignored (i.e. current scope overloads take precedence over parent scope overloads)
  * Function-pointer types should now work properly when the same function-pointer type is used in one function and later outside the scope of that function
    * Fixed by hoisting the typedef definitions to the global scope

## [0.4.0] - 2018-11-18
### Added
#### Waterwheel (lexer)
  * New token: `Returns` (i.e. `->`)
#### Palo (parser), AST, DET
  * Function-pointer type support

## [0.3.1] - 2018-11-18
### Fixed
#### DET
  * Use the same `Type` nodes for a `Function`'s `parameterVariables` and `parameters`
    * Previously, it would be deep cloned when populating the `parameterVariables` vector
  * Assign the `Function`'s scope to the `parentScope` property of parameter variables

## [0.3.0] - 2018-11-18
### Fixed
#### Palo (parser)
  * Fixed freestyle cherry-pick imports
    * Previously, a single import done in this style would work fine, but multiple imports of this style wouldn't
    * Fixed by imitating the braced cherry-pick imports logic
### Added
#### Palo (parser)
  * Factored out most of the parser logic into its own generic class
    * Customizable to accommodate any future parsing needs
    * Might be able to move the actually parser logic into its own library 🤔
#### Preprocessor
  * Brand new AltaCore component
  * Enables compile-time code alternation
    * This is just a fancy way of saying that it allows different code to be enabled given certain conditions at compile time
  * Currently supports 5 different directives:
    * `if <expression>`
    * `else if <expression>`
    * `else`
    * `define <definition-name> [value-expression]`
    * `undefine <definition-name>`
  * Sufficient variety of expressions:
    * Boolean logic operators: `&&` and `||`
    * Boolean literals: `true` and `false`
    * String literals (e.g. `"foobar"`)
    * Macro calls (e.g. `defined(foobar)`; note: only builtin macros are supported at the moment)
    * Comparative operators: `==`  (only `==` is supported for now)
  * Definition substitution (`@<definition-name>@`, where `<definition-name>` is the name of the definition to substitute)
    * Not supported in string literals or import statements
  * 4 different expression types:
    * Boolean
    * String
    * Null
    * Undefined

## [0.2.0] - 2018-11-13
### Added
#### Waterwheel (lexer)
  * New tokens:
    * Addition and subtraction signs (`+` and `-`)
    * Asterisks and forward slashes (`*` and `/`)
#### Palo (parser)
  * `Expectation`s can now be coerced to `bool`s
    * This allows easy validation (i.e. `if (someExpectation)` instead of `if (someExpectation.valid)`)
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
  * Failure cache to avoid repetition and dramatically improve parse times and efficiency
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
