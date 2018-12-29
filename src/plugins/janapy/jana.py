

import time
import jana

jana.WaitUntilAllThreadsRunning()

for i in range(0,20):
	time.sleep(1)
	print '\nCalled jana.GetNeventsProcessed: %d' % jana.GetNeventsProcessed()
