
import os
import subprocess
import SCons
import glob
import re
import sys

#===========================================================
# The first 4 functions provide for building a library,
# program, multiple-programs, or plugin from all the source
# in the current directory.
#
# The next section contains useful utility functions.
#
# The functions that follow in the final section add support
# for various packages (e.g. ROOT, Xerces, ...)
#===========================================================


##################################
# library
##################################
def library(env, libname=''):

	# Library name comes from directory name
	if libname=='':
		libname = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++, and FORTRAN targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	sources = env['ALL_SOURCES']

	# Build static library from all source
	myobjs = env.Object(sources)
	mylib = env.Library(target = libname, source = myobjs)

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, mylib])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for library and headers
		installdir = env.subst('$INSTALLDIR')
		includedir = "%s/%s" %(env.subst('$INCDIR'), libname)
		libdir = env.subst('$LIBDIR')

		# Install targets 
		env.Install(libdir, mylib)
		env.Install(includedir, env.Glob('*.h*'))


##################################
# executable
##################################
def executable(env, exename=''):

	# Executable name comes from directory name
	if exename=='':
		exename = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++, and FORTRAN targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	# Push commonly used libraries to end of list
	ReorderCommonLibraries(env)

	sources = env['ALL_SOURCES']

	# Build program from all source
	myobjs = env.Object(sources)
	myexe = env.Program(target = exename, source = myobjs)

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, myexe])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for executable and headers
		installdir = env.subst('$INSTALLDIR')
		includedir = env.subst('$INCDIR')
		bindir = env.subst('$BINDIR')

		# Install targets 
		env.Install(bindir, myexe)


##################################
# executables
##################################
def executables(env):

	# This will generate multiple executables from the
	# source in the current directory. It does this
	# by identifying source files that define "main()"
	# and linking those with all source files that do not
	# define "main()". Program names are based on the 
	# filename of the source file defining "main()"
	main_sources = []
	common_sources = []
	curpath = os.getcwd()
	srcpath = env.Dir('.').srcnode().abspath
	os.chdir(srcpath)
	files = glob.glob('*.c') + glob.glob('*.cc') + glob.glob('*.cpp')
	for f in files:
		if 'main(' in open(f).read():
			main_sources.append(f)
		else:
			common_sources.append(f)

	for f in glob.glob('*.F'):
		if '      PROGRAM ' in open(f).read():
			main_sources.append(f)
		else:
			common_sources.append(f)
	os.chdir(curpath)
	
	env.PrependUnique(CPPPATH = ['.'])

	# Push commonly used libraries to end of list
	ReorderCommonLibraries(env)

	common_sources.extend(env['ALL_SOURCES'])

	# Build program from all source
	main_objs = env.Object(main_sources)
	common_objs = env.Object(common_sources)

	progs = []
	for obj in main_objs:
		exename = re.sub('\.o$', '', str(obj))  # strip off ".o" from object file name
		progs.append(env.Program(target = exename, source = [obj, common_objs]))

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([common_objs, main_objs, progs])
	else:
		# We're in launch directory (or descendent) schedule installation
		bindir = env.subst('$BINDIR')
		env.Install(bindir, progs)


##################################
# plugin
##################################
def plugin(env, pluginname=''):

	# Library name comes from directory name
	if pluginname=='':
		pluginname = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++ targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	sources = env['ALL_SOURCES']

	# Build static library from all source
	myobjs = env.SharedObject(sources)
	myplugin = env.SharedLibrary(target = pluginname, source = myobjs, SHLIBPREFIX='', SHLIBSUFFIX='.so')

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, myplugin])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for plugin and headers
		installdir = env.subst('$INSTALLDIR')
		includedir = "%s/%s" %(env.subst('$INCDIR'), pluginname)
		pluginsdir = env.subst('$PLUGINSDIR')

		# Install targets 
		installed = env.Install(pluginsdir, myplugin)
		env.Install(includedir, env.Glob('*.h*'))



#===========================================================
# Misc utility routines for the SBMS system
#===========================================================

##################################
# AddCompileFlags
##################################
def AddCompileFlags(env, allflags):

	# The allflags parameter should be a string containing all
	# of the link flags (e.g. what is returned by root-config --cflags)
	# It is split on white space and the parameters sorted into
	# the 2 lists: ccflags, cpppath

	ccflags = []
	cpppath = []
	for f in allflags.split():
		if f.startswith('-I'):
			cpppath.append(f[2:])
		else:
			ccflags.append(f)
	
	if len(ccflags)>0 :
		env.AppendUnique(CCFLAGS=ccflags)

	if len(cpppath)>0 :
		env.AppendUnique(CPPPATH=cpppath)


##################################
# AddLinkFlags
##################################
def AddLinkFlags(env, allflags):

	# The allflags parameter should be a string containing all
	# of the link flags (e.g. what is returned by root-config --glibs)
	# It is split on white space and the parameters sorted into
	# the 3 lists: linkflags, libpath, and libs

	linkflags = []
	libpath = []
	libs = []
	for f in allflags.split():
		if f.startswith('-L'):
			libpath.append(f[2:])
		elif f.startswith('-l'):
			libs.append(f[2:])
		else:
			linkflags.append(f)

	if len(linkflags)>0 :
		env.AppendUnique(LINKFLAGS=linkflags)

	if len(libpath)>0 :
		env.AppendUnique(LIBPATH=libpath)
		
	if len(libs)>0 :
		env.AppendUnique(LIBS=libs)


##################################
# ReorderCommonLibraries
##################################
def ReorderCommonLibraries(env):

	# Some common libraries are often added by multiple packages 
	# (e.g. libz is one that many packages use). The gcc4.8.0
	# compiler that comes with Ubuntu13.10 seems particularly
	# sensitive to the ordering of the libraries. This means if
	# one package "AppendUnique"s the "z" library, it may appear
	# too early in the link command for another library that needs
	# it, even though the second library tries appending it at the
	# end. This routine looks for some commonly used libraries 
	# in the LIBS variable of the given environment and moves them
	# to the end of the list.

	# If LIBS is not set or is a simple string, return now
	if type(env['LIBS']) is not list: return

	# If any of the following are in LIBS, they will be moved
	# to the back of LIBS maintaining the order in this list
	libs = ['ccdb', 'mysql', 'xerces-c','z', 'bz2', 'pthread', 'm', 'dl']
	for lib in libs:
		if lib in env['LIBS']:
			env['LIBS'].remove(lib)
			env.Append(LIBS=[lib])


##################################
# ApplyPlatformSpecificSettings
##################################
def ApplyPlatformSpecificSettings(env, platform):

	# Look for SBMS file based on this platform and run the InitENV
	# function in it to allow for platform-specific settings. Normally,
	# the BMS_OSNAME will be passed in which almost certainly contains
	# "."s. The Python module loader doesn't like these and we have to
	# replace them with "-"s to appease it.

	platform = re.sub('\.', '-', platform)

	modname = "sbms_%s" % platform
	if (int(env['SHOWBUILD']) > 0):
		print "looking for %s.py" % modname
	try:
		InitENV = getattr(__import__(modname), "InitENV")

		# Run the InitENV function (if found)
		if(InitENV != None):
			print "sbms : Applying settings for platform %s" % platform
			InitENV(env)

	except ImportError,e:
		if (int(env['SHOWBUILD']) > 0): print "%s" % e
		pass


##################################
# OptionallyBuild
##################################
def OptionallyBuild(env, dirs):

	# This is used to add directories that are not built as
	# part of the standard build, but can still be added
	# to the dependency tree so that the user can build them
	# by either invoking scons from within the specific source
	# directory or by specifying it on the command line.
	#
	# 

	subdirs = []
	for dir in dirs:
		add_dir = False
		if env.GetLaunchDir().endswith(dir): add_dir = True
		#if dir in env['COMMAND_LINE_TARGETS']: add_dir = True
		for target in env['COMMAND_LINE_TARGETS']:
			if target.endswith(dir): add_dir = True
		
		if add_dir : subdirs.extend([dir])

	if len(subdirs)>0 : env.SConscript(dirs=subdirs, exports='env', duplicate=0)


##################################
# TestCompile
##################################
def TestCompile(env, name, includes, content, options):

	# This provides an autoconf-like method to test compilation
	# of a C++ program to see which arguments are needed to get it
	# to compile and link. The arguments are:
	# env       - build environment
	# name      - name of test (used to make unique filenames)
	# includes  - list of header files to be #included in test program
	# content   - content of test program (n.b. this is what gets placed
	#             inside of "main()" and before the return statement)
	# options   - list of different argument lists that should be tried
	#             to see which results in a successful compilation/link.
	#             The first to succeed is returned (as a list, not a single 
	#             string). If none succeed, then a Python "None" value is
	#             returned. Note that each element of the list is itself a
	#             string that may contain many arguments, separated by spaces.
	#
	# n.b. if either the m32 or m64 flags are set by the user 
	# via the command line then "-m32" or "-m64" are added to the 
	# compile command. Otherwise, nothing is added and the default
	# bitness is used.
	ifname = '%s' % env.File('.%s_%s.cc' % (env['OSNAME'], name))
	ofname = '%s' % env.File('.%s_%s' % (env['OSNAME'], name))
	f = open(ifname, 'w')
	for header in includes: f.write('#include<%s>\n' % header)
	f.write('int main(int n, char*argv[]){%s;return 0;}\n' % content)
	f.close();
	args = [env['CXX'], '-o', ofname]
	if (env['BITNESS32']!=0) : args.append('-m32')
	if (env['BITNESS64']!=0) : args.append('-m64')
	args.append(ifname)
	ret = None
	for opt in options:
		myargs = opt.split()
		if(env['SHOWBUILD'] >0):
			print 'Test compiling %s:' % name
			print args + myargs
		res = subprocess.call(args + myargs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		if res==0:
			if(env['SHOWBUILD'] >0): print '---Succeeded'
			ret = myargs
			break
		else:
			if(env['SHOWBUILD'] >1):
				print '----Failed. Test file content was:------'
				print subprocess.call(['cat', ifname])
				print '----------------------------------------'
				
	
	if os.path.exists(ifname): os.unlink(ifname);
	if os.path.exists(ofname): os.unlink(ofname);
	return ret

	

#===========================================================
# Package support follows
#===========================================================

##################################
# pthread
##################################
def Add_pthread(env):
	includes = ['pthread.h']
	content = 'pthread_create(NULL, NULL, NULL, NULL);'
	if(TestCompile(env, 'pthread', includes, content, ['']) == None):
		if(TestCompile(env, 'pthread', includes, content, ['-pthread']) != None):
			env.AppendUnique(LINKFLAGS=['-pthread'])
		else:
			if(TestCompile(env, 'pthread', includes, content, ['-lpthread']) != None):
				env.AppendUnique(LIBS=['pthread'])
			


##################################
# JANA
##################################
def AddJANA(env):
	AddXERCES(env)
	AddCCDB(env)
	env.AppendUnique(LIBS=['JANA','dl'])


##################################
# JANAInstalled (use an already installed jana-config file)
##################################
def AddJANAInstalled(env):
	jana_home = os.getenv('JANA_HOME')
	if(jana_home != None):
		JANA_CFLAGS = subprocess.Popen(["%s/bin/jana-config" % jana_home, "--cflags"], stdout=subprocess.PIPE).communicate()[0]
		JANA_LINKFLAGS = subprocess.Popen(["%s/bin/jana-config" % jana_home, "--libs"], stdout=subprocess.PIPE).communicate()[0]

		AddCompileFlags(env, JANA_CFLAGS)
		AddLinkFlags(env, JANA_LINKFLAGS)


##################################
# HDDS
##################################
def AddHDDS(env):
	hdds_home = os.getenv('HDDS_HOME', 'hdds')
	env.AppendUnique(CPPPATH = ["%s/src" % hdds_home])
	env.AppendUnique(LIBPATH = ["%s/lib/%s" % (hdds_home, env['OSNAME'])])


##################################
# HDDM
##################################
def AddHDDM(env):
	env.AppendUnique(LIBS = 'HDDM')


##################################
# EVIO
##################################
def AddEVIO(env):
	evioroot = os.getenv('EVIOROOT', 'evio')
	env.AppendUnique(CPPPATH = ['%s/include' % evioroot])
	env.AppendUnique(LIBPATH = ['%s/lib' % evioroot])
	env.AppendUnique(LIBS=['evioxx', 'evio'])
	AddET(env)


##################################
# ET
##################################
def AddET(env):

	# Only add ET if ETROOT is set
	etroot = os.getenv('ETROOT', 'none')
	if(etroot != 'none') :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_ET'])
		env.AppendUnique(CPPPATH = ['%s/include' % etroot])
		env.AppendUnique(LIBPATH = ['%s/lib' % etroot])
		env.AppendUnique(LIBS=['et_remote', 'et'])


##################################
# CMSG
##################################
def AddCMSG(env):

	# Only add cMsg if CMSGROOT is set
	cmsgroot = os.getenv('CMSGROOT', 'none')
	if(cmsgroot != 'none') :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_CMSG'])
		env.AppendUnique(CPPPATH = ['%s/include' % cmsgroot])
		env.AppendUnique(LIBPATH = ['%s/lib' % cmsgroot])
		env.AppendUnique(LIBS=['cmsgxx', 'cmsg', 'cmsgRegex'])


##################################
# xstream
##################################
def Add_xstream(env):
	env.AppendUnique(CPPPATH = ['#external/xstream/include'])
	env.AppendUnique(CCFLAGS = ['-fPIC'])
	env.AppendUnique(LIBS=['xstream', 'bz2', 'z'])


##################################
# CCDB
##################################
def AddCCDB(env):
	ccdb_home = os.getenv('CCDB_HOME', 'none')
	if(ccdb_home != 'none'):
		CCDB_CPPPATH = "%s/include" % (ccdb_home)
		CCDB_LIBPATH = "%s/lib" % (ccdb_home)
		CCDB_LIBS = "ccdb"
		env.AppendUnique(CPPPATH = [CCDB_CPPPATH])
		env.AppendUnique(LIBPATH = [CCDB_LIBPATH])
		env.AppendUnique(LIBS    = [CCDB_LIBS])


##################################
# Xerces
##################################
def AddXERCES(env):
	# This relies on sbms_config.py::mk_jana_config_h having been run
	try:  # (need try block in case HAVE_XERCES is not defined in env)
		if env['HAVE_XERCES']==1:
			XERCES_LIBS = "xerces-c"
			env.AppendUnique(LIBS    = [XERCES_LIBS])
			xercescroot = os.getenv('XERCESCROOT')
			if(xercescroot != None):
				XERCES_CPPPATH = "%s/include" % (xercescroot)
				XERCES_LIBPATH = "%s/lib" % (xercescroot)
				env.AppendUnique(CPPPATH = [XERCES_CPPPATH])
				env.AppendUnique(LIBPATH = [XERCES_LIBPATH])
	except:
		env['HAVE_XERCES']=0  # would usually get here if env['HAVE_XERCES'] is not defined


##################################
# CERNLIB
##################################
def AddCERNLIB(env):
	env.PrependUnique(FORTRANFLAGS = ['-ffixed-line-length-0', '-fno-second-underscore'])
	env.PrependUnique(FORTRANFLAGS = ['-fno-automatic'])
	env.PrependUnique(FORTRANPATH = ['include'])
	cern = os.getenv('CERN', '/usr/local/cern/PRO')
	cern_level = os.getenv('CERN_LEVEL', '2006')
	cern_root = '%s/%s' % (cern, cern_level)
	CERN_FORTRANPATH = "%s/include" % cern_root
	CERN_LIBPATH = "%s/lib" % cern_root
	env.AppendUnique(FORTRANPATH   = [CERN_FORTRANPATH])
	env.AppendUnique(CPPPATH   = CERN_FORTRANPATH)
	env.AppendUnique(LIBPATH   = CERN_LIBPATH)
	env.AppendUnique(LINKFLAGS = ['-rdynamic'])
	env.AppendUnique(LIBS      = ['gfortran', 'geant321', 'pawlib', 'lapack3', 'blas', 'graflib', 'grafX11', 'packlib', 'mathlib', 'kernlib', 'X11', 'nsl', 'crypt', 'dl'])
	env.SetOption('warn', 'no-fortran-cxx-mix')  # supress warnings about linking fortran with c++


##################################
# ROOT
##################################
def AddROOT(env):
	#
	# Here we use the root-config program to give us the compiler
	# and linker options needed for ROOT. We use the AddCompileFlags()
	# and AddLinkFlags() routines (defined below) to split the arguments
	# into the categories scons wants. E.g. scons wants to know the
	# search path and basenames for libraries rather than just giving it
	# the full compiler options like "-L/path/to/lib -lmylib".
	#
	# We also create a builder for ROOT dictionaries and add targets to
	# build dictionaries for any headers with "ClassDef" in them.

	rootsys = os.getenv('ROOTSYS')
	if rootsys != None:

		ROOT_CFLAGS = subprocess.Popen(["%s/bin/root-config" % rootsys, "--cflags"], stdout=subprocess.PIPE).communicate()[0]
		ROOT_LINKFLAGS = subprocess.Popen(["%s/bin/root-config" % rootsys, "--glibs"], stdout=subprocess.PIPE).communicate()[0]
		AddCompileFlags(env, ROOT_CFLAGS)
		AddLinkFlags(env, ROOT_LINKFLAGS)
		env.AppendUnique(LIBS = "Geom")
		if os.getenv('LD_LIBRARY_PATH'  ) != None : env.Append(LD_LIBRARY_PATH   = os.environ['LD_LIBRARY_PATH'  ])
		if os.getenv('DYLD_LIBRARY_PATH') != None : env.Append(DYLD_LIBRARY_PATH = os.environ['DYLD_LIBRARY_PATH'])

		# NOTE on (DY)LD_LIBRARY_PATH :
		# Linux (and most unixes) use LD_LIBRARY_PATH while Mac OS X uses
		# DYLD_LIBRARY_PATH. Unfortunately, the "thisroot.csh" script distributed
		# with ROOT sets both of these so we can't use the presence of the
		# DYLD_LIBRARY_PATH environment variable to decide which of these to 
		# work with. Thus, we just append to whichever are set, which may be both.

		# Create Builder that can convert .h file into _Dict.cc file
		if os.getenv('LD_LIBRARY_PATH'  ) != None : env.AppendENVPath('LD_LIBRARY_PATH'  , '%s/lib' % rootsys )
		if os.getenv('DYLD_LIBRARY_PATH') != None : env.AppendENVPath('DYLD_LIBRARY_PATH', '%s/lib' % rootsys )
		rootcintpath  = "%s/bin/rootcint" % (rootsys)
		rootclingpath = "%s/bin/rootcling" % (rootsys)
		if env['SHOWBUILD']==0:
			rootcintaction  = SCons.Script.Action("%s -f $TARGET -c $SOURCE" % (rootcintpath) , 'ROOTCINT   [$SOURCE]')
			rootclingaction = SCons.Script.Action("%s -f $TARGET -c $SOURCE" % (rootclingpath), 'ROOTCLING  [$SOURCE]')
		else:
			rootcintaction  = SCons.Script.Action("%s -f $TARGET -c $SOURCE" % (rootcintpath) )
			rootclingaction = SCons.Script.Action("%s -f $TARGET -c $SOURCE" % (rootclingpath))
		if os.path.exists(rootclingpath) :
			bld = SCons.Script.Builder(action = rootclingaction, suffix='_Dict.cc', src_suffix='.h')
		elif os.path.exists(rootcintpath):
			bld = SCons.Script.Builder(action = rootcintaction, suffix='_Dict.cc', src_suffix='.h')
		else:
			print 'Neither rootcint nor rootcling exists. Unable to create ROOT dictionaries if any encountered.'
			return

		env.Append(BUILDERS = {'ROOTDict' : bld})

		# Generate ROOT dictionary file targets for each header
		# containing "ClassDef"
		#
		# n.b. It seems if scons is run when the build directory doesn't exist,
		# then the cwd is set to the source directory. Otherwise, it is the
		# build directory. Since the headers will only exist in the source
		# directory, we must temporarily cd into that to look for headers that
		# we wish to generate dictionaries for. (This took a long time to figure
		# out!)
		curpath = os.getcwd()
		srcpath = env.Dir('.').srcnode().abspath
		if(int(env['SHOWBUILD'])>1):
			print "---- Scanning for headers to generate ROOT dictionaries in: %s" % srcpath
		os.chdir(srcpath)
		for f in glob.glob('*.[h|hh|hpp]'):
			if 'ClassDef' in open(f).read():
				env.AppendUnique(ALL_SOURCES = env.ROOTDict(f))
				if(int(env['SHOWBUILD'])>1):
					print "       ROOT dictionary for %s" % f
		os.chdir(curpath)


