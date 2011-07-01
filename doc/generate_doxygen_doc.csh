#!/bin/tcsh -f

# Replace PROJECT_NUMER line in configuration file with
# jana version number.
set version=`../bin/jana -v | grep "JANA version:" | awk '{print $3}'`
rm -f Doyxfile.tmp
cat Doxyfile | sed -e 's/alpha/'${version}'/g' > Doxyfile.tmp

doxygen Doxyfile.tmp
cp images/* html
rm -f Doyxfile.tmp


echo " "
echo 'HTML documentation is in the directory "html"'
echo " "
echo " "
echo " To generate a PDF reference manual from the generated LaTex"
echo "source, do the following:"
echo " "
echo "make -C latex refman.pdf"
echo " "
echo "The manual will be latex/refman.pdf"
echo " "

