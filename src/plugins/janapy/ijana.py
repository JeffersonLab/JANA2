
import sys
import time
import subprocess
import threading
import tty
import termios
import select
import fcntl
import struct
import jana

#------------------ ANSI Escape sequences ------------------

RESET   = u'\u001b[0m'
BLACK   = u'\u001b[30m'
RED     = u'\u001b[31m'
GREEN   = u'\u001b[32m'
YELLOW  = u'\u001b[33m'
BLUE    = u'\u001b[34m'
MAGENTA = u'\u001b[35m'
CYAN    = u'\u001b[36m'
WHITE   = u'\u001b[37m'

BRIGHT_BLACK   = u'\u001b[90m'
BRIGHT_RED     = u'\u001b[91m'
BRIGHT_GREEN   = u'\u001b[92m'
BRIGHT_YELLOW  = u'\u001b[93m'
BRIGHT_BLUE    = u'\u001b[94m'
BRIGHT_MAGENTA = u'\u001b[95m'
BRIGHT_CYAN    = u'\u001b[96m'
BRIGHT_WHITE   = u'\u001b[97m'

BACKGROUND_BLACK   = u'\u001b[40m'
BACKGROUND_RED     = u'\u001b[41m'
BACKGROUND_GREEN   = u'\u001b[42m'
BACKGROUND_YELLOW  = u'\u001b[43m'
BACKGROUND_BLUE    = u'\u001b[44m'
BACKGROUND_MAGENTA = u'\u001b[45m'
BACKGROUND_CYAN    = u'\u001b[46m'
BACKGROUND_WHITE   = u'\u001b[47m'

BOLD      = u'\u001b[1m'
UNDERLINE = u'\u001b[4m'
REVERESED = u'\u001b[7m'

def UP(n)              : sys.stdout.write( u'\u001b['+str(n)+'A' )
def DOWN(n)            : sys.stdout.write( u'\u001b['+str(n)+'B' )
def RIGHT(n)           : sys.stdout.write( u'\u001b['+str(n)+'C' )
def LEFT(n)            : sys.stdout.write( u'\u001b['+str(n)+'D' )
def CLEAR()            : sys.stdout.write( u'\u001b[2J' )
def CLEARLINE()        : sys.stdout.write( u'\u001b[0K' )
def HOME()             : sys.stdout.write( u'\u001b[;H' )
def MOVETO(X,Y)        : sys.stdout.write( u'\u001b[%d;%dH' % (int(Y), int(X)) )
def PRINTAT(X,Y,S)     : MOVETO(X,Y) ; sys.stdout.write( S )
def PRINTCENTERED(Y,S) : MOVETO( (NCOLS-len(S))/2 , Y ) ; sys.stdout.write( S )

#-------------------- CLI Commands -------------------------
COMMANDS = {}
COMMANDS['help'] = 1
#-----------------------------------------------------------

#------------------------------
# draw_banner
#------------------------------
def draw_banner():
	
	# Get terminal size and store in globals
	global NROWS, NCOLS
	NROWS, NCOLS, hp, wp = struct.unpack('HHHH', fcntl.ioctl(sys.stdout.fileno(), termios.TIOCGWINSZ, struct.pack('HHHH', 0, 0, 0, 0)))
	
	banner_height = 15

	# Make sure we have enough lines to draw banner. Inform user if not
	if NROWS<banner_height+5 or NCOLS<60:
		mess = 'Please make terminal '
		if NROWS<banner_height+5:
			mess += 'taller'
			if NCOLS<60: mess += ' and wider'
		if NCOLS<60: mess += 'wider'
		MOVETO(1,1)
		CLEARLINE()
		sys.stdout.write( REVERESED )
		PRINTCENTERED(1, mess)
		sys.stdout.write( RESET )
		MOVETO(1,NROWS)
		sys.stdout.flush()
		return

	# Draw border/background
	sys.stdout.write( BACKGROUND_MAGENTA + BRIGHT_WHITE + BOLD  )
	PRINTAT( 1, 1, '-'*NCOLS)
	for i in range(2, banner_height): PRINTAT( 1, i, '|' + ' '*(NCOLS-2) + '|')
	PRINTAT( 1, banner_height, '-'*NCOLS)

	# Draw status
	PRINTAT( 3, 2, 'Num. Events Processed: %d' % jana.GetNeventsProcessed() )
	PRINTAT( 3, 3, ' Num. Tasks Completed: %d' % jana.GetNtasksCompleted() )
	PRINTAT( 3, 4, '           Num. Cores: %d' % jana.GetNcores() )
	PRINTAT( 3, 5, '        Num. JThreads: %d' % jana.GetNJThreads() )

	PRINTAT( NCOLS/2, 2, 'Rate: %5.0fHz (%5.0fHz avg.)' % (jana.GetInstantaneousRate(), jana.GetIntegratedRate()) )

	# Cursor is repositioned and stdout flushed in command_line after calling this
	# so no need to do it here. Just reset to default drawing options.
	sys.stdout.write( RESET )

#------------------------------
# print_help
#------------------------------
def print_help():
	print '\n JANA Python Interactive CLI Help'
	print '\r------------------------------------'
	print '\r banner [on/off]    set banner on/off (def. on)"'
	print '\r exit               same as "quit"'
	print '\r help               print this message'
	print '\r history            print history of this session'
	print '\r parameter cmd ...  get/set/list config. parameters'
	print '\r quit               stop data processing and quit the program'
	print '\r resume             resume data processing'
	print '\r status             print some status info'
	print '\r stop               pause data processing'

#------------------------------
# process_command
#------------------------------
def process_command( input ):
	global CLI_ACTIVE, BANNER_ON, mode

	stripped = input.rstrip()
	tokens = input.rstrip().split()
	if len(tokens) < 1: return
	cmd = tokens[0]
	args = tokens[1:]
	
	print 'processing command: ' + ' '.join(tokens)
	LEFT(1000)

	#--- banner
	if cmd=='banner':
		if len(args) == 0:
			print 'command banner requires you to specify "on" or "off". (see help for details)'
		elif args[0] == 'on' : BANNER_ON = True
		elif args[0] == 'off': BANNER_ON = False
		else: print 'command banner requires you to specify "on" or "off". (see help for details)'
	#--- exit, quit
	elif cmd=='exit' or cmd=='quit':
		CLI_ACTIVE = False
		jana.Quit()
	#--- help
	elif cmd=='help':
		print_help()
	#--- history
	elif cmd=='history':
		idx = 0
		for h in history:
			print '%3d %s\r' % (idx, h)
			idx += 1
		print '%3d %s\r' % (idx, ' '.join(tokens) )  # include this history command which will be added below
	#--- nthreads
	elif cmd=='nthreads':
		if len(args) == 0:
			print 'Number of JThreads: %d' % jana.GetNJThreads()
		elif len(args)==1:
			print 'Number of JThreads now at: %d' % jana.SetNJThreads( int(args[0]) )
		else:
			print 'njthreads takes either 0 or 1 argument (see help for details)'
	#--- parameter
	elif cmd=='parameter':
		if len(args) == 0:
			print 'command parameter requires arguments. (see help for details)'
		elif args[0] == 'get':
			if len(args)==2:
				print '%s: %s' % (args[1], jana.GetParameterValue(args[1]))
			else:
				print 'command parameter get requires exactly 1 argument! (see help for details)'
		elif args[0] == 'set':
			if len(args)==3:
				jana.SetParameter(args[1], args[2])
			else:
				print 'command parameter set requires exactly 2 arguments! (see help for details)'
		elif args[0] == 'list':
			if len(args)==1:
				jana.PrintParameters(True)
			else:
				print 'command parameter list requires no arguments! (see help for details)'
	#--- resume
	elif cmd=='resume':
		jana.Resume()
		print 'event processing resumed'
	#--- status
	elif cmd=='status':
		jana.PrintStatus()
		print ''
	#--- stop
	elif cmd=='stop':
		jana.Stop(True)
		print 'event processing stopped after %d events (use "resume" to start again)' % jana.GetNeventsProcessed()
	#--- unknown command
	else:
		print 'Unknown command: ' + cmd
		return

	history.append( ' '.join(tokens) ) # add to history


#------------------------------
# tab_complete
#------------------------------
def tab_complete( input ):
	return input

#------------------------------
# syntax_highlight
#------------------------------
def syntax_highlight(input):
	stripped = input.rstrip()
	return stripped + RED + " " *  (len(input) - len(stripped)) + RESET

#------------------------------
# command_line
#------------------------------
def command_line():

	global CLI_ACTIVE, history, mode, BANNER_ON

	# Record tty settings so we can restore them when finished
	# processing the CLI
	mode = termios.tcgetattr(sys.stdin)
	
	# Wait for first event to process so all messages are printed
	# before switching to "raw" mode which screws up newlines
	while jana.GetNeventsProcessed()<1: time.sleep(1)

	# Set to raw mode so single characters get passed to us as they are typed
	tty.setraw(sys.stdin)

	prompt = 'jana> '

	CLI_ACTIVE = True
	BANNER_ON  = True
	history    = []
	last_draw_time = 0

	while CLI_ACTIVE and not jana.IsDrainingQueues(): # loop for each line
		# Define data-model for an input-string with a cursor
		input = ""
		index = 0
		input_save = ""
		history_index = len(history)
		sys.stdout.write(prompt)
		sys.stdout.flush()
		while CLI_ACTIVE and not jana.IsDrainingQueues(): # loop for each character
		
			# Redraw the screen periodically
			if BANNER_ON and (time.time() - last_draw_time > 1):
				draw_banner()
				MOVETO(1,NROWS)
				if index >= 0: RIGHT( index+len(prompt) ) ; sys.stdout.flush()
				last_draw_time = time.time()

			# Wait for a character to be typed. 0.100 is timeout in seconds
			# Doing this first allows us to check if the app is quitting
			# even if the user is not typing so *maybe* we can restore the
			# termios settings before the final report is printed. It also
			# allows us to update the screen periodically when nothing is
			# being typed.
			i, o, e = select.select( [sys.stdin], [], [], 0.100 )
			if not i: continue  # nothing was typed

			char = ord(sys.stdin.read(1)) # read one char and get char code
			
			# Manage internal data-model
			if char == 3: return    # CTRL-C
			elif char in {10, 13}: break # LF or CR
			elif char ==  9:        # TAB
				rindex = len(input) - index
				input = tab_complete(input)
				index = len(input) - rindex
			elif 32 <= char <= 126: # Non-special character
				input = input[:index] + chr(char) + input[index:]
				index += 1
			elif char == 127:       # Delete/Backspace
				input = input[:index-1] + input[index:]
				index -= 1
			elif char == 27:        # Left or right arrow start
				next1, next2 = ord(sys.stdin.read(1)), ord(sys.stdin.read(1))
				if next1==91 and next2==68: index = max(0, index - 1)    # Left arrow
				if next1==91 and next2==67: index = min(len(input), index + 1)    # Right arrow
				if next1==91 and next2==66:                              # Down arrow
					if history_index < (len(history)-1):
						history_index += 1
						input = history[ history_index ]
						index = len(input)
					elif history_index == (len(history)-1):
						history_index += 1
						input = input_save
						index = len(input)

				if next1==91 and next2==65:                              # Up arrow
					if history_index > 0:
						if history_index == len(history): input_save = input
						history_index -= 1
						input = history[ history_index ]
						index = len(input)

#			else:
#				print '\nunknown char: %d\n' % int(char)
#				sys.stdout.flush()

			# Print current input-string
			MOVETO(1,NROWS)
			CLEARLINE()
			sys.stdout.write(prompt + syntax_highlight(input))
			MOVETO(1,NROWS)
			if index >= 0: RIGHT( index+len(prompt) )
			sys.stdout.flush()
		
		# Process one input line
		print ''
		MOVETO(1,NROWS)
		termios.tcsetattr(sys.stdin, termios.TCSAFLUSH, mode)
		process_command( input )
		tty.setraw(sys.stdin)
		MOVETO(1,NROWS)
		sys.stdout.flush()
		input = ""
		index = 0
		last_draw_time = 0 # force banner to redraw on next iteration

	# Restore terminal settings
	termios.tcsetattr(sys.stdin, termios.TCSAFLUSH, mode)
	print '\nCLI exiting ...\n'

#===========================================================
#                         MAIN

jana.SetTicker( False )
jana.WaitUntilAllThreadsRunning()

# print '=================================================================================='
# import inspect
# for m in inspect.getmembers(jana): print str(m)
# print '=================================================================================='
command_line()


