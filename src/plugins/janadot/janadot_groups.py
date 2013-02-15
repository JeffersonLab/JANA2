#!/usr/bin/env python

import sys
import os
import re
from time import gmtime, localtime, strftime
from os.path import join

if len(sys.argv) != 2:
	print ''
	print 'Usage:'
	print '      janadot_groups.py  /path/to/src'
	print ''
	print '  This script will scan a directory tree (starting at the specified'
	print 'location) for files with names like "*_factory*.h" using them to'
	print 'generate a configuration file suitable for use with the janadot'
	print 'plugin. (Note that the janadot plugin does not require this.)'
	print 'It uses the relative path and base name of the files it finds'
	print 'matching the pattern to generate groups that the janadot output'
	print 'will use to draw a bounding box around those factories. The idea'
	print 'is that the classes found together in a directory should be'
	print 'grouped together in the janadot display to help guide the user'
	print 'when viewing the call graph.'
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
	print 'If you wish to make certain bounding boxes specific colors'
	print 'then add a string like "color_red" to the data class list'
	print 'for that group. It can be anywhere in the list. The last'
	print 'part of the string (after "color_" will be used as the color'
	print 'of the bounding box. This can be done using any text editor'
	print 'on the produced "janadot_groups.conf" file.'
	print ''
	sys.exit(0);

topdir = os.path.abspath(sys.argv[1])

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
for gname in factory_files:
	line='JANADOT:GROUP:'+gname+' '
	for class_name in factory_files[gname]:
		line += class_name+","

	# Chop off last "," from line and add CR
	line = re.sub('\,$','\n', line)

	f.write(line)
f.close()



