# Memory model

## Mutable Value Semantics (MVS) 

Racordon _et al._ introduced the [Mutable Value Semantics] (MVS) as a new memory model for high-performance programming languages.[^1]
Both [Swift] and [Hylo] (formerly Val) adopt this model in their design, but in different ways.  The former tries to make values look like safe references, while the latter treats everything as a pure value, eliminating the need for a Garbage Collector (GC) or Atomic Reference Counting (ARC) altogether.

<details>

<summary>A deeper dive into the differences between Swift's and Hylo's MVS</summary>

### Swift's MVS

In Swift, for **value types** (structs), the MVS is implemented via a copy-on-write (CoW) mechanism. When you assign an array to a new variable, Swift does not immediately copy the data; both variables point to the same memory. The copy only occurs if one of the instances is modified. This requires hidden reference counting (ARC) to track how many _owners_ there are. If the count is greater than 1, a copy is performed. This introduces some latency, as every mutation involves a small ARC check, which can cause overhead in high-performance contexts.
To illustrate this, consider the following Swift code:
```swift
var a = [1, 2, 3] // 'a' owns the array
var b = a         // 'b' shares ownership with 'a', no copy yet
b.append(4)       // 'b' mutates the array, triggers a copy due to ARC check
print(a)          // Output: [1, 2, 3]
print(b)          // Output: [1, 2, 3, 4]
```
Additionally, Swift's MVS allows for **reference types** (classes) that can be shared and mutated without copying, which can lead to confusion and bugs if not used carefully — i.e., it allows aliasing and mutation, which can break local reasoning about code.
Consider the following example:
```swift
class MyClass {
    var value: Int
    init(value: Int) { self.value = value }
}
var obj1 = MyClass(value: 10) // 'obj1' owns the instance
var obj2 = obj1               // 'obj2' shares ownership with 'obj1', no copy, both point to the same instance
obj2.value = 20               // Mutating 'obj2' also mutates 'obj1' due to shared reference
print(obj1.value)             // Output: 20
print(obj2.value)             // Output: 20
```

Over the years, Swift has added various keywords to manage mutability and ownership, such as `mutating` for value types, `inout` for function parameters, and `consuming` for move semantics. However, these features can be complex and may not always provide the performance benefits that MVS aims to achieve, especially in scenarios where fine-grained control over memory is required.

### Hylo's MVS

Hylo takes a more radical approach to MVS. It removes the CoW mechanism in favor of an _ownership model_ where values are truly independent and an _in-place mutation_ semantics that allows for efficient updates without copying. The language statically guarantees that there are no shared mutable references, which eliminates the need for a GC or ARC. Consider the following Hylo code:
```rust
let a = [1, 2, 3] // 'a' owns the array
var b = a         // 'b' is a new value, a copy of 'a', no shared ownership
b.append(4)       // 'b' mutates its own copy, no effect on 'a'
print(a)          // Output: [1, 2, 3]
print(b)          // Output: [1, 2, 3, 4]
```
Note that, if `a` is not used after `b` is created, the compiler can **move** the array from `a` to `b` without copying, since `a` is no longer needed. This allows for efficient memory usage while maintaining the benefits of value semantics.

Hylo has no reference types but allows, via `inout` and `sink` keywords, for efficient mutation of values without copying. 
To provide two examples of this, consider the following Hylo code:
```rust
var point = (x: 0.0, y: 1.0) // 'point' owns the tuple
inout x = &point.x           // 'x' is a mutable reference to 'point.x', no copy, but safe due to ownership rules
&x = 3.14                    // Mutating 'x' updates 'point.x' directly, no copy, and safe due to ownership rules
print(point)                 // Output: (x: 3.14, y: 1.0)
```
```rust
fun offset_sink(_ base: sink Vector2, by delta: Vector2) -> Vector2 {
  &base.x += delta.x 
  &base.y += delta.y
  return base        // Escaping the sink value allows it to be returned without copying, as ownership is transferred to the caller
}

fun main() {
  let v = (x: 1, y: 2)
  print(offset_sink(v, (x: 3, y: 5)))  // Output: (x: 4, y: 7)
  print(v)                             // Error: 'v' has been moved and cannot be used after being passed as a sink
}
```

</details>

To show the power of MVS, let's consider how Rust's ownership and borrowing system compares to Hylo's MVS model.

Rust's ownership and borrowing system is a powerful tool for ensuring memory safety, but it can be complex and may not always provide the benefits that MVS aims to achieve. Consider the following Rust code:
```rust
struct T;
fn own_t(t: T) {}

fn ref_mut_t(t: &mut T) {
    own_t(*t); // cannot move out of `*t` which 
               // is behind a mutable reference
    *t = T;
}
```
In this example, the `ref_mut_t` function attempts to move the value `t` out of the mutable reference, which is not allowed in Rust. This is because Rust's ownership system does not allow dereferencing a mutable reference to move the value it points to, as it would violate Rust's guarantees about memory safety. To work around this, you would need to copy the value or use some other mechanism to transfer ownership, which can introduce overhead and complexity.

In Hylo, the same code would be valid and would not require any special handling, as the language's MVS model allows for efficient mutation of values without copying.
```rust
type T {}
fun own_t(_ t: sink T) {}

fun ref_mut_t(_ t: inout T) {
    // In Hylo, we can pass `inout t` as a sink to `own_t`,
    // which allows us to transfer ownership without copying.
    own_t(t) 
    
    // `t` cannot be used after being passed as a 
    // sink parameter, as it has been moved.

    t = T()
}
```
In this example, the `ref_mut_t` function can pass `t` as a sink to `own_t`, which allows for efficient mutation of the value without copying. After `own_t` is called, `t` is considered "moved" and cannot be used until it is re-initialized, which is a safe and efficient way to manage memory.


## Eter's memory model

Eter embraces the MVS as a core part of its memory model.
In Eter, all values are truly independent and can be mutated in place without copying, while the compiler statically guarantees that there are no shared mutable references. 
This eliminates the need for a GC or ARC and allows for efficient memory usage while maintaining the benefits of value semantics.

### The building blocks of memory

**Storage Unit.**  &#8193; The basic storage element in Eter's memory model is the byte, which is made up of 8 consecutive bits. An Eter program's memory is composed of one or more blocks of adjacent bytes, and each byte is identified by a distinct address.


**Memory Location.**  &#8193; A _memory location_ is a contiguous sequence of bytes that can be read from or written to as a unit. 
Each memory location is sized according to the type of value it holds, and it has a unique address that identifies its starting byte in memory.
Two memory locations are considered _disjoint_ if they do not overlap in memory, meaning that they do not share any bytes. 
A memory location `ml1` _contains_ another memory location `ml2` if the bytes of `ml2` are a subset of the bytes of `ml1`.
Eter's memory model ensures that all memory locations are either disjoint or one contains the other, which allows for efficient memory management and prevents issues related to aliasing and shared mutable state.
 A _sub-memory location_ is a memory location that is contained within another memory location. A location is called _root memory location_ if it is not a sub-memory location of any other location.

### Memory location lifetime

> [!WARNING]
> TODO


### `let` variables

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Immutable Ownership) | **No** (Read-Only) | **View** (Shared) |

A `let` variable is an immutable value that cannot be modified after it is initialized. 
When a `let` variable is assigned a value, it creates a new memory location to store that value.
If a `let` variable is assigned another `let` variable, the compiler can optimize this assignment by creating a reference to the original memory location instead of copying the value, since both variables are immutable and cannot be modified. 
This allows for efficient memory usage while maintaining the benefits of value semantics.
A `let` variable is valid as long as it is in scope, i.e., cannot be moved to another variable or function without copying the value, since it is immutable and cannot be modified after initialization.[^2]

The following example illustrates the behavior of `let` variables in Eter's memory model:
```rust
let x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a let variable that owns the array.
let y: arr<i32>[3] = x; // 'y' is a new let variable that references the same array.
// Both 'x' and 'y' point to the same memory location, no copy is made.
// 'x' and 'y' are alive and can be used, but cannot be modified.
```


### `mut` variables

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Mutable Ownership) | **Yes** (Mutable) | **Copy** (Independent) |

A `mut` variable is a mutable value that can be modified after it is initialized. 
When a `mut` variable is assigned a value, it creates a new memory location to store that value.
If a `mut` variable is assigned another `mut` variable, the compiler must create a copy of the value to ensure that both variables have their own independent memory locations, since they can be modified. 
This allows for efficient mutation of values without copying and eliminates the possibility of shared mutable references.
A `mut` variable is not always valid in scope, as it can be moved to another variable or function without copying the value, since it is mutable and can be modified after initialization.

The following example illustrates the behavior of `mut` variables in Eter's memory model:
```rust
mut x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a mut variable that owns the array.
mut y: arr<i32>[3] = x; // 'y' is a new mut variable that is a copy of 'x'.
// 'y' has its own memory location, and modifications to 'y' do not affect 'x'.
y[0] = 10;
// x = [1, 2, 3]
// y = [10, 2, 3]
```

### `own` variables

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Exclusive Ownership) | **Yes** (Mutable) | **Move** (Transfer) |

A `own` variable is a mutable value that has exclusive ownership of its memory location. 
When an `own` variable is assigned a value, it creates a new memory location to store that value.
If an `own` variable is assigned another `own` variable, the compiler can optimize this assignment by transferring ownership of the memory location from the source variable to the destination variable without copying the value, since the source variable will no longer be valid after the transfer. 
This allows for efficient memory usage while maintaining the benefits of value semantics.
An `own` variable is not valid in scope after it has been moved to another variable or function, since it has exclusive ownership of its memory location and cannot be shared or modified after the transfer.

The following example illustrates the behavior of `own` variables in Eter's memory model:
```rust
own x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is an own variable that owns the array.
own y: arr<i32>[3] = x; // 'y' is a new own variable that takes ownership of the array from 'x'.
// 'x' is no longer valid and cannot be used, while 'y' now owns the array and can be modified.
y[0] = 10;
// x = invalid
// y = [10, 2, 3]
```

### `proj` variables

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **No** (Loan) | **Yes** (In-place Mutation) | **Exclusive/Anchored** (Direct) |

A `proj` variable is a mutable value that does not have ownership of its memory location but can mutate it in place. 
When a `proj` variable is assigned a value, it creates a new memory location to store that value.
If a `proj` variable is assigned another `proj` variable, the compiler can optimize this assignment by creating a reference to the original memory location instead of copying the value, since both variables can mutate the same memory location. 
This allows for efficient mutation of values without copying and eliminates the possibility of shared mutable references, while still allowing for efficient memory usage.
A `proj` variable is not valid in scope after it has been moved to another variable or function, since it does not have ownership of its memory location and cannot be shared or modified after the transfer.

The following example illustrates the behavior of `proj` variables in Eter's memory model:
```rust
proj x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a proj variable that references the array.
proj y: arr<i32>[3] = x; // 'y' is a new proj variable that references the same array as 'x'.
// Both 'x' and 'y' reference the same memory location, and modifications to either 'x' or 'y' affect the same array.
y[0] = 10;
// x = [10, 2, 3]
// y = [10, 2, 3]
```

The projections follow the McCall's [Law of Exclusivity].
In the Racordon _et al._'s words, it creates a crucial optimization opportunity: it is safe to sidestep the conceptual copies by allowing the callee to write the argument's memory in the caller's context.
That is, `proj` argument passing can be implemented as pass-by-reference without surfacing reference semantics in the programming model.
The following example exemplifies the [Law of Exclusivity] in Eter's memory model:
```rust
proj x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a proj variable that references the array.
proj y: arr<i32>[3] = x; // 'y' is a new proj variable that references the same array as 'x'.
proj z: arr<i32>[3] = x; // ❌ ERROR: 'z' cannot reference the same array as 'x' and 'y' due to the Law of Exclusivity.
```
The trasparent pass-by-reference semantics of `proj` variables is shown in the following example:
```rust
fn increment(proj a: arr<i32>[3]) { /* Increment each element of the array in place */ }

proj x: arr<i32>[3] = arr<i32>[1, 2, 3];
increment(x); // Passing 'x' as a proj argument allows for direct modification of the array
// x = [2, 3, 4]
```

#### The "Lexical Prison"

It is important to note that `proj` bindings are fundamentally similar to the concept of Naded _et al._'s [Borrowing], found in languages like Rust. However, the core distinction lies in how your language ensures the uniqueness of the data being accessed — that is, the guarantee that there are no other references to the same data that could lead to aliasing and shared mutable state.

Unlike general-purpose references, a `proj` is a _second-class citizen_. This means it is restricted by a lexically-bounded lifetime: a `proj` cannot be stored in long-lived data structures or "escape" the scope in which it was defined. It must always "[appear in person]", meaning it exists only as a direct, traceable link to the original value it references.

Consider the following example to showcase the difference between `proj` and Rust's borrowing:
```rust
// Rust's borrowing system allows for multiple mutable references, which can lead to aliasing issues.
fn get_first(v: &'a mut Vec<i32>) -> &'a mut i32 {
    &mut v[0] // Legal: The borrow "escapes" the function
}
```
Eter's `proj` system, on the other hand, does not allow for such escaping references, ensuring that the path to the value being accessed is unique and cannot be aliased:
```rust
fn get_first(proj v: arr<i32>) -> proj i32 {
    ret proj v[0]; // ❌ ERROR: 'proj' cannot be returned. 
                   // It must "appear in person" and cannot escape its lexical scope.
}
```

#### Path-Based Exclusivity vs. Pointer Aliasing


Because `proj` is restricted in this way, the compiler can prevent dangerous aliasing  simply by checking the "path" to the value being accessed.
A path consists of the variable name followed by any member accesses (like `.field`) or array subscripts (like `[0]`).

The system ensures safety by verifying that the same path, or overlapping segments of that path, never appears more than once in the same context. If `proj` were a "first-class" citizen (like a pointer that could be passed around freely), the compiler would have to perform complex analysis to guess what a variable points to. With `proj`, the path itself is the identity.

For instance, consider the following example:
```rust
struct Point { mut x: i32, mut y: i32 }

fn move_point(proj p: Point, proj target: i32) { /* Move 'p' towards 'target' in place */ }

fn main() {
    mut my_point: Point = Point { x: 0, y: 0 };
    mut offset: i32 = 5;
    move_point(proj my_point, proj offset); // OK: 'my_point' and 'offset' are distinct paths.
    move_point(proj my_point, proj my_point.x); // ❌ ERROR: 'my_point' and 'my_point.x' overlap paths. The sub-tree 'my_point.x' is part of the path 'my_point', so they cannot both be 'proj' in the same context.
}
```

#### Conservative Array Indexing

A specific restriction applies when a `proj` identifies an element within an array. The type system allows multiple `proj` bindings into the same array only if it can mathematically prove that the indices do not overlap.

**Static Certainty:** Accessing x[0] and x[1] via `proj` is permitted because the indices are distinct constants.

**Dynamic Uncertainty:** Accessing x[f(0)] and x[1] is prohibited. In this scenario, the type system is conservative: it assumes that f(0) could potentially evaluate to 1. Since it cannot guarantee exclusivity, it rejects the code to prevent a potential collision.

The following example illustrates this concept:
```rust
fun swap(proj a: i32, proj b: i32) { /* Swap the values of 'a' and 'b' in place */ }

fun main() {
    mut x: arr<i32>[3] = [10, 20, 30];

    swap(proj x[0], proj x[1]) // OK: The compiler can statically verify that 'x[0]' and 'x[1]' are distinct paths, so it allows the swap.

    let i = get_index() // Assume 'get_index()' returns an integer at runtime.
    swap(proj x[i], proj x[1]) // ❌ ERROR: The compiler is CONSERVATIVE. 
                               // Because 'i' could be 1 at runtime, it assumes the paths 
                               // 'x[i]' and 'x[1]' MIGHT overlap. 
                               // To stay safe without a borrow checker, it rejects the call.
}
```

#### Ownership Transfer through Projections

As shown at the beginning of this chapter, a critical advantage of the MVS model over Rust's borrowing system is the ability to temporarily move a value out of a projection. In traditional reference systems, a mutable reference is a "look but don't take" contract: you can modify the data, but you cannot steal it.

In Eter, a `proj` binding is more flexible. 
It acknowledges that while a projection is active, the projector is the temporary owner of the value it references. 
This allows to "empty" the slot by moving the value (transferring it to a own parameter) and then "refilling" it later.

Concretely, the following code is valid in Eter but would be illegal in Rust:
```rust
struct T {}
fn own_t(own t: T) {} 

fn ref_mut_t(proj t: T) {
    own_t(t); // Move the value out of the projection, transferring ownership to 'own_t'.
    // 't' is now considered "moved" and cannot be used until it is re-initialized.
    t = T;    // Refill the projection with a new value after the move.
}
```

The constraints of the `proj` system ensure that this pattern is safe and does not lead to aliasing or shared mutable state, while still allowing for efficient mutation of values without copying.


---
---
---
---
---
---
---
---
---
---
---
---
---
---
---
---
---
---



### Value transfer semantics

| ↓ to \ from → | `let x = 𝓿;` | `mut x = 𝓿;` | `own x = 𝓿;` | `proj x = 𝓿;` |
|-|-|-|-|-|
| `let  y = x;` | $O(1)$ ✅ (View)        | $O(1)$ ✅ (View) | $O(1)$ ✅ (View) | $O(1)$ ✅ (Sub-View) |
| `mut  y = x;` | $O(N)$ ✅ (Copy)       | $O(N)$ ✅ (Copy) | $O(N)$ ✅ (Copy) | $O(N)$ ✅ (Copy) |
| `own  y = x;` | ❌ (Cannot move `let`)  | $O(1)$ ✅ (Move) | $O(1)$ ✅ (Move) | ❌ (Cannot move `proj`) | 
| `proj y = x;` | ❌ (Needs `mut` source) | $O(1)$ ✅ (Link) | $O(1)$ ✅ (Link) | $O(1)$ ✅ (Re-Link) |

### Interprocedural Value transfer semantics

| ↓ to \ from → | `let x = 𝓿;` | `mut x = 𝓿;` | `own x = 𝓿;` | `proj x = 𝓿;` |
|-|-|-|-|-|
| `fn_let(let x)` | TODO | TODO | TODO | TODO |
| `fn_mut(mut x)` | TODO | TODO | TODO | TODO |
| `fn_own(own x)` | TODO | TODO | TODO | TODO |
| `fn_proj(proj x)` | TODO | TODO | TODO | TODO |




### Use of Pointers (unsafe scopes)
 - `let ptr<i32>` 
 - `let mut ptr<i32>`
 - `let ptr<mut i32>`
 - `let mut ptr<mut i32>` 



### Threads and Concurrency

> [!WARNING]
> This section limits itself to a brief overview of how Eter's memory model interacts with threads and concurrency. Since in its primordial design Eter is a single-threaded language, but with plans to support multi-threading in the future, the details of this interaction are still being explored and may evolve as the language develops.




[^1]: We refer the reader to the following resources: (i) The research section of the [hylo-lang website](https://hylo-lang.org/docs/contributing/research-collab/) for a comprehensive list of papers related to MVS, (ii) the [Hylo language specification](https://hylo-lang.org/docs/reference/specification/), (iii) the [Utilizing value semantics in Swift](https://www.swiftbysundell.com/articles/utilizing-value-semantics-in-swift/) post, and (iv) the [Ruminating about mutable value semantics](https://www.scattered-thoughts.net/writing/ruminating-about-mutable-value-semantics/) post.

[^2]: For Rust developers, it's important to note that `let` variables in Eter are different from Rust's `let` bindings. In Eter, `let` variables are always valid in scope and cannot be moved or modified after initialization, while in Rust, `let` bindings can be mutable and can be moved or modified depending on the context. This distinction is crucial for understanding how Eter's memory model works and how it differs from Rust's ownership and borrowing system.


[Mutable Value Semantics]: https://kyouko-taiga.github.io/assets/papers/jot2022-mvs.pdf
[Law of Exclusivity]: https://github.com/swiftlang/swift/blob/main/docs/OwnershipManifesto.md
[Borrowing]: https://doi.org/10.1145/2103656.2103722
[appear in person]: https://link.springer.com/article/10.1023/A:1010000313106
[Hylo]: https://hylo-lang.org/
[Swift]: https://swift.org/

