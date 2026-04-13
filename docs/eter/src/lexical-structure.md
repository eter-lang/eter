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
1. literals (e.g., `42`, `"hello"`, etc.)
2. keywords (e.g., `if`, `else`, `while`, etc.)
3. identifiers (e.g., `foo`, `bar`, etc.)
4. operators (e.g., `+`, `-`, `*`, etc.)
5. delimiters (e.g., `(`, `)`, `{`, `}`, etc.)

> [!NOTE]
> The input `a << b` is translated to a sequence of 4 tokens: identifier, raw-operator, raw-operator, identifier. The first and third tokens are known to be followed by an inline space.

Unless otherwise specified, the token recognized at a given lexical position is the one having the longest possible sequence of characters.
