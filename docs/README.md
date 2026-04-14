# The Eter Reference 

This folder contains the source of "The Eter Reference", the official reference documentation for the Eter programming language.

## Working Locally

The Eter reference is built using [mdBook](https://rust-lang.github.io/mdBook/).

To work on the Eter reference locally, you will need to install `mdbook` first. You can do this with `cargo`:
```bash
cargo install mdbook
```

Then, you can start the live-reloading docs server with:
```bash
mdbook serve --open
```
It will be available at `http://127.0.0.1:3000/`.
