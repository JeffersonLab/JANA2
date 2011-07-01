
#
# This makefile will attempt to generate documentation for JANA
# using both Doxygen and ROOT's THTML facility. These are done
# using the respective scripts in this directoy. The resulting
# files are packaged up in a tarball to make it easy to transport
# and install on the web
#
#

version=$(shell get_jana_version.csh)
jana_doc_dir=jana_doc_$(version)

all:
	generate_root_doc.csh
	generate_doxygen_doc.csh
	make -C latex refman.pdf
	rm -rf $(jana_doc_dir)
	mkdir $(jana_doc_dir)
	cp latex/refman.pdf $(jana_doc_dir)
	cp -rp html $(jana_doc_dir)
	cp -rp htmldoc $(jana_doc_dir)
	cp -rp man $(jana_doc_dir)
	cat index.html.tmpl | sed -e 's/JANAVERSION/$(version)/g' > $(jana_doc_dir)/index.html
	tar czf $(jana_doc_dir).tgz $(jana_doc_dir)
	@echo ""
	@echo "Copy the file $(jana_doc_dir).tgz to the webserver and unpack it."
	@echo "Make sure something links to $(jana_doc_dir)/index.html"
	@echo
	
clean:
	rm -rf Doxyfile.tmp errors.log html htmldoc $(jana_doc_dir) $(jana_doc_dir).tgz source latex man 

