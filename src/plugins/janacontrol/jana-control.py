#!/usr/bin/env python
#
# GUI for monitoring and controlling a running JANA process
# via ZMQ messages.
#
# This is only useful if the process was started with the janacontrol
# plugin attached.
#
# By default, this uses port 11238 (defined in janacontrol.cc), but
# can use a different port number if specified by the JANA_ZMQ_PORT
# when the JANA process was started.
#
# NOTE: This was originally copied from the HOSS GUI script so there
# are quite a bit of things that are not used. This needs a major overhaul
# to remove unnecessary things and to clean it up.

# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.


import zmq
import json
import threading
import traceback
import time
from tkinter import *
import tkinter.messagebox as messagebox
import tkinter.ttk as ttk
import tkinter.font as tkFont
from collections import OrderedDict
# from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
# from matplotlib.figure import Figure
# import matplotlib.pyplot as plt
# import numpy as np

# -- Globals
DONE      = False
PORT      = 11238
HOST      = 'localhost'
SOCKET    = None

#-----------------------------------------------------------------------------
# MultiColumnListbox
#
# This copied from https://stackoverflow.com/questions/5286093/display-listbox-with-columns-using-tkinter
# I've modified this by Adding the _upsrt method to insert and update data.
# The sortby proc was also made a method of this class.
#
class MultiColumnListbox(object):
	def __init__(self, parent, columns, title):
		self.parent = parent
		self.columns = columns
		self.item_map = {} # key=hostname val=item returned by insert
		self.tree = None
		self._setup_widgets(title)
		self._build_tree()

	def _setup_widgets(self, title):
		msg = ttk.Label(self.parent, wraplength="4i", justify="left", anchor="n", padding=(10, 2, 10, 6), text=title)
		msg.grid(row=0, sticky=W+E)
		container = ttk.Frame(self.parent)
		container.grid(row=1, sticky=N+S+W+E)
		# create a treeview with dual scrollbars
		self.tree = ttk.Treeview(columns=self.columns, height=15, show="headings")
		vsb = ttk.Scrollbar(orient="vertical", command=self.tree.yview)
		hsb = ttk.Scrollbar(orient="horizontal", command=self.tree.xview)
		self.tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
		self.tree.grid(column=0, row=0, sticky='nsew', in_=container)
		vsb.grid(column=1, row=0, sticky='ns', in_=container)
		hsb.grid(column=0, row=1, sticky='ew', in_=container)
		Grid.rowconfigure(container, 0, weight=1)
		Grid.columnconfigure(container, 0, weight=1)
		Grid.rowconfigure(self.parent, 1, weight=1)
		Grid.columnconfigure(self.parent, 0, weight=1)

	def _build_tree(self):
		for col in self.columns:
			self.tree.heading(col, text=col.title(), command=lambda c=col: self.sortby(self.tree, c, 0))
			# adjust the column's width to the header string
			myanchor = E
			if col == self.columns[0]: myanchor=W
			colwidth = int(tkFont.Font().measure(col.title()))
			self.tree.column(col, width=colwidth, anchor=myanchor, stretch=NO)

	# Upsert row in table. "row" should be dictionary with keys for every
	# column. If a row for the "host" already exists, its values are
	# updated. Otherwise a new row is inserted for that host.
	def _upsrt_row(self, row):
		# Create a list of the values for each entry corresponding to the column names
		row_list = []
		for col in self.columns:
			if col in row:
				row_list.append( row[col] )
			else:
				row_list.append( '' )
		item = tuple(row_list)
		
		# Use first column as key
		mykey = item[0]
		if mykey not in self.item_map.keys():
			self.item_map[mykey] = self.tree.insert('', 'end', values=item)
		# Update row with new values and adjust column's width if necessary to fit each value
		for ix, val in enumerate(item):
			self.tree.set(self.item_map[mykey], self.columns[ix], val)
			col_w = tkFont.Font().measure(val)
			if self.tree.column(self.columns[ix],width=None)<col_w:
				self.tree.column(self.columns[ix], width=col_w)

	# sort tree contents when a column header is clicked on
	def sortby(self, tree, col, descending):
		# grab values to sort
		data = [(tree.set(child, col), child) for child in tree.get_children('')]
		# if the data to be sorted is numeric change to float
		#data =  change_numeric(data)
		# now sort the data in place
		data.sort(reverse=descending)
		for ix, item in enumerate(data): tree.move(item[1], '', ix)
		# switch the heading so it will sort in the opposite direction
		tree.heading(col, command=lambda column=col: self.sortby(tree, column, int(not descending)))

#-----------------------------------------------------------------------------
# StripChart
#
# This copied from http://code.activestate.com/recipes/578871-simple-tkinter-strip-chart-python-3/
# (c) MIT License Copyright 2014 Ronald H Longo
#
# Some significant improvements made by David Lawrence.
#
# The "trackColors" argument to constructor should be dictionary of values with
# track name as key and color as value (both strings). The names should match
# values passed to plotValues. e.g. plotValues(A=1, B=2, C=0) if the keys in
# dictionary were "A", "B", and "C".
class StripChart( Frame ):
	def __init__( self, parent, scale, historySize, trackColors, trackLabels, *args, **opts ):
		# Initialize
		super().__init__( parent, *args, **opts )
		self._trackHist   = OrderedDict() # Map: Track Name -> list of canvas objID
		self._trackColor  = trackColors   # Map: Track Name -> color
		self._trackLabel  = trackLabels   # Map: Track Name -> label (for legend)
		self.ymax = 1.0E-8
		self.yautoscale = BooleanVar()
		self.yautoscale.set(True)
		self._clearValues = False
		self._ticks = 0

		# offsets in pixels
		self.yoff = 10
		self.xoff = 10
		self.num_hlines = 2  # does not include top and bottom lines!

		# Options
		self._cbautoscale = Checkbutton(self, text='y-autoscale', variable=self.yautoscale)
		self._cbautoscale.grid(row=0, sticky=W)
		self._bclear = Button(self, text='clear', command=self.clearHistory)
		self._bclear.grid(row=0, column=1, sticky=W)

		self._chartHeight = scale + 1
		self._chartLength = historySize * 2  # Stretch for readability

		self._canvas = Canvas( self, height=self._chartHeight + 17, width=self._chartLength, background='black' )
		self._canvas.grid( row=1, columnspan=100, sticky=N+S+E+W )

		# Draw horizontal to divide plot from tick labels
		self._hlines = []
		self._hlabels = []
		x, y  = 0, 0 # these will be overwritten in resizeCanvas pretty much immediately
		x2,y2 = 1,1  # these will be overwritten in resizeCanvas pretty much immediately
		for idx in range(0, self.num_hlines+2):
			dash = (3,4)                                # midlines
			if idx == 0 : dash = (2,4)                  # top line
			if idx == self.num_hlines+1 : dash = (1,1)  # bottom line
			self._hlines.append( self._canvas.create_line( x, y, x2, y2, fill='white' , dash=dash) )
			self._hlabels.append( self._canvas.create_text( x, y, text='---', anchor=W, fill='white' ) )

		# Draw legend
		i = 0
		self._legendlines = []
		self._legendtext = []
		for k,color in trackColors.items():
			self._legendlines.append( self._canvas.create_line( 0, 0, 0, 1 , width=4, fill=color ))
			label = k
			if k in self._trackLabel: label = self._trackLabel[k]
			self._legendtext.append( self._canvas.create_text( 0, 0, text='\n'.join(label), fill='white', font=('Clean', 9), anchor=N ))
			i += 1

		# Init track def and histories lists
		self._trackColor.update( { 'tick':'white', 'tickline':'white',
                                 'ticklabel':'white' } )
		for trackName in self._trackColor.keys(): self._trackHist[ trackName ] = [None] * historySize

		Grid.rowconfigure(self, 0, weight=0)    # Options
		Grid.rowconfigure(self, 1, weight=1)    # Canvas
		Grid.columnconfigure(self, 0, weight=1)
		self.bind('<Configure>', self.resizeCanvas)

	#---------------------------------------------------
	# resizeCanvas
	#
	# Called when window size changes which includes right when the program firt starts up.
	def resizeCanvas(self, event):
		print('Resized: width=%d  height=%d' % (event.width, event.height))
		self._chartHeight = event.height - 60
		self._chartLength = event.width - 50

		# Adjust number of entries in history list
		historySize = self._chartLength/2
		for trackName, trackHistory in self._trackHist.items():
			while len(trackHistory)>historySize: self._canvas.delete( trackHistory.pop(0) ) # Remove left-most canvas objs
			Nneeded = int(historySize-len(trackHistory))
			if Nneeded>0: trackHistory.extend( [None] * Nneeded )

		yoff = self.xoff
		x2  = self.xoff + self._chartLength
		y   = yoff + self._chartHeight + 2

		num_hlines = len(self._hlines)
		for idx in range(0, num_hlines):
			ymid = int((y-yoff)*idx/(num_hlines-1)) + yoff
			self._canvas.coords( self._hlines[idx],  [0, ymid, x2, ymid] )
			self._canvas.coords( self._hlabels[idx], [4, ymid+10] )

		# Legend
		for i in range(0, len(self._legendlines)):
			x = self.xoff + self._chartLength + 10 + i*10
			y = self.yoff
			self._canvas.coords( self._legendlines[i], [x, y, x, y+15] )
			self._canvas.coords( self._legendtext[i], [x, y+20] )

	#---------------------------------------------------
	# plotValues
	#
	# Add new time step to strip chart, scrolling it to the left and removing oldest time step.
	# This is called periodically from MyWindow::TimerUpdate
	def plotValues( self, **vals ):

		# If user hit Clear button, then clear history first
		if self._clearValues : self.clearValues()

		# Increment time counter and draw vertical tick occasionally.
		if (self._ticks % 30) == 0: self. drawTick( text=str(self._ticks), dash=(1,4) )
		self._ticks += 1

		for trackName, trackHistory in self._trackHist.items():
			# Scroll left-wards
			self._canvas.delete( trackHistory.pop(0) ) # Remove left-most canvas objs
			self._canvas.move( trackName, -2, 0 )      # Scroll canvas objs 2 pixels left
				  
			# Plot the new values
			try:
				val = vals[ trackName ]
				yoff = self.yoff
				xoff = self.xoff
				x = xoff + self._chartLength
				y = self.YtoCanvas( val )
				color = self._trackColor[ trackName ]

				width = 4
				# if trackName=='A': width = 4
				# if trackName=='B': width = 6
				# if trackName=='C': width = 2
				objId = self._canvas.create_line( x, y, x+2, y, fill=color, width=width, tags=trackName )
				trackHistory.append( objId )

			except:
				trackHistory.append( None )
	
		# Check if we need to change scale of chart
		if self.yautoscale.get():
			ymax = self.getYmax()
			if ymax < 1.0E-2: ymax = 1.0E-2
			if ymax != self.ymax:
				scale = self.ymax/ymax
				for trackName, trackHistory in self._trackHist.items():
					if trackName.startswith('tick'): continue  # skip tick marks and labels
					for objId in trackHistory:
						if objId is None : continue
						y = scale*self.YfromCanvas(objId)
						coords = self._canvas.coords(objId)
						coords[1] = coords[3] = self.YtoCanvas(y)
						self._canvas.coords(objId, coords)
				self.ymax = ymax # update only after all calls to YfromCanvas and YtoCanvas are made
		else:
			self.ymax = self.ymax_default

		# Redraw labels
		num_hlines = len(self._hlines)
		for idx in range(0, num_hlines):
			y = self.ymax*idx/(num_hlines-1)
			self._canvas.itemconfig(self._hlabels[idx], text='%3.1f GB/s' % (self.ymax - y))

	#---------------------------------------------------
	# clearHistory
	#
	# (see clearValues below)
	def clearHistory(self):
		print('Clear button pressed')
		self._clearValues = True

	#---------------------------------------------------
	# clearValues
	#
	# This is called from plotValues iff the self._clearValues flag is set. It needs to
	# be done that way since plotValues is called asynchronously to when the "Clear"
	# button is pressed.
	def clearValues(self):
		# Remove all scrolling objects
		print('Clearing history')
		for trackName, trackHistory in self._trackHist.items():
			# Scroll left-wards
			for t in trackHistory: self._canvas.delete( t )
			self._trackHist[trackName] = [None] * len(self._trackHist[trackName])
		# self.rate_stripchart_ticks = 0
		self._ticks = 0
		self._clearValues = False

	def drawTick( self, text=None, **lineOpts ):
		# draw vertical tick line
		yoff = self.yoff
		xoff = self.xoff
		x  = self._chartLength + xoff
		y  = yoff
		x2 = x
		y2 = yoff + self._chartHeight + 2
		color = self._trackColor[ 'tickline' ]
		
		objId = self._canvas.create_line( x, y, x2, y2, fill=color, tags='tick', **lineOpts )
		self._trackHist[ 'tickline' ].append( objId )
		
		# draw tick label
		if text is not None:
			x = self._chartLength + xoff
			y = self._chartHeight + yoff + 10
			color = self._trackColor[ 'ticklabel' ]
			
			objId = self._canvas.create_text( x, y, text=text, fill=color, tags='tick' )
			self._trackHist[ 'ticklabel' ].append( objId )

	def configTrackColors( self, **trackColors ):
		# Change plotted data color
		for trackName, colorName in trackColors.items( ):
			self._canvas.itemconfigure( trackName, fill=colorName )
		
		# Change settings so future data has the new color
		self._trackColor.update( trackColors )

	def SetYmax( self, ymax ):
		if ymax is None:
			self.yautoscale.set(True)
		else:
			self.yautoscale.set(False)
			self.ymax_default = ymax

	def YtoCanvas(self, y):
		h = self._chartHeight
		deltay = y*h/self.ymax
		return self.yoff + self._chartHeight - deltay

	def YfromCanvas(self, objId):
		h = self._chartHeight
		y = self._canvas.coords(objId)[1]
		return (self.yoff + self._chartHeight - y)*self.ymax/h

	def getYmax(self):
		ymax = 0
		for trackName, trackHistory in self._trackHist.items():
			if trackName.startswith('tick'): continue   # skip tick marks and labels
			for objId in trackHistory:
				if objId is not None :
					y = self.YfromCanvas( objId )
					if y > ymax: ymax = y
		return ymax


#-----------------------------------------------------------------------------
# MyWindow   (The GUI)
class MyWindow(Frame):

	#=========================
	# __init__
	def __init__(self, master=None):
		Frame.__init__(self, master)
		self.master = master
		self.init_window()
		self.cmd_queue = []  # List of commands to send during TimerUpdate call

	#=========================
	# init_window
	def init_window(self):
	
		self.labelsFont = tkFont.Font(family="Helvetica", size=16)
		self.labmap = {}
	
		self.master.title('JANA Status/Control GUI')
		self.grid(row=0, column=0, sticky=N+S+E+W ) # Fill root window
		Grid.rowconfigure(self, 0, weight=1)
		Grid.columnconfigure(self, 0, weight=1)

		#----- Information Section (Top)
		infoframe = Frame(self)
		infoframe.grid( row=0, sticky=N+S+E+W)

		# Host Info (left)
		labels = {
			'program': 'Program Name',
			'host'   :'host',
			'PID'    :'PID'
		}
		hostinfoframe = Frame(infoframe)
		hostinfoframe.grid( row=0, column=0, sticky=N+S+E+W, padx=30, ipadx=10 )
		for i,(varname,label) in enumerate(labels.items()):
			self.labmap[varname] = StringVar()
			ttk.Label(hostinfoframe, anchor='e', text=label+': ', font=self.labelsFont).grid(row=i, column=0, sticky=E+W)
			ttk.Label(hostinfoframe, anchor='w', textvariable=self.labmap[varname], font=self.labelsFont  ).grid(row=i, column=1, sticky=E+W)
		
		# Process Info (right)
		labels = { # keys are names in JSON record, vales are text labels for GUI
			'NEventsProcessed': 'Number of Events',
			'rate_avg': 'Avg. Rate (Hz)',
			'rate_instantaneous': 'Rate (Hz)',
			'NThreads': 'Number of Threads',
			'cpu_total': 'CPU Total Usage (%)',
			'cpu_idle': 'CPU idle (%)',
			'cpu_nice': 'CPU nice (%)',
			'cpu_sys': 'CPU system (%)',
			'cpu_user': 'CPU user (%)',
			'ram_tot_GB': 'RAM total (GB)',
			'ram_avail_GB': 'RAM avail. (GB)',
			'ram_free_GB': 'RAM free (GB)',
			'ram_used_this_proc_GB': 'RAM used this proc (GB)'
		}
		procinfoframe = Frame(infoframe)
		procinfoframe.grid( row=0, column=1, sticky=N+S+E+W, padx=30, ipadx=10 )
		for i,(varname,label) in enumerate(labels.items()):
			self.labmap[varname] = StringVar()
			ttk.Label(procinfoframe, anchor='e', text=label+': ', font=self.labelsFont).grid(row=i, column=0, sticky=E+W)
			ttk.Label(procinfoframe, anchor='w', textvariable=self.labmap[varname], font=self.labelsFont  ).grid(row=i, column=1, sticky=E+W)


		# Initialize all labels
		for key in self.labmap.keys(): self.labmap[key].set('---')

		#----- Control Section (Middle)
		controlframe = Frame(self)
		controlframe.grid( row=1, sticky=N+S+E+W)
		controlframe.columnconfigure(0, weight=1)
		but1 = ttk.Button(controlframe, text='Nthreads-', command=self.DecrNThreads)
		but1.grid(row=0, column=0)
		but2 = Button(controlframe, text='Nthreads+', command=self.IncrNthreads)
		but2.grid(row=0, column=1)

		remotequitButton = Button(controlframe, text="Quit Remote",command=self.QuitRemote)
		remotequitButton.grid( row=1, column=0, columnspan=2, sticky=E+W )

		#----- GUI controls (Bottom)
		guicontrolframe = Frame(self)
		guicontrolframe.grid( row=2, sticky=E+W )
		guicontrolframe.columnconfigure(1, weight=1)


		closeButton = Button(guicontrolframe, text="Close",command=self.Quit)
		closeButton.grid( row=0, column=2, sticky=E+W)
		closeButton.columnconfigure(0, weight=1)

		#===== Configure weights of layout
		Grid.rowconfigure(self, 0, weight=10)  # Info
		Grid.rowconfigure(self, 1, weight=10)  # Control
		Grid.rowconfigure(self, 2, weight=1)   # GUI controls
		Grid.columnconfigure(self, 0, weight=1)


	#=========================
	# Quit
	def Quit(self):
		global DONE, root
		DONE=True
		root.destroy()

	#=========================
	# QuitRemote
	def QuitRemote(self):
		self.cmd_queue.append( 'quit' )

	#=========================
	# DecrNThreads
	def DecrNThreads(self):
		N = self.last_info['NThreads']
		N -= 1
		self.cmd_queue.append( 'set_nthreads '+str(N) )

	#=========================
	# IncrNthreads
	def IncrNthreads(self):
		N = self.last_info['NThreads']
		N += 1
		self.cmd_queue.append( 'set_nthreads '+str(N) )

	# #=========================
	# # OnDoubleClick
	# def OnDoubleClick(self, event, tree, hostinfo):
	# 	# Show info dialog with all values from JSON record
	# 	item = tree.identify('item', event.x, event.y)
	# 	vals = tree.item(item)['values']
	# 	if len(vals) < 1: return
	# 	host = vals[0]
	# 	mess = ''
	# 	for k,v in hostinfo[host].items(): mess += k + ' = ' + str(v) + '\n'
	# 	messagebox.showinfo(host, mess)
	#
	# #=========================
	# # Add_hdrdmacp_HostInfo
	# def Add_hdrdmacp_HostInfo(self, hostinfo):
	# 	# Keep clean copy of most recent info from server indexed by host
	# 	host = hostinfo['host']
	# 	self.hostinfo[host] = hostinfo
	#
	# 	ram_disk_percent_free = 100.0-float(hostinfo['/media/ramdisk_percent_used'])
	#
	# 	# Modify some fields for presentation in listbox
	# 	myhostinfo = dict.copy(hostinfo)
	# 	myhostinfo['avg5min'] = '%3.1fGB/s' % float(hostinfo['avg5min'])
	# 	myhostinfo['10 sec. avg.'] = '%3.2fGB/s' % float(hostinfo['avg10sec'])
	# 	myhostinfo['1 min. avg.'] = '%3.2fGB/s' % float(hostinfo['avg1min'])
	# 	myhostinfo['5 min. avg.'] = '%3.2fGB/s' % float(hostinfo['avg5min'])
	# 	myhostinfo['Received Total'] = '%3.3fTB' % float(hostinfo['TB_received'])
	# 	myhostinfo['RAM disk free'] = '%3.1f GB (%3.1f%%)' % (myhostinfo['/media/ramdisk_avail'], ram_disk_percent_free)
	# 	myhostinfo['RAM free'] = '%3.1f%%' % (100.0*float(myhostinfo['ram_avail_GB'])/float(myhostinfo['ram_tot_GB']))
	# 	myhostinfo['idle'] = ('%3.1f' % myhostinfo['cpu_idle']) + '%'
	# 	self.hostslb1._upsrt_row(myhostinfo)
	#
	# #=========================
	# # Add_HOSS_HostInfo
	# def Add_HOSS_HostInfo(self, hostinfo):
	# 	# Keep clean copy of most recent info from server indexed by host
	# 	host = hostinfo['host']
	# 	self.hostinfo_HOSS[host] = hostinfo
	#
	# 	tbusy = float(hostinfo['tbusy'])
	# 	tidle = float(hostinfo['tidle'])
	# 	try:
	# 		# Replace single quotes with double quotes so json.loads is happy then count total errors
	# 		Nerrs = sum( json.loads(hostinfo['errors'].replace("'",'"')).values() )
	# 	except:
	# 		traceback.print_exc()
	# 		print("myhostinfo['errors'] = '" + hostinfo['errors'] + "'")
	# 		Nerrs = 0
	#
	# 	# Modify some fields for presentation in listbox
	# 	myhostinfo = dict.copy(hostinfo)
	# 	myhostinfo['Busy'] = '%3.1f%%' % (100.0*tbusy/(tbusy+tidle))
	# 	myhostinfo['tbusy'] = '%3.1fs' % tbusy
	# 	myhostinfo['tidle'] = '%3.1fs' % tidle
	# 	myhostinfo['Nprocs'] = myhostinfo['num_procs_total']
	# 	myhostinfo['Nerrs'] = Nerrs
	# 	myhostinfo['command'] = myhostinfo['last_cmd']
	#
	# 	if len(myhostinfo['command']) > 60:
	# 		myhostinfo['command'] = myhostinfo['command'][:57] + '...'
	#
	# 	self.hostslb2._upsrt_row(myhostinfo)

	#=========================
	# TimerUpdate
	#
	# This method is run in a separate thread and continuously loops to
	# update things outside of the mainloop.
	def TimerUpdate(self):
		while not DONE:
		
			# Send any queued commands
			for cmd in self.cmd_queue:
				try:
					SOCKET.send( cmd.encode() )
					message = SOCKET.recv()
					print('command: ' + cmd + '   -- result: ' + message.decode())
				except:
					pass
		
			# Clear command queue
			self.cmd_queue.clear()
		
			# Get status
			try:
				SOCKET.send( b'get_status')
				message = SOCKET.recv()
				info = json.loads( message.decode() )
				self.last_info = info
				#print(info)
				
				# Loop over top-level keys in the JSON record and any that
				# have a corresponding entry in the labmap member of the
				# class, set the value of the label.
				labkeys = self.labmap.keys()
				for k,v in info.items():
					if k in labkeys: self.labmap[k].set(v)
			except:
				break

			# Limit rate through loop
			time.sleep(0.5)

# #-----------------------------------------------------------------------------
# # MySubscriber_hdrdmacp
# #
# # This is run in each thread created to subscribe to a publisher host.
# # It makes the connection and subscribes to all messages then loops
# # continuously as long as the global DONE variable is set to True.
# def MySubscriber_hdrdmacp(context, host):
# 	global DONE, app
#
# 	print('Connecting to ' + host + ' ...')
#
# 	# Create socket
# 	socket = context.socket(zmq.SUB)
# 	socket.connect("tcp://" + host)
# 	socket.setsockopt_string(zmq.SUBSCRIBE, '') # Accept all messages from publisher
#
# 	while not DONE:
# 		try:
# 			string = socket.recv_string(flags=zmq.NOBLOCK)
# 			#print('Received string:\n' + string + '\n')
# 			myinfo = json.loads( string )
# 			myinfo['received'] = time.time()
#
# 			app.Add_hdrdmacp_HostInfo( myinfo )
#
# 		except zmq.Again as e:
# 			time.sleep(1)
#
# #-----------------------------------------------------------------------------
# # MySubscriber_HOSS
# #
# # This is run in each thread created to subscribe to a publisher host.
# # It makes the connection and subscribes to all messages then loops
# # continuously as long as the global DONE variable is set to True.
# def MySubscriber_HOSS(context, host):
# 	global DONE, app
#
# 	print('Connecting to ' + host + ' ...')
#
# 	# Create socket
# 	socket = context.socket(zmq.SUB)
# 	socket.connect("tcp://" + host)
# 	socket.setsockopt_string(zmq.SUBSCRIBE, '') # Accept all messages from publisher
#
# 	while not DONE:
# 		try:
# 			string = socket.recv_string(flags=zmq.NOBLOCK)
# 			#print('Received string:\n' + string + '\n')
# 			myinfo = json.loads( string )
# 			myinfo['received'] = time.time()
#
# 			app.Add_HOSS_HostInfo( myinfo )
#
# 		except zmq.Again as e:
# 			time.sleep(1)

#=============================================================================
#------------------- main  (rest of script is in global scope) ---------------

# Create window
root = Tk()
root.geometry("900x700")
Grid.rowconfigure(root, 0, weight=1)
Grid.columnconfigure(root, 0, weight=1)
app = MyWindow(root)

#  Only single 0MQ context is needed
context = zmq.Context()
connstr = "tcp://" + HOST + ":" + str(PORT)
print('Connecting to ' + connstr )
SOCKET = context.socket( zmq.REQ )
SOCKET.connect(connstr)

# Create a thread for periodic updates
threads = []
t = threading.Thread(target=app.TimerUpdate)
t.start()
threads.append(t)

# Run main GUI loop until user closes window
root.mainloop()

#-----------------------------------------------------------------------------
print('GUI finished. Cleaning up ...')

# Closing socket will force recv call to throw exception allowing thread to finish so it can be joined.
SOCKET.close(linger=0)
context.destroy(linger=0)

# Join all threads
DONE = True
for t in threads: t.join()


print('\nFinished\n')
