
import time
import janapy

print('Hello from jana.py!!!')

# Turn off JANA's standard ticker so we can print our own updates
janapy.SetTicker(False)

# Wait for 4 seconds before allowing processing to start
for i in range(1,5):
	time.sleep(1)
	print(" waiting ... %d" % (4-i))

# Start event processing
janapy.Start()

# Wait for 5 seconds while processing events
for i in range(1,6):
	time.sleep(1)
	print(" running ... %d  (Nevents: %d)" % (i, janapy.GetNeventsProcessed()))

# Tell program to quit gracefully
janapy.Quit()


