
import sys
sys.path.append('scons_scripts')

from init_env import init_environment
from util import scanFiles
from util import get_subdirectories

env = init_environment("jana xercesc")

# Include Path
incpath = [ 'src']
env.Append(CPPPATH = incpath)



jana_sources = scanFiles('src/JANA' , accept=["*.cc"])
jana_lib = env.Library(source = jana_sources, target = 'lib/JANA')


utilities = get_subdirectories('src/utilities')
utilities = filter( (lambda a : a.rfind('janahbook')==-1 ), utilities )
utilities = filter( (lambda a : a.rfind('jcalibws')==-1 ),  utilities )
utilities = filter( (lambda a : a.rfind('.svn')==-1 ),  utilities )

for u in utilities:
	udir = 'src/utilities/' + u
	ubin = 'bin/' + u
	usource = scanFiles(udir , accept=["*.cc"], )
	uprog = env.Program(source = usource, target = ubin)
	Depends(uprog, jana_lib)




