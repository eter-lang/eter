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
In this example, the `ref_mut_t` function attempts to move the value `t` out of the mutable reference, which is not allowed in Rust. This is because Rust's ownership system does not allow dereferencing a mutable reference to move the value it points to, as it would violate Rust's guarantees about memory safety. To work around this, you would need to clone the value or use some other mechanism to transfer ownership, which can introduce overhead and complexity.

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


## Eter's Memory Model

Eter embraces the MVS as a core part of its memory model.
In Eter, all values are truly independent and can be mutated in place without copying, while the compiler statically guarantees that there are no shared mutable references. 
This eliminates the need for a GC or ARC and allows for efficient memory usage while maintaining the benefits of value semantics.

### Immutability

### Mutability

### Ownership

## Use of Pointers (unsafe scopes)
 - `let ptr<i32>` 
 - `let mut ptr<i32>`
 - `let ptr<mut i32>`
 - `let mut ptr<mut i32>` 


[^1]: We refer the reader to the following resources: (i) The research section of the [hylo-lang website](https://hylo-lang.org/docs/contributing/research-collab/) for a comprehensive list of papers related to MVS, (ii) the [Hylo language specification](https://hylo-lang.org/docs/reference/specification/), (iii) the [Utilizing value semantics in Swift](https://www.swiftbysundell.com/articles/utilizing-value-semantics-in-swift/) post, and (iv) the [Ruminating about mutable value semantics](https://www.scattered-thoughts.net/writing/ruminating-about-mutable-value-semantics/) post.


[Mutable Value Semantics]: https://kyouko-taiga.github.io/assets/papers/jot2022-mvs.pdf
[Hylo]: https://hylo-lang.org/
[Swift]: https://swift.org/

