C++ project, **Doxygen + Graphviz + LaTeX** can generate HTML documentation and a PDF.

### 1. Install the tools

On Debian:

```bash
sudo apt update
sudo apt install doxygen graphviz texlive-latex-base texlive-latex-extra
```

Check:

```bash
doxygen --version
dot -V
pdflatex --version
```

### 2. Create a Doxygen configuration

From your project directory:

```bash
cd ~/Desktop/MY_GIT/File_Extension
doxygen -g Doxyfile
```

This creates:

```text
File_Extension/
├── Doxyfile
├── include/
├── src/
├── Makefile
└── ...
```

### 3. Configure `Doxyfile`

The important settings are:

```text
PROJECT_NAME           = "File Extension"
OUTPUT_DIRECTORY       = docs

INPUT                  = include src
RECURSIVE              = YES

FILE_PATTERNS          = *.h *.hpp *.cpp

EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
EXTRACT_STATIC         = YES

GENERATE_HTML          = YES
GENERATE_LATEX         = YES

HAVE_DOT               = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES
CLASS_DIAGRAMS         = YES
```

For a C++ library, I also recommend:

```text
QUIET                  = NO
WARN_IF_UNDOCUMENTED   = NO
WARN_IF_DOC_ERROR      = YES
```

### 4. Generate documentation

Run:

```bash
doxygen Doxyfile
```

You should get:

```text
docs/
├── html/
└── latex/
```

Open the HTML documentation:

```bash
firefox docs/html/index.html
```

### 5. Generate the PDF

Go into the generated LaTeX directory:

```bash
cd docs/latex
make
```

Then:

```bash
ls -lh refman.pdf
```

Your PDF will be:

```text
docs/latex/refman.pdf
```

Open it:

```bash
xdg-open refman.pdf
```

---

### One-command version

You can add this to your existing `Makefile`:

```makefile
docs:
  doxygen Doxyfile

docs-pdf: docs
  $(MAKE) -C docs/latex

docs-clean:
  rm -rf docs

.PHONY: docs docs-pdf docs-clean
```

Then:

```bash
make docs-pdf
```

will generate:

```text
docs/
├── html/
│   └── index.html
└── latex/
    └── refman.pdf
```

**One important point:** Doxygen can document your existing code automatically, but the quality of the documentation becomes much better if your classes/functions have `///` or `/** ... */` comments.

For example:

```cpp
/**
 * @brief Exports tick data to a CSV file.
 *
 * Writes timestamp and price information using the
 * configured separator and decimal precision.
 */
class CsvExporter {
    ...
};
```
