#
# This example demonstrates how to implement a JEventProccessor in Python.
#
# This can be run like so:
#
#   jana -Pplugins=janapy -PJANA_PYTHON_FILE=MyProcessor.py
#
#
import jana
import inspect

# JEventProcessor class defined in janapy module
class MyProcessor( jana.JEventProcessor ):
	#--------------------------------------
	# Constructor
	def __init__(self):
		super().__init__(self)
		super().Prefetch('Hit')

	#--------------------------------------
	# Init
	def Init( self ):
		print('Python Init called')

	#--------------------------------------
	# Process
	def Process( self ):
		hits = super().Get('Hit') # hits is standard python dictionary with string types for keys and values
		for hit in hits:
			print('E=' + hit['E'] + '  t= ' + hit['t'] )

	#--------------------------------------
	# Finish
	def Finish( self ):
		print('Python Finish called')

#-----------------------------------------------------------

jana.SetParameterValue( 'NEVENTS', '10')

proc = MyProcessor()
jana.AddProcessor( proc )

jana.AddPlugin('JTestRoot')
jana.Run()

print('PYTHON DONE.')

