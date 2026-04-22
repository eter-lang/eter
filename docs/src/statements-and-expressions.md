# Statements and expressions

Eter is an expression-oriented language, meaning that most semantic constructs within the language evaluate to a value. To understand the syntactic and semantic structure of a program, it is fundamental to distinguish between **statements** and **expressions**:

* **Expressions** are combinations of variables, literals, operators, and function calls that are evaluated by the compiler to compute and return a resulting value. Every expression has a well-defined type. Because they yield values, expressions can be arbitrarily nested and composed to build more complex logic.
* **Statements** are instructions that dictate the control flow and state mutations of a program. They do not evaluate to a usable value (conceptually, they evaluate to the unit type `()`). Their primary purpose is to execute side effects, such as introducing new bindings into a scope, modifying existing memory locations, or defining items.

In short: expressions *compute* values, while statements *perform* actions.

Expressions and statements are intrinsically linked: you can convert an expression into an **expression statement** by appending a semicolon `;`. This forces the compiler to evaluate the expression solely for its side effects (e.g., executing a function call) while explicitly discarding any resulting value.

## Statements

Statements are executed in sequence within a block. Eter supports several types of statements, including item declarations, assignments, and expression statements.

### Statement terminator

Statements are generally terminated by a semicolon `;`. The semicolon indicates the end of a statement and separates it from the next one.

> [!NOTE]
> Some statements, such as those ending with a block expression (e.g., `if`, `while`, `match`), do not require a trailing semicolon unless they are part of a larger expression or assignment.

### Assignment statements

An assignment statement evaluates an expression and binds its value to a memory location represented by a place expression (such as a variable, a struct field, or an array index).

The syntax for an assignment statement uses the `=` operator or a compound assignment operator (e.g., `+=`, `-=`, `*=`).

| Type | Example | Description |
| :--- | :--- | :--- |
| **Basic Assignment** | `x = 5;` | Assigns the value `5` to the variable `x`. |
| **Compound Assignment** | `y += 2;` | Adds `2` to `y` and assigns the result back to `y`. |
| **Field Assignment** | `point.x = 10;` | Assigns `10` to the field `x` of `point`. |

Assignment statements perform an action and do not evaluate to a value, meaning they cannot be chained like `a = b = c`.

### Item declaration statements (`let`)

Item declaration statements introduce new items into the current scope. The most common item declaration within a block is the `let` statement, which binds a new local variable by **always specifying its name and type**. Variables are immutable by default unless marked with the `mut` keyword (more in the [Memory Model Chapter](./memory-model.md)).

| Declaration | Description | Example |
| :--- | :--- | :--- |
| **Variable Binding** | Binds a value to a new local variable, with a required type annotation. | `let name: String = "Alice";` |
| **Mutable Binding** | Binds a value to a mutable local variable. | `mut count: i32 = 0;` |
| **Constant Declaration** | Defines a compile-time constant. | `let MAX_VAL: u32 = 100;` |

> [!NOTE]
> Constant names are typically written in `UPPER_SNAKE_CASE` by convention, but they are not constrained to be uppercase by the language.

#### Nested Items and Scoping

Items (such as nested `let` variables) can be declared inside any block scope. This allows you to restrict the visibility of an item strictly to the block it was declared in, keeping the outer namespace clean.

Nested items can be declared inside regular function blocks, unnamed (anonymous) scopes `{ ... }`, and `unsafe { ... }` blocks. They are particularly useful inside functional blocks like `if` or `if-else` expressions to encapsulate temporary logic.

```rust
fn main() {
    let outer_val: i32 = 10;
    let condition: bool = true;

    // 1. Functional Block Scope (if-else)
    // Useful for isolating variables or helper logic
    // that is only needed for a specific branch.
    if condition {
        let inner_val: i32 = 20;
        let LOCAL_MULTIPLIER: i32 = 5;
        let result: i32 = inner_val * LOCAL_MULTIPLIER;
        do_something(result);
    } else {
        let fallback_val: i32 = 0;
        do_something(fallback_val);
    }
    // Error! `inner_val`, `LOCAL_MULTIPLIER`, and `fallback_val`
    // are out of scope and cannot be accessed here.

    // 2. Unsafe Block Scope
    // Used to wrap operations that bypass some of the compiler's safety checks.
    // Like any block, you can declare local items inside it.
    unsafe {
        // Declaring a local item inside an unsafe block
        let pointer: ptr<i32> = ptr::new(outer_val);
        let dereferenced: i32 = ptr::value(pointer);
    }
    // `pointer` is out of scope here.
}
```

### Expression statements

An expression statement is an expression that is evaluated for its side effects, followed by a semicolon. The value produced by the expression is discarded.

Common examples include function calls.

| Type | Example | Description |
| :--- | :--- | :--- |
| **Function Call** | `do_something("Hello");` | Executes the function and discards the return value. |

When a block-based expression (such as `if`, `match`, or a simple `{ ... }` block) is used as an expression statement, the trailing semicolon is optional.

---

## Expressions

An expression evaluates to a value and can be used in most places where a value is expected. Expressions can be nested and combined.

### Literal expressions

A literal expression consists of a literal token and evaluates to the value represented by that token.

```rust
let age: i32 = 42;              // Integer literal
let name: str = "Alice";       // String literal
let is_valid: bool = true;      // Boolean literal
let letter: char = 'A';         // Character literal
```

### Path expressions (::)

A path expression refers to an item, variable, or constant in the current scope or another module using the path separator `::`. You can also use paths to explicitly refer to the current module (`self`) or the parent module (`super`).

```rust
let max_val: u32 = std::u32::MAX;    // Fully qualified path to a constant
let math_pi: f64 = math::PI;         // Path to a module item

// Accessing an item within the current actual scope
let current_item: i32 = self::helper_function();
```

### Block expressions

A block expression is a sequence of statements enclosed in braces `{}`. The value of a block expression is the value of its final expression (the one without a trailing semicolon). If the block ends with a statement (with a semicolon), it evaluates to `()`.

```rust
let y: i32 = {
    let x: i32 = 5;
    return x + 1; 
}; // y is now 6
```

### Operator expressions

Operator expressions apply unary or binary operators to operands.

```rust
let sum: i32 = 10 + 20;         // Binary arithmetic operator
let is_false: bool = !true;      // Unary logical NOT operator
let flag: bool = (a == b);       // Binary comparison operator
```

### Grouped expressions

Parentheses `()` can be used to explicitly group expressions and control the order of evaluation, overriding default operator precedence.

```rust
let result: i32 = (2 + 3) * 4;  // Evaluates to 20 instead of 14
```

### Access expressions

Access expressions allow you to retrieve specific elements from compound types like arrays, tuples, and structs. Depending on the underlying type, the syntax to access an element varies.

#### Array and index expressions

Array expressions create fixed-size collections of elements. Index expressions retrieve elements from an array or slice using brackets `[]`. Indexing is always zero-based.

```rust
let a: [i32; 3] = [1, 2, 3];        // Array expression (list of elements)
let zeros: [i32; 5] = [0; 5];       // Array expression (repeated value: [0, 0, 0, 0, 0])
let first: i32 = a[0];              // Index expression (accessing the first element)
```

#### Tensor and index expressions

Tensor expressions create fixed-size, multi-dimensional collections of homogeneous elements (nD tensors). Tensor literal expressions use nested arrays.

```rust
let t: [i32; 2, 2] = [[1, 2], [3, 4]]; // Tensor expression (2x2 matrix)
let element: i32 = t[0, 1];            // Index expression (accessing row 0, col 1)
```

#### Tuple and index expressions

Tuple expressions create ordered, fixed-size, heterogeneous collections. Tuple elements are accessed using dot `.` notation followed by a literal integer index.

```rust
let point: (i32, i32, str) = (10, 20, "label");  // Tuple expression
let x: i32 = point.0;                             // Tuple index expression (gets 10)
let desc: str = point.2;                         // Tuple index expression (gets "label")
```

#### Struct expressions and Field access

Struct expressions create instances of user-defined struct types. They specify the name of the struct and provide values for its fields. Field access expressions retrieve the value of a specific named field from a struct or union using the dot `.` operator.

```rust
// Struct instantiation expression
let p: Point = Point { x: 10, y: 20 }; 

// Field access expression
let my_x: i32 = p.x;                 // Accesses the 'x' field of the struct 'p'
```

### Call expressions

Call expressions invoke functions or closures. They consist of an expression that evaluates to a callable entity, followed by a parenthesized list of arguments.

```rust
let result: i32 = add(5, 3);         // Function call
```

### Loop expressions

Loop expressions are used to execute a block of code multiple times. Because they are expressions, they can optionally evaluate to a value (e.g., by using the `break` keyword).

Eter supports both `for` and `while` loops.

```rust
// loop expression returning a value
mut counter: i32 = 0;
let final_value: i32 = while true {
    counter += 1;
    if counter == 10 {
        break counter * 2; // Returns 20 from the loop expression
    }
};

// while loop
mut n: i32 = 5;
while n > 0 {
    n -= 1;
}

// for loop

for( i: i32 = 0; i < 10; i++){
    if (i % 2 == 0){
        do_something(i);
    }
}

```

### If expressions

An `if` expression evaluates a boolean condition and executes the corresponding block. If an `else` branch is provided, the `if` expression can evaluate to a value, provided both branches return the same type.

```rust
let condition: bool = true;

// if used as an expression to assign a value
let result: str = if condition {
    "Success"
} else {
    "Failure"
};

// if used for side effects (evaluates to ())
if result == "Success" {
    do_something("Everything is fine");
}
```

### Match expressions

A `match` expression provides pattern matching. It compares a value against a series of patterns and executes the block corresponding to the first matching pattern. Like `if` expressions, `match` blocks can return a value if all branches resolve to the same type.

Patterns can include literals, variables, and the catch-all wildcard `_`. 

Crucially, because each branch in a `match` must evaluate as an expression to return a value, you generally do not use a semicolon to terminate the branch's expression. Instead, you use a comma `,` to separate and define the "next" pattern in the sequence.

```rust
let status_code: i32 = 404;

let message: str = match status_code {
    200 => "OK",                     // Notice the comma instead of a semicolon
    404 => "Not Found",
    500 => "Internal Server Error",
    // The underscore `_` acts as a wildcard, catching any unhandled values
    _ => "Unknown Error",
};
```

You can also use patterns to destructure types or bind variables:

```rust
let coords: (i32, i32) = (0, 10);

match coords {
    (0, 0) => do_something("Origin"),
    (x, 0) => do_something("On the X axis"),
    (0, y) => do_something("On the Y axis"),
    (x, y) => do_something("Somewhere else"),
}
```

### Return expressions

A `return` expression immediately terminates the current function or closure and evaluates to a value that is passed back to the caller. A `return` expression without a value implies `return ()`.

```rust
fn get_positive(val: i32) -> i32 {
    if val < 0 {
        return 0; // Early return expression
    }
    
    return val;
}
```

### Underscore expressions

The underscore `_` can be used as an expression pattern to explicitly discard a value. This is useful when calling a function for its side effects, but explicitly acknowledging that you are choosing not to use its return value. It is also heavily used in `match` blocks as a wildcard (as shown above).

```rust
// Discard the result of a function that returns a value
let _: i32 = compute_heavy_task();
```
