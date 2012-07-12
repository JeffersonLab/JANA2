#!/bin/tcsh -f

set savedir=$PWD

echo " "
echo " "
echo "=========== Copying source ========="
rm -rf source
mkdir -p source
set cmd="cp -r $JANA_HOME/include source"
echo $cmd
$cmd
cd source/include/JANA

# Remove lingering dictionary and LinkDef.h files
touch nada_Dict.nada nada_LinkDef.h # (avoid warnings)
rm -f *_Dict.* *LinkDef.h


echo " "
echo " "
echo "=========== Generating Dictionaries ========="
foreach f (`ls J*.h | grep -v JFactory.h`)
#foreach f (JException.h)
	echo Generating dictionary for $f ...
	set cmd="rootcint `basename $f .h`_Dict.C -c -p -I.. -I$JANA_HOME/include -DXERCES3=1 -I$XERCESCROOT/include $f"
	echo $cmd
	$cmd
end

echo " "
echo " "
echo "=========== Compiling executable ========="
set cmd="g++ `root-config --cflags --libs` -lHtml -lThread -I$JANA_HOME/include -I$XERCESCROOT/include -o gendoc *_Dict.C ../../../gendoc.cc -L$JANA_HOME/lib -lJANA -L$XERCESCROOT/lib -lxerces-c"
echo $cmd
$cmd

echo "=========== Running exectuable ========="
set cmd="./gendoc"
echo $cmd
$cmd

rm -rf ../../../htmldoc
mv htmldoc ../../../

cd $savedir
