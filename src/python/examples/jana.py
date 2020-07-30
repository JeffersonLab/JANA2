
import time
import jana

print('Hello from jana.py!!!')

# Turn off JANA's standard ticker so we can print our own updates
jana.SetTicker(False)

# Wait for 4 seconds before allowing processing to start
for i in range(1,5):
	time.sleep(1)
	print(" waiting ... %d" % (4-i))

# Start event processing
jana.Start()

# Wait for 5 seconds while processing events
for i in range(1,6):
	time.sleep(1)
	print(" running ... %d  (Nevents: %d)" % (i, jana.GetNeventsProcessed()))

# Tell program to quit gracefully
jana.Quit()

