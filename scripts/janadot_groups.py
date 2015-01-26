#!/usr/bin/env python

import sys
import getopt
import os
import re
from time import gmtime, localtime, strftime
from os.path import join


#-----------------------
# Usage
#-----------------------
def Usage():
	print ''
	print 'Usage:'
	print '      janadot_groups.py  [-hcb] /path/to/src'
	print ''
	print '  This script will scan a directory tree (starting at the specified'
	print 'location) for files with names like "*_factory*.h" using them to'
	print 'generate a configuration file suitable for use with the janadot'
	print 'plugin. (Note that the janadot plugin does not require this.)'
	print 'The relative path and base name of the files it finds are used'
	print 'to match the pattern to generate groups that the janadot output'
	print 'will use to draw a bounding box around those factories. The idea'
	print 'is that the classes found together in a directory should be'
	print 'grouped together in the janadot display to help guide the user'
	print 'when viewing the call graph.'
	print ''
	print 'Options:'
	print ''
	print '   -h    Print usage statement'
	print '   -c    Do not assign colors to the classes in a group'
	print '   -b    Draw bounding box around classes in a group'
	print ''
	print 'File names must follow a naming convention like this:'
	print ''
	print 'DataClassName_factory.h'
	print ''
	print 'or'
	print ''
	print 'DataClassName_factory_Tag.h'
	print ''
	print 'The second example being for a tagged factory using the tag "Tag".'
	print 'Once the configuration file is generated, use it with janadot like'
	print 'this:'
	print ''
	print 'jana -PPLUGINS=janadot --config=janadot_groups.conf ...'
	print ''
	print 'If you wish different colors for the groups then either this'
	print 'script or the produced janadot.conf file must be edited by hand.'
	print 'Similarly if one wanted only certain groups to be drawn together'
	print 'inside a bounding box.'
	print ''
	sys.exit(0);


topdir = ''
DRAW_BOUNDING_BOX = False
ASSIGN_COLORS = True

# Parse argument list for command line switches
for arg in sys.argv[1:]:
	if arg in ('-h', '--help'):
		Usage()
	elif arg in ('-b','--bounding_box'):
		DRAW_BOUNDING_BOX = True
	elif arg in ('-c','--no_colors'):
		ASSIGN_COLORS = False
	elif (arg[0]!='-'):
		topdir=arg

if len(topdir) == 0:
	print '\n\nYou must supply a starting directory!'
	Usage()


# Box colors
colors = ['red','cadetblue1','lavender','yellow','magenta','salmon','slateblue1','cornsilk','gold','plum','pink','orchid']

# Search directory tree
factory_files = {}
p = re.compile(".*_factory.*\.h$")
for root, dirs, files in os.walk(topdir):

	# Loop over files
	tot_size = 0
	for file in files:
		# Only keep headers that match the factory class naming convention
		if (p.match(file)):
			# Remove the root directory from the path leaving only next level directory
			sroot = re.sub(topdir+'/*','',root)
			sroot = re.sub('/$','',sroot)

			# Remove ".h" from filename
			sfile =re.sub('\.h','',file)

			# Replace "_factory_" with ":" in case it is a tagged factory
			sfile =re.sub('_factory_',':',sfile)

			# Erase "_factory in case it is an untagged factory
			sfile =re.sub('_factory','',sfile)

			# If last character is ":" then remove it since factory is untagged
			sfile =re.sub(':$','',sfile)
			
			# This guarantees an entry for this key exists in the dictionary
			# without this line, the scripts errors on the line after this
			factory_files.setdefault(sroot, [])

			factory_files[sroot].append(sfile)
			print "root="+sroot+"  sfile="+sfile+"  file="+file

# Open output file
f = open('janadot_groups.conf', 'w')
f.write('#\n')
f.write('# This file generated automatically by the janadot_groups.py script\n')
f.write('# '+strftime("%a, %d %b %Y %H:%M:%S", localtime())+'\n')
f.write('#\n')
f.write('\n')

# Loop over library names, creating a JANADOT:GROUP: for each
icolor=0
for gname in factory_files:
	line='JANADOT:GROUP:'+gname+' '
	
	# Add color definition
	if(ASSIGN_COLORS):
		icolor += 1
		if(icolor >= len(colors)): icolor = 0
		line+='color_'+colors[icolor]+','

	# Set 'no_box' option if specified
	if(not(DRAW_BOUNDING_BOX)):
		line+='no_box,'

	# Add class name list
	for class_name in factory_files[gname]:
		line += class_name+","

	# Chop off last "," from line and add CR
	line = re.sub('\,$','\n', line)

	f.write(line)
f.close()


