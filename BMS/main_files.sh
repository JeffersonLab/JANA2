#!/bin/sh

grep -l " main(" *.c *.cc *.cpp *.cxx 2> /dev/null

grep -l " main (" *.c *.cc *.cpp *.cxx 2> /dev/null

grep -l -i "      PROGRAM " *.f *.F 2> /dev/null
