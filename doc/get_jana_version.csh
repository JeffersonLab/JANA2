#!/bin/tcsh -f 
#
# get_jana_version.csh
# July 1, 2011 David Lawrence
#


set major=`cat ../src/JANA/JVersion.h | & awk '/major = / {printf("%c\n",$3)}'`
set minor=`cat ../src/JANA/JVersion.h | & awk '/minor = / {printf("%c\n",$3)}'`
set build=`cat ../src/JANA/JVersion.h | & awk '/build = / {print $3}'`


echo ${major}.${minor}.${build}
