# Lexical structure

A sequence of Unicode characters is translated into a sequence of tokens. The following rules apply during translation:

1. Comments are treated as though they are a single space `U+20`.
2. Spaces are ignored unless they appear between the opening and closing delimiters of a character or string literal. Unicode characters with the "White_Space" property are recognized as spaces.
3. New-line delimiters are ignored unless they appear between the opening and closing delimiters of a character or string literal. Unicode characters with the "Line_Break" property are recognized as new-line delimiters.

## Comments

The character sequence `//` starts a single-line comment, which terminates immediately before the next new-line delimiter.

The character sequences `/*` and `*/` are multiline comment opening and closing delimiters, respectively. A multiline comment opening delimiter starts a comment that terminates immediately after a matching closing delimiter. Each opening delimiter must have a matching closing delimiter. Multiline comments may nest and need not contain any new-line characters. 

> [!NOTE]
> The character sequences `//` have no special meaning in a multiline comment. The character sequences `/*` and `*/` have no special meaning in a single-line comment. The character sequences `//` and `/*` have no special meaning in a string literal. String and character literal delimiters have no special meaning in a comment.

## Tokens

A token is a terminal symbol of the syntactic grammar. It falls into one of five categories:

| Category | Description | Examples |
| :--- | :--- | :--- |
| **Literals** | Fixed values in the source code | `42`, `"hello"`, `true` |
| **Keywords** | Reserved words with special meaning | `if`, `else`, `while` |
| **Identifiers** | Names given to entities (variables, functions, etc.) | `foo`, `bar`, `count` |
| **Operators** | Symbols representing computations or logic | `+`, `-`, `*`, `==` |
| **Delimiters** | Symbols used for grouping and structure | `(`, `)`, `{`, `}` |

> [!NOTE]
> The input `a << b` is translated to a sequence of 4 tokens: identifier, raw-operator, raw-operator, identifier.

Unless otherwise specified, the token recognized at a given lexical position is the one having the longest possible sequence of characters.

---

### Literals

Literals are tokens representing fixed values in the source code. The language supports the following types of literals:

| Literal Type | Description | Standard Examples | Special/Escaped Examples |
| :--- | :--- | :--- | :--- |
| **Integer** | Decimal and hexadecimal formats | `123` | `0x1A`, `0x_FF_00` |
| **Floating-Point** | Like an integer literal but includes a decimal point `.` | `3.14159`, `0.5` | |
| **Boolean** | Represent truth values | `true`, `false` | |
| **Character** | Single Unicode scalar value enclosed in single quotes | `'a'`, `'R'` | `'\n'`, `'\t'`, `'\''`, `'\u{1F600}'` |
| **String** | Sequence of characters enclosed in double quotes | `"hello world"` | `r"C:\Path"`, `r#"He said, "Hello!""#` |
| **C-Style String** | Null-terminated string literal identified by the `@c` qualifier | `@c"hello"` | `@c"c style string"` |

### Keywords 

Keywords are reserved words that have special meaning in the language. They cannot be used as identifiers. The language defines the following keywords:

| Strict Keywords | | | |
| :--- | :--- | :--- | :--- |
| `as` | `enum` | `match` | `true` |
| `break` | `false` | `mod` | `type` |
| `const` | `fn` | `mut` | `use` |
| `continue` | `for` | `pub` | `where` |
| `if` | `ref` | `while` | `else` |
|`return` | `let` | `struct` | 

*Note: Some keywords might be reserved for future use or macro rules.*

### Identifiers 

An identifier is a name used to identify a variable, function, class, module, or other user-defined item. 

Identifiers must follow these rules:
1. They must begin with a letter (`a`-`z`, `A`-`Z`) or an underscore `_`.
2. Subsequent characters may be letters, digits (`0`-`9`), or underscores `_`.
3. They cannot match one of the reserved keywords.

| Status | Example | Reason |
| :--- | :--- | :--- |
| **Valid** | `my_var`, `_private`, `Count1` | Follows all rules. |
| **Invalid** | `1stPlace` | Cannot start with a digit. |
| **Invalid** | `my-var` | Hyphens are not allowed (parsed as subtraction). |
| **Invalid** | `if` | `if` is a reserved keyword. |

### Operators 

Operators are special symbols or combinations of symbols used to perform operations on values or variables.

| Category | Operators | Description |
| :--- | :--- | :--- |
| **Arithmetic** | `+`, `-`, `*`, `/`, `%`, `++`, `--` | Standard mathematical operations, increment, and decrement. |
| **Bitwise** | `&`, `\|`, `^`, `<<`, `>>` | Operations on the bit level. |
| **Logical** | `&&`, `\|\|`, `!` | Boolean logic (AND, OR, NOT). |
| **Comparison** | `==`, `!=`, `<`, `>`, `<=`, `>=` | Relational equality and ordering. |
| **Assignment** | `=`, `+=`, `-=`, `*=`, `/=`, etc. | Assigns values, optionally compound. |
| **Structural** | `.`, `=>`| Member access, matching. |

### Delimiters 

Delimiters are punctuation characters used to group tokens, separate lists, or define structure.

| Delimiter | Name | Primary Usage |
| :--- | :--- | :--- |
| `(` `)` | Parentheses | Function calls, grouping expressions, tuples. |
| `{` `}` | Braces | Block expressions, struct definitions, matching bodies. |
| `[` `]` | Brackets | Array indexing, slices, array literals. |
| `,` | Comma | Separating arguments, tuple elements, and list items. |
| `;` | Semicolon | Terminating statements. |
| `:` | Colon | Type annotations, struct field initialization, return types. |
| `::` | Double Colon | Path separation (namespaces, modules). |





