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
b.append(4)       // 'b' mutates its mov copy, no effect on 'a'
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

Eter embraces the MVS as a core part of its memory model, but introduces a unique twist by introducing the concept of **circuits** to manage how values are stored, accessed, and mutated in memory.
In Eter, all values are treated as independent entities with their own memory locations, and can be transferred between different circuits that govern their ownership, mutability, and memory semantics.
This eliminates the need for a GC or ARC and allows for efficient memory usage while maintaining the benefits of value semantics.

### The building blocks of memory

**Storage Unit.**  &#8193; The basic storage element in Eter's memory model is the byte, which is made up of 8 consecutive bits. An Eter program's memory is composed of one or more blocks of adjacent bytes, and each byte is identified by a distinct address.


**Memory Location.**  &#8193; A _memory location_ is a contiguous sequence of bytes that can be read from or written to as a unit. 
Each memory location is sized according to the type of value it holds, and it has a unique address that identifies its starting byte in memory.
Two memory locations are considered _disjoint_ if they do not overlap in memory, meaning that they do not share any bytes. 
A memory location `ml1` _contains_ another memory location `ml2` if the bytes of `ml2` are a subset of the bytes of `ml1`.
A _sub-memory location_ is a memory location that is contained within another memory location. A location is called _root memory location_ if it is not a sub-memory location of any other location.
Eter's memory model ensures that all memory locations are either disjoint or one contains the other, which allows for efficient memory management and prevents issues related to aliasing and shared mutable state.

### Memory location lifetime

> [!WARNING]
> TODO

### The Eter's Circuits

Different types of variables in Eter have different ownership, mutability, and memory semantics, which determine how they interact with memory and how they can be used in the program. 
There are three main types of variables in Eter's memory model: `let`, `let mut`, and `let proj`. 
Each of these variable defines a different **circuit** for how values are owned, mutated, and accessed in memory.
One can think of these **circuits** as governing how values flow through the program and how they are stored and accessed in memory.

The Eter programming language uses these circuits consistently. 
It means that the same rules apply to all types of values, whether they are simple scalars, complex structs, or even functions.
Additionally, the semantic is consistent across all operations, including variable assignment, function calls, and field access.

##### Matrix of value transfer semantics

| ↓ to \ from → | `let x = 𝓿` | `let mut x = 𝓿` |  `let proj x = 𝓿` |
|-|-|-|-|
| `let  y = x`      | $O(1)$ ✅ (Alias)        | $O(1)$ ✅ (Move) | ❌ (Forbidden) |
| `let mut  y = x`  | $O(N)$ ⚠️ (Copy)         | $O(1)$ ✅ (Move) | $O(1)$ ✅ (Extract/Refill) |
| `let proj y = &x` | ❌ (Needs `mut` source)  | $O(1)$ ✅ (Link) | $O(1)$ ✅ (Re-Link) |
| `&x = 𝓿'`         | ❌ (Forbidden)           | ✅ (In-place)    | ✅ (In-place) |
| `x = 𝓿'`          | TODO  | TODO   | TODO |



#### The `let` circuit

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Shared) | **No** (Read-Only) | **Alias** (Reference Linked) |

The `let` circuit consists of immutable `let` variables that retain ownership of their values.
A `let` variable is an immutable value that cannot be modified after it is initialized. 
When a `let` variable is assigned a value, it creates a new memory location within the `let` circuit to store that value.
A `let` variable is valid as long as it is in scope, i.e., cannot be moved to another variable or function without copying the value, since it is immutable and cannot be modified after initialization.[^2]
The ownership of a `let` variable is shared, meaning that multiple `let` variables can reference the same memory location and the memory location is not deallocated until all `let` variables that reference it go out of scope.


##### From `let` to `let` circuit

If a `let` variable is assigned another `let` variable it remains in the `let` circuit, and both variables reference the same memory location.
If a `let` variable is assigned another `let` variable, the compiler can optimize this assignment by creating a reference to the original memory location instead of copying the value, since both variables are immutable and cannot be modified. 
For example, consider the following code:
```rust
let x: i32 = 1; // 'x' is a let variable that owns 
                // the integer value.
let y: i32 = x; // 'y' is a new let variable that 
                // references the same integer value as 'x'.
// Both 'x' and 'y' reference the same memory location, 
// and since they are immutable, they cannot be modified.
```

##### From `let` to `mut` circuit

If a `let` variable is assigned to a `mut` variable, it is moved to the `mut` circuit. 
However, since the `let` circuit allows for shared views (multiple variables referencing the same memory location) and the `let` variables must be valid in scope, the compiler performs an implicit copy to ensure that the new `mut` variable has its own independent and exclusive memory location.
This ensures that any subsequent modifications to the `mut` variable do not affect the original `let` variable or any other variables that might be referencing the same data.
The following example illustrates this behavior:
```rust
let x: i32 = 1; // 'x' is a let variable that owns 
                // the integer value.
let mut y: i32 = x; // 'y' is a new mut variable 
                    // that is a copy of 'x'.
// 'y' has its own memory location, and modifications 
// to 'y' do not affect 'x'.
y = 10;
// x = 1
// y = 10
```
If we would admit a direct move from `let` to `mut` without copying, in the presence of concurrent threads or multiple views, we would have a situation where one thread could be modifying the value through the `mut` variable while another thread is still referencing the same memory location through the `let` variable, leading to data races and undefined behavior. Instead, by enforcing an implicit copy, the compiler ensures that each thread or view has its own independent copy of the data, thus maintaining thread safety and preventing data races.
```rust
use std::thread::{self, JoinHandle};

let x: i32 = 1; // 'x' is a let variable that owns 
                // the integer value.

let first: JoinHandle<unit> = thread::spawn(|| { let _ = x; /* Thread 1 reads 'x' */ });
let second: JoinHandle<unit> = thread::spawn(|| { let _ = x; /* Thread 2 reads 'x' */ });

first.join().or_else(|_| panic("Thread 1 panicked"));
second.join().or_else(|_| panic("Thread 2 panicked"));
```


##### From `let` to `mov` circuit

The transition from a `let` variable to a `mov` variable is prohibited by the compiler.
The reason for this restriction is rooted in the fundamental guarantee of the `mov` circuit: exclusive, unique ownership.
To _promote_ a shared `let` value to an exclusive `mov` value without a copy would violate the safety of all other `let` variables currently viewing that memory. 

Therefore, if you need to transform a `let` value into a `mov` value, you must perform an explicit two-step process:
1. First, copy the value into the `mut` circuit (which creates a new, independent memory location).
2. Then, move that `mut` variable into the `mov` circuit.

```rust
let x: i32 = 1;

// let mov y: i32 = x; // ❌ ERROR: Cannot move an immutable 'let' variable 
                       // into the exclusive 'mov' circuit.

let mut tmp = x;       // 1. Implicit copy to create independence.
let mov y = tmp;       // 2. Transfer ownership to the mov circuit.
```

##### From `let` to `proj` circuit

The transition from a `let` variable to a `proj` variable is prohibited by the compiler.
By definition, a projection grants mutable access to a memory location. The `let` circuit, however, operates on the guarantee of immutability. Allowing a projection to be created from a `let` source would create a _backdoor_ to modify data that is supposed to be read-only, potentially affecting all other `let` variables that share (view) the same memory location.

To obtain a projection from a value currently in the `let` circuit, you must first transition into a circuit that supports exclusive ownership and mutation:
1. First, copy the `let` value into a `mut` or `mov` variable (creating a new, independent memory location).
2. Then, project from that new variable.
```rust
let x: i32 = 100;

// let proj p = &x; // ❌ ERROR: Cannot create a mutable projection from 
                    // an immutable 'let' source.

let mut tmp = x;    // 1. Implicit copy to gain independent ownership.
let proj p = &tmp;  // 2. Now you can project and mutate safely.
&p = 200;
```


#### The `mut` circuit

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Independent) | **Yes** (Mutable) | **Copy** (Independent) |

The `mut` circuit consists of mutable `mut` variables that retain ownership of their values.
A `mut` variable is a mutable value that can be modified after it is initialized. 
When a `mut` variable is assigned a value, it creates a new memory location to store that value.
The core principle of this circuit is _physical independence_: the compiler guarantees that every `mut` variable owns a disjoint block of memory.
While this circuit creates a clear boundary for mutation, it also needs to perform a several copies to maintain the value-isolated semantics, which can lead to performance overhead if not optimized properly.
However, modern optimizing compilers perform various optimizations to minimize the cost of the copies resulting from the `mut` circuit, such as
1. **Scalar Promotion**: for small, fixed-size types (like `i32`, `bool`, or `f64`), the copy is often just a single CPU register operation, such as a `mov` instruction. In these cases, the cost of an implicit copy is negligible and effectively $O(1)$ at the hardware level. 
2. **Copy Elision & SSA**: if the compiler's static analysis (often via Static Single Assignment form) determines that the source variable is never used again after the assignment, it can _elide_ the copy entirely. The destination variable simply takes over the existing memory location, effectively transforming the `mut`-to-`mut` assignment into a zero-cost move.
3. **Constant & Copy Propagation**: if a `mut` variable is assigned a value that is known at compile-time, or is a copy of another variable that hasn't changed, the compiler can propagate that value directly to all usage points, bypassing the need to allocate and copy physical memory locations until absolutely necessary for a mutation.
4. **Dead Store Elimination**: if a `mut` variable is copied but the destination is overwritten (e.g., `y = x; y = 10;`) before being read, the compiler will identify the first copy as a _dead store_ and remove it entirely from the generated machine code.


##### From `mut` to `mut` circuit

If a `mut` variable is assigned to another `mut` variable, they remain within the `mut` circuit, but the compiler must create a full copy of the value.
This ensures that both variables have their own disjoint memory locations. 
Since both are mutable, this _value-isolated_ semantic is what prevents shared mutable state: changing one variable will never side-effect the other.
For example, consider the following code:
```rust
let mut x: i32 = 42; 
let mut y = x; // ⚠️ O(N) Copy: 'y' is a fresh clone of 'x'

// Both exist independently in the mut circuit.
y = 100; 

// x: 42 (Unchanged)
// y: 100 (Independent)
```

##### From `mut` to `let` circuit

Assigning a `mut` variable to a `let` variable moves the value into the `let` circuit. This operation effectively _freezes_ the current state of the data, transforming a private, mutable resource into a shared, read-only one.
Depending on whether the original `mut` variable is used afterwards, the compiler applies two different strategies:
1. Freeze Optimization: if the source `mut` variable is no longer used after the assignment, the compiler performs a zero-cost $O(1)$ transfer. The memory location is simply re-labeled as immutable.
```rust
let mut x: i32 = 42;
let y: i32 = x; //'y' takes ownership of the value from 'x' without 
                // copying, since 'x' is not used afterwards.
// 'x' is not used after this point, so the compiler can optimize 
// the assignment by reusing the same memory location for 'y' without copying.
```
2. Implicit Copy: if the source `mut` variable continues to be used (and potentially mutated) after the assignment, the compiler must create a copy. This ensures that the new `let` variable remains a faithful _snapshot_ of the data at the moment of assignment, unaffected by future changes to the original `mut` variable.
```rust
let mut x: i32 = 42;
let y: i32 = x; // 'y' is a snapshot of 'x' at this point in time.
x = 100; // Mutating 'x' does not affect 'y'.
// x: 100 (Mutated)
// y: 42 (Snapshot, unchanged)
```

##### From `mut` to `mov` circuit

The transition from a `mut` variable to a `mov` variable is an _ownership promotion_ in the exclusive `mov` circuit.
This is always an $O(1)$ operation that freezes the current state of the data.
Since the `mut` variable already possessed its own independent memory location, the compiler simply transfers the _exclusive key_ of that memory to the new `mov` variable.
The original `mut` variable is immediately invalidated (consumed). 
This is crucial for performance: you can build a complex object in the `mut` circuit and then _freeze_ its exclusivity by moving it to the `mov` circuit without any reallocation.
For example, consider the following code:
```rust
let mut x: i32 = 42; // 'x' is a mut variable that owns the integer value.
&x += 1; // 'x' is mutated in place, now holds 43.
/* A thousand lines of complex logic that builds up 'x' to a final state... */
let mov y: i32 = x; // 'y' is a new mov variable that takes ownership 
                    // of the integer value from 'x' without copying.
// 'x' is no longer valid and cannot be used, while 'y' now owns the integer value.
```

##### From `mut` to `proj` circuit

Transitioning from a `mut` variable to a `proj` variable allows you to _borrow_ mutable access to a value without triggering the independent copy logic inherent to the `mut` circuit. While a `mut` variable owns its memory independently, a `proj` acts as a temporary, high-performance window into that memory.
This transition is an $O(1)$ operation that establishes a shadow. The original `mut` variable remains the owner but is temporarily _frozen_ (shadowed) until the projection is no longer in use.
The compiler can optimize this assignment by creating a reference to the original memory location without copying the value, since the `proj` variable can mutate the same memory location as the `mut` variable.
For example, consider the following code:
```rust
fn main() {
    let mut a: i32 = 1; // 'a' is a mut variable that owns 
                        // the integer value.

    // Create a projection 'p' that references the same 
    // memory location as 'a'. 
    let proj p = &a; 

    // While 'p' is active, 'a' is shadowed (locked).
    // let tmp: i32 = a; // ❌ ERROR: 'a' is currently shadowed by a projection.

    // We can mutate the original memory through the projection
    &p = 0; 

    // The latest use of 'p' determines when 'a' becomes valid again. 
    // After this point, 'a' can be used again.
}
```
This snippet could be reasonably rewritten without a `proj` variable, by directly mutating `a` in place: `&a = 0;`.


The utility of `proj` when starting from a `mut` source is twofold:
- In the `mut` circuit, passing a variable to a function or another scope might trigger a copy to preserve value independence. By using a projection, you explicitly tell the compiler: "Don't copy the data; let this secondary path modify the existing bytes directly."
```rust
fn do_something(proj x: i32) { &x += 20; }

fn main() {
    let mut x: i32 = 40; // 'x' is a mut variable that owns 
                         // the integer value.

    // Pass 'x' as a projection to 'do_something', allowing it 
    // to mutate the value in place without taking ownership.
    do_something(&x);

    // 'x' is still valid and can be used after the function call,
    // since the projection does not consume the original variable.
}
```
- When working with complex data structures (like structs or arrays), you often want to mutate specific fields without copying the entire structure. Projections allow you to create mutable references to individual fields, enabling efficient in-place updates while keeping the overall structure intact.
```rust
struct S { 
    pub mut first: i32, 
    pub mut second: i32 
}

fn do_something(let x: i32) { /* ... */ }

fn main() {
    let mut s: S = S::new(); // 's' is a mut variable that owns the struct.

    let proj f: i32 = &s.first;
    let proj s: i32 = &s.second;

    &f = 100;
    &s = 200;

    do_something(f);
    do_something(s);

    // 's' is still valid and can be used after the projections, 
    // since the projections do not consume the original variable.
}
```

#### The `mov` circuit

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **Yes** (Exclusive) | **Yes** (Mutable) | **Move** (Transfer) |

The `mov` circuit consists of mutable `mov` variables that have exclusive ownership of their values.
A `mov` variable is a mutable value that has exclusive ownership of its memory location. 
When a `mov` variable is assigned a value, it creates a new memory location within the `mov` circuit to store that value.
In general, a `mov` variable is not valid in scope after it has been moved to another variable or function, since it has exclusive ownership of its memory location and cannot be shared or modified after the transfer.

##### From `mov` to `mov` circuit

If a `mov` variable is assigned another `mov` variable it remains in the `mov` circuit, but the ownership of the value is transferred from the source variable to the destination variable.
The compiler can optimize this assignment by transferring ownership of the memory location from the source variable to the destination variable without copying the value, since the source variable will no longer be valid after the transfer. 
For example, consider the following code:
```rust
let mov x: i32 = 1; // 'x' is a mov variable that owns the 
                    // integer value.
let mov y: i32 = x; // 'y' is a new mov variable that takes 
                    // ownership of the integer value from 'x'.

// 'x' is no longer valid and cannot be used,  while 'y' 
// now owns the integer value and can be modified.
y = 10;

// let mov z: i32 = x; // ❌ ERROR: 'x' cannot be used 
                       // after being moved to 'y'.
```

##### From `mov` to `let` circuit

If a `mov` variable is assigned to a `let` variable it is moved to the `let` circuit, and the ownership of the value is transferred from the `mov` variable to the `let` variable.
The compiler can optimize this assignment by transferring ownership of the memory location from the `mov` variable to the `let` variable without copying the value, since the `mov` variable will no longer be valid after the transfer. 
Since the `let` variable is immutable, it cannot be modified after the transfer, and the `mov` variable is no longer valid in scope. 
The following example illustrates this behavior:
```rust
let mov x: i32 = 1; // 'x' is a mov variable that owns 
                    // the integer value.
let y: i32 = x; // 'y' is a new let variable that takes 
                // ownership of the integer value from 'x'.

// 'x' is no longer valid and cannot be used, while 'y' 
// now owns the integer value and cannot be modified.

// let mov z: i32 = x; // ❌ ERROR: 'x' cannot be used 
                       // after being moved to 'y'.
```

One might wonder why we would transfer ownership from a powerful mov variable to a restricted let variable. The reason lies in _contract enforcement_ and _parallelism_:
- In large systems, you often want to perform a series of high-speed mutations (in the `mov` circuit) and then _lock_ the result. By moving the value to a `let` variable, you guarantee that no one, not even the original logic, can accidentally modify the data after it has reached its _final_ state.
- A `mov` variable is exclusive and cannot be shared. By moving it into the `let` circuit, you transform a _hot_ exclusive resource into a _cold_ shared value. This allows the compiler to safely share the data across multiple threads or views, knowing it will never change again.

##### From `mov` to `mut` circuit

If a `mov` variable is assigned to a `mut` variable it is moved to the `mut` circuit, and the ownership of the value is transferred from the `mov` variable to the `mut` variable.
The compiler can optimize this assignment by transferring ownership of the memory location from the `mov` variable to the `mut` variable without copying the value, since the `mov` variable will no longer be valid after the transfer.
The following example illustrates this behavior:
```rust
let mov x: i32 = 1; // 'x' is a mov variable that owns 
                    // the integer value.
let mut y: i32 = x; // 'y' is a new mut variable that takes 
                    // ownership of the integer value from 'x'.

// 'x' is no longer valid and cannot be used, while 'y' 
// now owns the integer value and can be modified.

// let mov z: i32 = x; // ❌ ERROR: 'x' cannot be used 
                       // after being moved to 'y'.
```


Since the `mov` variable is already mutable, one may wonder why it is necessary to transfer ownership to a `mut` variable instead of keeping it in the `mov` circuit. The reason is that the `mut` circuit allows for a more flexible management of the variable's lifecycle within the local scope. While a `mov` variable is strictly tied to a _use-it-and-lose-it_ philosophy (where every assignment or pass-to-function terminates the original binding), a `mut` variable acts as a stable container. As mentioned in the previous section, once the value enters the `mut` circuit:
- The variable y can be passed to multiple functions as a view (`let`) or a projection (`proj`) without ever losing its name or its ability to be mutated further down in the code.

```rust
fn double_view(let v: i32, let w: i32) { /* ... */ }

fn main() {
    let mov x: i32 = 1; // 'x' is a mov variable that owns 
                        // the integer value.
    // double_view(x, x); // ❌ ERROR: 'x' cannot be passed as a 
                          // view to multiple functions, since it is 
                          // a mov variable and would lose its name after 
                          // the first pass.

    let mut y: i32 = x; // 'y' is a new mut variable that takes 
                        // ownership of the integer value from 'x'.

    double_view(y, y); // 'y' can be passed as a view to multiple 
                       // functions without losing its name or mutability.

    y = 10; // 'y' can still be mutated after being passed to functions.
}
```

- If the programmer performs an operation that would normally require a copy (like assigning y to another `mut` variable), the `mut` circuit allows the compiler to perform that copy to preserve both variables. In the `mov` circuit, this would simply be a compiler error.

```rust
fn main() {
    let mov a: i32 = 1; // 'a' is a mov variable that owns 
                        // the integer value.

    // Within the mov circuit, we cannot decouple the ownership 
    // of 'a' into multiple variables.
    // let mut x: i32 = a; 
    // let mut y: i32 = a; // ❌ ERROR: 'a' is no longer valid after 
                           // being moved to 'x'.

    let mut x: i32 = a; 
    let mut y = x; 

    x = 100; 
    y = 200; 
}
```

##### From `mov` to `proj` circuit

If a `mov` variable is assigned to a `proj` variable, it enters the `proj` circuit. Unlike the previous transitions, the `mov` variable is not consumed and does not lose ownership. Instead, it becomes temporarily shadowed (or locked) while the projection is active.
The compiler establishes a direct link to the original memory location. Through the `proj` variable, the data can be mutated in-place, but the `proj` variable itself does not own the memory; it is merely a loan that allows for efficient mutation without copying.
The compiler can optimize this assignment by creating a reference to the original memory location without copying the value, since the `proj` variable can mutate the same memory location as the `mov` variable.
For example, consider the following code:
```rust
fn main() {
    let mov a: i32 = 1; // 'a' is a mov variable that owns 
                        // the integer value.

    // Create a projection 'p' that references the same 
    // memory location as 'a'. 
    let proj p = &a; 

    // While 'p' is active, 'a' is shadowed (locked).
    // let tmp: i32 = a; // ❌ ERROR: 'a' is currently shadowed by a projection.

    // We can mutate the original memory through the projection
    &p = 0; 

    // The latest use of 'p' determines when 'a' becomes valid again. 
    // After this point, 'a' can be used again.
}
```
Note that, this snippet could be reasonably rewritten without a `proj` variable, by directly mutating `a` in place: `&a = 0;`. 

At this point, the question arises: why do we need a `proj` variable at all? Why not just keep the variable in the `mov` circuit and allow it to be mutated directly?
The reason lies in abstraction and modular mutation:
- By projecting individual fields of a structure, you can precisely define which part of a resource is being modified. Additionally, it allows you to mutate multiple times without losing the original variable's name or ownership, as long as the projections are active.
```rust
struct S { mut first: i32, mut second: i32 }

fn do_something(let x: i32) { /* ... */ }

fn main() {
    let mov s: S = S::new(); // 's' is a mov variable that owns the struct.

    let proj f: i32 = &s.first;
    let proj s: i32 = &s.second;

    &f = 100;
    &s = 200;

    do_something(f);
    do_something(s);

    // 's' is still valid and can be used after the projections, 
    // since the projections do not consume the original variable.
}
```
- `proj` is the primary way to allow functions to modify a `mov` resource without taking ownership of it. This allows the caller to retain the resource after the function has finished its work.
```rust
fn do_something(proj x: i32) { &x += 20; }

fn main() {
    let mov x: i32 = 40; // 'x' is a mov variable that owns 
                         // the integer value.

    // Pass 'x' as a projection to 'do_something', allowing it 
    // to mutate the value in place without taking ownership.
    do_something(&x);

    // 'x' is still valid and can be used after the function call,
    // since the projection does not consume the original variable.
}
```


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

#### The `proj` circuit

| Ownership | Mutability | Memory semantics |
| :--- | :--- | :--- |
| **No** (Loan) | **Yes** (In-place Mutation) | **Exclusive/Anchored** (Direct) |

A `proj` variable is a mutable value that does not have ownership of its memory location but can mutate it in place. 
When a `proj` variable is assigned a value, it creates a new memory location to store that value.
If a `proj` variable is assigned another `proj` variable, the compiler can optimize this assignment by creating a reference to the original memory location instead of copying the value, since both variables can mutate the same memory location. 
This allows for efficient mutation of values without copying and eliminates the possibility of shared mutable references, while still allowing for efficient memory usage.

Assigning a `proj` value to a `let` binding does not copy the underlying value. It means that, as long as the `proj` variable is valid, the `let` variable can access the same memory location through the projection. However, since the `let` variable is immutable, it cannot modify the value, and any attempt to do so would result in a compile-time error.

A `proj` variable is not valid in scope after it has been moved to another variable or function, since it does not have ownership of its memory location and cannot be shared or modified after the transfer.

The following example illustrates the behavior of `proj` variables in Eter's memory model:
```rust
let proj x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a proj variable that references the array.
let proj y: arr<i32>[3] = x; // 'y' is a new proj variable that references the same array as 'x'.
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
let proj x: arr<i32>[3] = arr<i32>[1, 2, 3]; // 'x' is a proj variable that references the array.
let proj y: arr<i32>[3] = x; // 'y' is a new proj variable that references the same array as 'x'.
let proj z: arr<i32>[3] = x; // ❌ ERROR: 'z' cannot reference the same array as 'x' and 'y' due to the Law of Exclusivity.
```
The trasparent pass-by-reference semantics of `proj` variables is shown in the following example:
```rust
fn increment(proj a: arr<i32>[3]) { /* Increment each element of the array in place */ }

let proj x: arr<i32>[3] = arr<i32>[1, 2, 3];
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
struct Point { 
    pub mut x: i32, 
    pub mut y: i32 
}

fn move_point(proj p: Point, proj target: i32) {}

fn main() {
    let mut my_point: Point = Point { x: 0, y: 0 };
    let mut offset: i32 = 5;
    move_point(&my_point, &offset); // OK: 'my_point' and 'offset' 
                                            // are distinct paths.
    // move_point(& my_point, & my_point.x); // ❌ ERROR: 'my_point' and 'my_point.x' overlap paths. 
                                             // The sub-tree 'my_point.x' is part of the path
                                             // 'my_point', so they cannot both be 'proj' in 
                                             // the same context.
}
```

#### Conservative Array Indexing

A specific restriction applies when a `proj` identifies an element within an array. The type system allows multiple `proj` bindings into the same array only if it can mathematically prove that the indices do not overlap.

**Static Certainty:** Accessing x[0] and x[1] via `proj` is permitted because the indices are distinct constants.

**Dynamic Uncertainty:** Accessing x[f(0)] and x[1] is prohibited. In this scenario, the type system is conservative: it assumes that f(0) could potentially evaluate to 1. Since it cannot guarantee exclusivity, it rejects the code to prevent a potential collision.

The following example illustrates this concept:
```rust
fn swap(proj a: i32, proj b: i32) { /* Swap the values of 'a' and 'b' in place */ }

fn main() {
    let mut x: arr<i32>[3] = [10, 20, 30];

    swap(proj x[0], proj x[1]); // OK: The compiler can statically verify that 'x[0]' and 'x[1]' are distinct paths, so it allows the swap.

    let i: usize = get_index(); // Assume 'get_index()' returns an integer at runtime.
    swap(proj x[i], proj x[1]); // ❌ ERROR: The compiler is CONSERVATIVE. 
                               // Because 'i' could be 1 at runtime, it assumes the paths 
                               // 'x[i]' and 'x[1]' MIGHT overlap. 
                               // To stay safe without a borrow checker, it rejects the call.
}
```

#### Ownership Transfer through Projections

As shown at the beginning of this chapter, a critical advantage of the MVS model over Rust's borrowing system is the ability to temporarily move a value out of a projection. In traditional reference systems, a mutable reference is a "look but don't take" contract: you can modify the data, but you cannot steal it.

In Eter, a `proj` binding is more flexible. 
It acknowledges that while a projection is active, the projector is the temporary owner of the value it references. 
This allows to "empty" the slot by moving the value (transferring it to a mov parameter) and then "refilling" it later.

Concretely, the following code is valid in Eter but would be illegal in Rust:
```rust
struct T {}
fn own_t(mov t: T) {} 

fn ref_mut_t(proj t: T) {
    own_t(t); // Move the value out of the projection, transferring ownership to 'own_t'.
    // 't' is now considered "moved" and cannot be used until it is re-initialized.
    t = T;    // Refill the projection with a new value after the move.
}
```

The constraints of the `proj` system ensure that this pattern is safe and does not lead to aliasing or shared mutable state, while still allowing for efficient mutation of values without copying.



### Interprocedural Value transfer semantics

| ↓ to \ from → | `let x = 𝓿;` | `let mut x = 𝓿;` | `let mov x = 𝓿;` | `let proj x = 𝓿;` |
|-|-|-|-|-|
| `fn_let(let x)` | TODO | TODO | TODO | TODO |
| `fn_mut(mut x)` | TODO | TODO | TODO | TODO |
| `fn_own(mov x)` | TODO | TODO | TODO | TODO |
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

