
#
# Mac OS X specific settings
#

def InitENV(env):

	# This is needed to allow plugins to have their
	# global variables linked to those in the running
	# executable.
	env.AppendUnique(LINKFLAGS='-flat_namespace')

