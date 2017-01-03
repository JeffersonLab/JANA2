
import os
import sys
import subprocess
import glob

# Add SBMS directory to PYTHONPATH
sbmsdir = "%s/SBMS" % (os.getcwd())
sys.path.append(sbmsdir)

import sbms
import sbms_config

# Get command-line options
SHOWBUILD = ARGUMENTS.get('SHOWBUILD', 0)
OPTIMIZATION = ARGUMENTS.get('OPTIMIZATION', 2)
DEBUG = ARGUMENTS.get('DEBUG', 1)
PROFILE = ARGUMENTS.get('PROFILE', 0)
BITNESS32 = ARGUMENTS.get('m32', 0)
BITNESS64 = ARGUMENTS.get('m64', 0)

# Get platform-specific name
osname = os.getenv('BMS_OSNAME')
if(osname == None): 	osname = subprocess.Popen(["./SBMS/osrelease.pl"], stdout=subprocess.PIPE).communicate()[0].strip()
if BITNESS32!=0 : osname = osname.replace('x86_64','i686')
if BITNESS64!=0 : osname = osname.replace('i686','x86_64')


# Get architecture name
arch = ROOT_CFLAGS = subprocess.Popen(["uname"], stdout=subprocess.PIPE).communicate()[0].strip()

# Setup initial environment
installdir = "#%s" %(osname)
include = "%s/include" % (installdir)
bin = "%s/bin" % (installdir)
lib = "%s/lib" % (installdir)
plugins = "%s/plugins" % (installdir)
env = Environment(        ENV = os.environ,  # Bring in full environement, including PATH
                      CPPPATH = [include],
                      LIBPATH = [lib],
                  variant_dir = ".%s" % (osname))

# These are SBMS-specific variables (i.e. not default scons ones)
env.Replace(    INSTALLDIR    = installdir,
				OSNAME        = osname,
				INCDIR        = include,
				BINDIR        = bin,
				LIBDIR        = lib,
				PLUGINSDIR    = plugins,
				ALL_SOURCES   = [],        # used so we can add generated sources
				SHOWBUILD     = SHOWBUILD,
				BITNESS32     = BITNESS32,
				BITNESS64     = BITNESS64,
				COMMAND_LINE_TARGETS = COMMAND_LINE_TARGETS)

# Allow user to force bitness
if BITNESS32!=0:
	env.AppendUnique(CXXFLAGS    = '-m32')
	env.AppendUnique(CFLAGS      = '-m32')
	env.AppendUnique(LINKFLAGS   = '-m32')
	env.AppendUnique(SHLINKFLAGS = '-m32')
if BITNESS64!=0:
	env.AppendUnique(CXXFLAGS    = '-m64')
	env.AppendUnique(CFLAGS      = '-m64')
	env.AppendUnique(LINKFLAGS   = '-m64')
	env.AppendUnique(SHLINKFLAGS = '-m64')

# Use terse output unless otherwise specified
if SHOWBUILD==0:
	env.Replace(   CCCOMSTR        = "Compiling  [$SOURCE]",
				  CXXCOMSTR       = "Compiling  [$SOURCE]",
				  FORTRANPPCOMSTR = "Compiling  [$SOURCE]",
				  FORTRANCOMSTR   = "Compiling  [$SOURCE]",
				  SHCCCOMSTR      = "Compiling  [$SOURCE]",
				  SHCXXCOMSTR     = "Compiling  [$SOURCE]",
				  LINKCOMSTR      = "Linking    [$TARGET]",
				  SHLINKCOMSTR    = "Linking    [$TARGET]",
				  INSTALLSTR      = "Installing [$TARGET]",
				  ARCOMSTR        = "Archiving  [$TARGET]",
				  RANLIBCOMSTR    = "Ranlib     [$TARGET]")


# Get compiler from environment variables (if set)
env.Replace( CXX = os.getenv('CXX', 'c++'),
             CC  = os.getenv('CC' , 'cc'),
             FC  = os.getenv('FC' , 'gfortran') )

# Get compiler name
compiler = 'unknown'
compiler_string = subprocess.Popen([env['CC'],"-v"], stderr=subprocess.PIPE).communicate()[1]
if 'clang' in compiler_string:
	compiler = 'clang'
if 'gcc' in compiler_string and 'clang' not in compiler_string:
	compiler = 'gcc'
env.Replace(COMPILER = compiler)

# Add src and src/plugins to include search path
env.PrependUnique(CPPPATH = ['#', '#src', '#src/plugins'])

# Standard flags (optimization level and warnings)
env.PrependUnique(      CFLAGS = ['-O%s' % OPTIMIZATION, '-fPIC', '-Wall'])
env.PrependUnique(    CXXFLAGS = ['-O%s' % OPTIMIZATION, '-fPIC', '-Wall', '-std=c++11'])
env.PrependUnique(FORTRANFLAGS = ['-O%s' % OPTIMIZATION, '-fPIC', '-Wall'])

# Turn on debug symbols unless user told us not to
if not DEBUG=='0':
	env.PrependUnique(      CFLAGS = ['-g'])
	env.PrependUnique(    CXXFLAGS = ['-g'])
	env.PrependUnique(FORTRANFLAGS = ['-g'])

# Turn on profiling if user asked for it
if PROFILE=='1':
	env.PrependUnique(      CFLAGS = ['-pg'])
	env.PrependUnique(    CXXFLAGS = ['-pg'])
	env.PrependUnique(FORTRANFLAGS = ['-pg'])
	env.PrependUnique(   LINKFLAGS = ['-pg'])

# Add pthread (more efficient to do this here since it involves test compilations)
sbms.Add_pthread(env)

# Apply any platform/architecture specific settings
sbms.ApplyPlatformSpecificSettings(env, arch)
sbms.ApplyPlatformSpecificSettings(env, osname)

# generate configuration header file
sbms_config.mk_jana_config_h(env)

# generate jana-config helper script
sbms_config.mk_jana_config_script(env)

# build all src
SConscript('src/SConscript', variant_dir="src/.%s" % (osname), exports='env osname', duplicate=0)

# install scripts
SConscript('scripts/SConscript', exports='env osname', duplicate=0)

# Make install target
env.Alias('install', installdir)

# Create setenv and make link to src if user explicitly specified "install" target
build_targets = map(str,BUILD_TARGETS)
if len(build_targets)>0:
	if 'install' in build_targets:
		import sbms_setenv
		sbms_setenv.mk_setenv_csh(env)
		sbms_setenv.mk_setenv_bash(env)
		src_dir = '%s/src' % env.Dir(installdir).abspath
		try:
			os.symlink('../src', src_dir)
		except:
			pass

