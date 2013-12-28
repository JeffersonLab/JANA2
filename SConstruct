
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

# Get platform-specific name
osname = os.getenv('BMS_OSNAME', 'build')
if(osname == 'build'): 	osname = subprocess.Popen(["./osrelease.pl"], stdout=subprocess.PIPE).communicate()[0].strip()


# Get architecture name
arch = ROOT_CFLAGS = subprocess.Popen(["uname"], stdout=subprocess.PIPE).communicate()[0].strip()

# Setup initial environment
installdir = "#%s" %(osname)
include = "%s/include" % (installdir)
bin = "%s/bin" % (installdir)
lib = "%s/lib" % (installdir)
plugins = "%s/plugins" % (installdir)
env = Environment(    CPPPATH      = [include],
                      LIBPATH      = [lib],
                      variant_dir  =".%s" % (osname))

# These are SBMS-specific variables (i.e. not default scons ones)
env.Replace(    INSTALLDIR    = installdir,
				OSNAME        = osname,
				INCDIR        = include,
				BINDIR        = bin,
				LIBDIR        = lib,
				PLUGINSDIR    = plugins,
				ALL_SOURCES   = [],        # used so we can add generated sources
				SHOWBUILD     = SHOWBUILD,
				COMMAND_LINE_TARGETS = COMMAND_LINE_TARGETS)

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
env.Replace( CXX = os.getenv('CXX', 'g++'),
             CC  = os.getenv('CC' , 'gcc'),
             FC  = os.getenv('FC' , 'gfortran') )

# Add src and src/plugins to include search path
env.PrependUnique(CPPPATH = ['#src', '#src/plugins'])

# Turn on debug symbols
env.PrependUnique(CFLAGS = ['-g', '-fPIC'], CXXFLAGS = ['-g', '-fPIC'])

# Apply any platform/architecture specific settings
sbms.ApplyPlatformSpecificSettings(env, arch)
sbms.ApplyPlatformSpecificSettings(env, osname)

# generate configuration header file
sbms_config.mk_jana_config_h(env)

# generate jana-config helper script
sbms_config.mk_jana_config_script(env)

# build all src
SConscript('src/SConscript', variant_dir="src/.%s" % (osname), exports='env osname', duplicate=0)

# Make install target
env.Alias('install', installdir)

# Create setenv if user explicitly specified "install" target
build_targets = map(str,BUILD_TARGETS)
if len(build_targets)>0:
	if 'install' in build_targets:
		import sbms_setenv
		sbms_setenv.mk_setenv_csh(env)
		sbms_setenv.mk_setenv_bash(env)

