# Introduction

This book is the primary reference for the **Eter** programming language.

It does not serve as an introduction to the language. Background familiarity with the language is assumed.

Additionally, this book does not assume you are reading it sequentially.
Each chapter generally can be read standalone, but will cross-link to other chapters for facets of the language they refer to, but do not discuss.

> [!NOTE]
> For known bugs and omissions in this book, see our [GitHub issues]. If you see a case where the compiler behavior and the text here do not agree, file an issue so we can think about which is correct.

## Eter releases

> [!CAUTION]
> TODO: add release information here once we have a release.

### Conventions

Like all technical books, this book has certain conventions in how it displays information.
These conventions are documented here.

* Statements that define a term contain that term in *italics*.
  Whenever that term is used outside of that chapter, it is usually a link to the section that has this definition.

  An *example term* is an example of a term being defined.

* Notes that contain useful information about the state of the book or point out useful, but mostly out of scope, information are in note blocks.

  > [!NOTE]
  > This is an example note.

* Example blocks show an example that demonstrates some rule or points out some interesting aspect. Some examples may have hidden lines which can be viewed by clicking the eye icon that appears when hovering or tapping the example.

  > [!TIP]
  > This is a code example.
  > ```rust
  > println!("hello world");
  > ```

* Warnings that show unsound behavior in the language or possibly confusing interactions of language features are in a special warning box.

  > [!WARNING]
  > This is an example warning.

* Code snippets inline in the text are inside `<code>` tags.

  Longer code examples are in a syntax highlighted box that has controls for copying, executing, and showing hidden lines in the top right corner.

  ```rust
  # // This is a hidden line.
  fn main() {
      println!("This is a code example");
  }
  ```

  All examples are written for the latest edition unless otherwise stated.

* Informational blocks that contain useful information about the language, but are not normative, are in informational boxes.

  > [!IMPORTANT]
  > This is an important block.

* Caution blocks that contain information about unsound behavior in the language or possibly confusing interactions of language features are in a special caution box.
  > [!CAUTION]
  > This is a caution block.

* The grammar and lexical productions are described in the [Notation] chapter.

## Contributing

We welcome contributions of all kinds.

You can contribute to this book by opening an issue or sending a pull request to [the Eter repository].
If this book does not answer your question, and you think its answer is in scope of it, please do not hesitate to [file an issue].
Knowing what people use this book for the most helps direct our attention to making those sections the best that they can be.
And of course, if you see anything that is wrong or is non-normative but not specifically called out as such, please also [file an issue].

[github issues]: https://github.com/eter-lang/eter/issues
[the Eter repository]: https://github.com/eter-lang/eter/
[file an issue]: https://github.com/eter-lang/eter/issues
[Notation]: notation.md
