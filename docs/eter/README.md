# The Eter Reference 

This folder contains the source of "The Eter Reference", the official reference documentation for the Eter programming language.

## Working Locally

The Eter reference is built using [MkDocs](https://www.mkdocs.org/).

To work on the Eter reference locally, you can use the following commands to set up a Python virtual environment and install the required dependencies:
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

Then, you can start the live-reloading docs server with:
```bash
mkdocs serve
```
It will be available at `http://127.0.0.1:8000/`.

--- 

Useful MkDocs commands include:

* `mkdocs new [dir-name]` - Create a new project.
* `mkdocs serve` - Start the live-reloading docs server.
* `mkdocs build` - Build the documentation site.
* `mkdocs -h` - Print help message and exit.