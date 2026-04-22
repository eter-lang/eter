# Type system

## Statically sized types

### Boolean type
### Numeric types
### Textual types
### Array types 

Arrays represent 1D homogeneous product types. They are statically sized, contiguous blocks of memory containing elements of a single type `T`. 

The syntax for an array type is `[T; SIZE]`.

```rust
let arr: [i32; 3] = [1, 2, 3];
```

### Tensor types 

Tensors represent multi-dimensional (nD) homogeneous product types. Like arrays, they are statically sized collections of a single type `T`, but structured across multiple dimensions.

The syntax for a tensor type separates the dimensions with commas: `[T; SIZE1, SIZE2, ...]`.

```rust
let matrix: [f32; 2, 2] = [[1.0, 0.0], [0.0, 1.0]];
```

> [!NOTE]  
> Compound types like arrays and tensors are not "objects". Dynamic, heap-allocated collections (such as vectors or dynamic tensors) are managed through the standard library or via gradual typing features, rather than being built into the static compound type syntax.

### Pointer types
### Union types
### Tuple types

Tuples are Cartesian product types. They are ordered, statically sized, heterogeneous collections of values where each element can be of a different type.

The syntax for a tuple type uses parentheses `()` containing a comma-separated list of types: `(T, U, V)`.

```rust
let record: (i32, f64, str) = (42, 3.14, "hello");
```

## Static types layout
 
## Dinamically sized types

---

### Struct types
### Enum types
### View types




