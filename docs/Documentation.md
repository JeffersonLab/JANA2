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

To add doxygen links and some other fine tuning of doxygen page look, 
header and footer files were generated. If doxygen will ever change the template, 
those probably has to be regenerated: 

```
doxygen -w html doxygen_header.html doxygen_footer.html doxygen_stylesheet.css
```

Add this to doxygen footer before `</body>` closing tag: 

```html
<!-- JANA2 custom footer addition -->
<script type="text/javascript">
  $(document).ready(function() {
      let navHeader = '<li><a href="https://github.com/JeffersonLab/JANA2" target="_blank">Project GitHUB</a></li>';
      // Append it to the navigation div or another appropriate place in the menu
      $('#main-menu').append(navHeader);
  });
</script>
<!-- END JANA2 custom footer addition -->
```



### Docsify

The main documentation is powered by the Docsify JavaScript library https://docsify.js.org/#/

[Available plugins](https://docsify-theme-github.vercel.app/#/awesome?id=plugins):

- Example panels [github](https://github.com/VagnerDomingues/docsify-example-panels) [demo](https://vagnerdomingues.github.io/docsify-example-panels/#/)
- Docsify Fontawesome [github](https://github.com/erickjx/docsify-fontawesome) [Fontawesome](https://fontawesome.com/)
- Select documentation version [github](https://github.com/UliGall/docsify-versioned-plugin)
- Docsify themebable [github](https://jhildenbiddle.github.io/docsify-themeable/#/)
- Theme switcher [github](https://github.com/
- Marked is used as markdown [Marked](https://github.com/markedjs/marked)
