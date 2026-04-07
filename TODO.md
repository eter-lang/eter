# TODO List

- AI-generated code policy
- subdirectory for lib folder
- Understand if we need to have two types of tensors: "CPU tensors" and "GPU tensors" or if we can unify them with a single Tensor type that has different storage backends.
- Can we write @gpu kernels syntactically as normal Rust functions, and have the compiler automatically generate the necessary GPU code and data movement? 