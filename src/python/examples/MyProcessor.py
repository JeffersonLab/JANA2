
import jana
#from DHit import *
#from DCluster import *
#
# JEventProcessor class defined in janapy module
class MyProcessor( jana.JEventProcessor):
	def __init__(self):
		super().__init__(self)

	def Init( self ):
		print('Python Init called')

	# event is a JEvent object defined in janapy module
	def Process( self ):
		print('Python Process called')

	def Finish( self ):
		print('Python Finish called')

#		hits = event.Get( DHit )
#		for h in hits:
#			print('hit:  a=%d  b=%f  type=%s' % (h.a, h.b , type(h)))

#-----------------------------------------------------------

jana.SetParameterValue( 'JANA:DEBUG_PLUGIN_LOADING', '1')
jana.SetParameterValue( 'NTHREADS', '4')
jana.SetParameterValue( 'NEVENTS', '100')

# The janapy module itself serves as the JApplication facade
jana.AddProcessor( MyProcessor() )

jana.AddPlugin('jtest')
jana.Run()

print('PYTHON DONE.')

