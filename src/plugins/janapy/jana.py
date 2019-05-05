
import janapy

janapy.AddPlugin('jtest')
janapy.SetParameterValue('NEVENTS', 10000)
janapy.Run()
janapy.PrintFinalReport()

jana.SetTicker( False )
jana.WaitUntilAllThreadsRunning()

for i in range(0,20):
	time.sleep(1)
	print( '\nCalled jana.GetNeventsProcessed: %d' % jana.GetNeventsProcessed())
