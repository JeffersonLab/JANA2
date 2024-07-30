# Generating documentation

Generating documentation

### Doxygen

Doxygen is a widely used documentation generation tool for C++ projects. 
It parses the source code and accompanying comments, formatted according to Doxygen's configurable markup, 
producing documentation in various output formats such as HTML, XML, and LaTeX.

```bash
# Assuming cwd is JANA2/ - repository root folder
cd docs
doxygen Doxyfile
```

The command will generate documentation in `doxygen_build` folder so then:

- The output is saved to `docs/doxygen_build`
- html web site: `docs/doxygen_build/html`
- xml: `docs/doxygen_build/xml`
- latex: `docs/doxygen_build/latex`

One can test the generated website: 

```bash
# assuming cwd is JANA2/docs 
python3 -m http.server -d doxygen_build/html/ 8000
```