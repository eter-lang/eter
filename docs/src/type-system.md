# Type system

## Statically sized types

Statically sized types are those stored with static (immutable) and fixed amount of contiguous space. The amount of space occupied solely depends on the type of the variable and not the value represented.


### Boolean type
Booleans can represent one of two distinct values: `true` or `false`. Each boolean occupy 1 bit.

```rust
let rain: bool = true;
let sunny: bool = false;
```

### Numeric types
Numeric types represents numbers. A numeric type must be declared with a character (representing the type of number) and a number (representing the size of the variable)
```rust
let number : cXY = ... //c = the char of the type
                       //XY = the size to allocate
```
- #### Unsigned
Unsigned types represents absolute numbers (which, by convention are treated as positive numbers). Unsigned numbers are declared with the letter `u`. The following table shows the possible sizes.

| Type | Byte Size (bit) | Min Value | Max Value|
|-:|-:|:-:|:-|
| `u8` | 1 byte (8 bits) | $0$| $2^{8}-1$|
| `u16` | 2 bytes (16 bits) | $0$| $2^{16}-1$|
| `u32` | 4 bytes (32 bits) | $0$| $2^{32}-1$|
| `u64` | 8 bytes (64 bits)| $0$| $2^{64}-1$|
| `u128` | 16 bytes (128 bits) | $0$| $2^{128}-1$|
```rust
let x1 : u8 = 10;
let x2 : u8 = 300; // compiling error. The variable type has not enough size to represent the value
```

- #### Integer
Integer types represents integer numbers, both positive and negative. Integer numbers are declared with the letter `i`. The following table shows the possible sizes.
| Type | Byte Size (bit) | Min Value | Max Value|
|-:|-:|:-:|:-|
| `i8` | 1 byte (8 bits) | $-2^{7}$| $2^{7}-1$|
| `i16` | 2 bytes (16 bits) | $-2^{15}$| $2^{15}-1$|
| `i32` | 4 bytes (32 bits) | $-2^{31}$| $2^{31}-1$|
| `i64` | 8 bytes (64 bits)| $-2^{63}$| $2^{63}-1$|
| `i128` | 16 bytes (128 bits) | $-2^{127}$| $2^{127}-1$|

- #### Float
Float types represents numbers in floating point (thus numbers with decimal) both positive and negative following the [IEEE 754 standard](https://en.wikipedia.org/wiki/IEEE_754). Float numbers are declared with the letter `f`. The only possible sizes are `32` or `64`. Thus the two possible types are `f32` (single precision) and `f64` (double precision).
| Type | Byte Size (bit) | Min Value (Normalized) | Min Value (Subnormal)| Max Value|
|-:|-:|:-:|:-:|:-|
| `f32` | 4 byte (32 bits) | $1.0×2^{−126}$|$2^{−149}$| $(2−2^{−23})×2^{127}$|
| `f64` | 8 bytes (64 bits) | $1.0×2^{−1022}$|$2^{−1074}$| $(2−2^{−52})×2^{1023}$|

```rust
let x : f32 = 10.5;
let y : f64 = 10.5f;
```
As seen in the example both the declarations are valid.

 ### Character
A character represents a [Unicode scalar value](https://en.wikipedia.org/wiki/List_of_Unicode_characters). Characters are declared as `char` and the declared value must be enclosed in single quotes. Every character is stored in 4 bytes (32 bit), thus a sort of alias to `u32` type.
```rust
let c: char = 'a';
let emoji: char = '😀';
let unicode: char = '\u{1F600}';
```
### Pointer types 
  > [!NOTE]
  > This is an example note.

## Compound data types
The following types still have a static (immutable) amount of space allocated but it depends both on the types and the amount of values to store.

### Textual types
Textual types represents strings, thus sequences of characters.

 - #### Strings
 Strings are declared as `str`and the value enclosed in double quotes.

 ```rust
 let say : str = "Hello";
 ```

 - #### C-Strings
C-strings are Null-terminated string literal identified by the `@c` qualifier.

 ```rust
 let say : str = @c"Hello";
 ```
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

### Tuple types

Tuples are Cartesian product types. They are ordered, statically sized, heterogeneous collections of values where each element can be of a different type.

The syntax for a tuple type uses parentheses `()` containing a comma-separated list of types: `(T, U, V)`.

```rust
let record: (i32, f64, str) = (42, 3.14, "hello");
```
### Struct types
### Enum types

## Static types layout
 
## Dinamically sized types

---


### Union types
  > [!NOTE]
  > This is an example note.
  
Union types (also known as [sum types](https://en.wikipedia.org/wiki/Tagged_union)) are types that can store different types of values but only one at a time. Unions types are declared similar to structs but with the `union` kayword(comma separated). 
```rust
union MyUnion {
    f1: u32,
    f2: f32,
}
```
An example of two identically declared Unions with differently stored values.

### View types




