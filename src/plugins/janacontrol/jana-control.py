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
# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

import os
import sys
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

# TODO: Allow command line arguments to change host and port numbers
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
		msg = ttk.Label(self.parent, wraplength="4i", justify="left", anchor="n", padding=(10, 2, 10, 6), text=title, width=55)
		msg.grid(row=0, sticky=W+E)
		container = ttk.Frame(self.parent)
		container.grid(row=1, sticky=N+S+W+E)
		# create a treeview with dual scrollbars
		self.tree = ttk.Treeview(container,columns=self.columns, height=15, show="headings")
		# self.tree.column("#3", minwidth=400, width=400, stretch=True)
		vsb = ttk.Scrollbar(container,orient="vertical", command=self.tree.yview)
		hsb = ttk.Scrollbar(container,orient="horizontal", command=self.tree.xview)
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
			col_w = tkFont.Font().measure(val) + 10
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
# MyWindow   (The Main GUI)
class MyWindow(ttk.Frame):

	#=========================
	# __init__
	def __init__(self, master=None):
		Frame.__init__(self, master)
		self.master = master
		self.init_window()
		self.cmd_queue = []  # List of commands to send during TimerUpdate call
		self.debug_window = None

	#=========================
	# init_window
	def init_window(self):
	
		self.labelsFont = tkFont.Font(family="Helvetica", size=16)
		self.bannerFont = tkFont.Font(family="Comic Sans MS", size=36)
		self.labmap = {}
	
		self.master.title('JANA Status/Control GUI')
		self.grid(row=0, column=0, sticky=N+S+E+W ) # Fill root window
		Grid.rowconfigure(self, 0, weight=1)
		Grid.columnconfigure(self, 0, weight=1)

		#----- Banner
		bannerframe = ttk.Frame(self)
		bannerframe.grid( row=0, sticky=E+W)
		ttk.Label(bannerframe, anchor='center', text='JANA Status/Control GUI', font=self.bannerFont, background='white').grid(row=0, column=0, sticky=E+W)
		Grid.columnconfigure(bannerframe, 0, weight=1)

		#----- Information Section (Top)
		infoframe = ttk.Frame(self)
		infoframe.grid( row=1, sticky=N+S+E+W)

		# Process Info
		labels = { # keys are names in JSON record, vales are text labels for GUI
			'program':'Program Name',
			'host'   :'host',
			'PID'    :'PID',
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
		procinfoframe = ttk.Frame(infoframe)
		procinfoframe.grid( row=0, column=0, sticky=N+S+E+W, padx=30, ipadx=10 )
		for i,(varname,label) in enumerate(labels.items()):
			self.labmap[varname] = StringVar()
			ttk.Label(procinfoframe, anchor='e', text=label+': ', font=self.labelsFont).grid(row=i, column=0, sticky=E+W)
			ttk.Label(procinfoframe, anchor='w', textvariable=self.labmap[varname], font=self.labelsFont  ).grid(row=i, column=1, sticky=E+W)

		# Initialize all labels
		for key in self.labmap.keys(): self.labmap[key].set('---')

		# Factory Info (right)
		facinfoframe = ttk.Frame(infoframe)
		facinfoframe.grid( row=0, column=1, sticky=N+S+E+W, padx=30, ipadx=10 )
		self.facinfo_columns = ['FactoryName', 'FactoryTag', 'DataType', 'plugin']
		self.factory_info = MultiColumnListbox(facinfoframe, columns=self.facinfo_columns, title='Factories')

		#----- Control Section (Middle)
		controlframe = ttk.Frame(self)
		controlframe.grid( row=2, sticky=N+S+E+W)
		controlframe.columnconfigure(0, weight=1)

		# Nthreads
		nthreadsframe = ttk.Frame(controlframe)
		nthreadsframe.grid( row=0, sticky=E+W)
		ttk.Label(nthreadsframe, anchor='center', text='Num. threads', background='white').grid(row=0, column=0, columnspan=2, sticky=E+W)
		# Grid.columnconfigure(nthreadsframe, 0, weight=1)

		but1 = ttk.Button(nthreadsframe, text='--', command=self.DecrNThreads)
		but1.grid(row=1, column=0)
		but2 = ttk.Button(nthreadsframe, text='++', command=self.IncrNthreads)
		but2.grid(row=1, column=1)
		but3 = ttk.Button(controlframe, text='Debugger', command=self.OpenDebugger)
		but3.grid(row=0, column=2)
		but4 = ttk.Button(controlframe, text='test', command=self.Test)
		but4.grid(row=1, column=0, sticky=W)

		remotequitButton = ttk.Button(controlframe, text="Quit Remote",command=self.QuitRemote)
		remotequitButton.grid( row=2, column=0, columnspan=10, sticky=E+W )

		#----- GUI controls (Bottom)
		guicontrolframe = ttk.Frame(self)
		guicontrolframe.grid( row=3, sticky=E+W )
		guicontrolframe.columnconfigure(1, weight=1)

		closeButton = ttk.Button(guicontrolframe, text="Quit",command=self.Quit)
		closeButton.grid( row=0, column=2, sticky=E+W)
		closeButton.columnconfigure(0, weight=1)

		#===== Configure weights of layout
		Grid.rowconfigure(self, 0, weight=1)  # Info
		Grid.rowconfigure(self, 1, weight=10)  # Info
		Grid.rowconfigure(self, 2, weight=10)  # Control
		Grid.rowconfigure(self, 3, weight=1)   # GUI controls
		Grid.columnconfigure(self, 0, weight=1)

	#=========================
	# PrintResult
	#
	# This is used as the callback for simple commands that don't need to process
	# the return values, but would like a success or failure message to be
	# printed.
	def PrintResult(self, cmd, result):
		print('command: ' + cmd + '   -- result: ' + result)

	#=========================
	# Quit
	def Quit(self):
		global DONE, root
		DONE=True
		root.destroy()

	#=========================
	# QuitRemote
	def QuitRemote(self):
		self.cmd_queue.append( ('quit', self.PrintResult) )

	#=========================
	# DecrNThreads
	def DecrNThreads(self):
		self.cmd_queue.append( ('decrement_nthreads', self.PrintResult) )

	#=========================
	# IncrNthreads
	def IncrNthreads(self):
		self.cmd_queue.append( ('increment_nthreads', self.PrintResult) )

	#=========================
	# OpenDebugger
	def OpenDebugger(self):
		if not self.debug_window:
			self.debug_window = MyDebugWindow(self)
		self.debug_window.RaiseToTop()

	#=========================
	# Test
	def Test(self):
		self.cmd_queue.append( ('debug_mode 1', self.PrintResult) )
		self.cmd_queue.append( ('next_event', self.PrintResult) )
		self.cmd_queue.append( ('get_object_count', self.PrintResult) )

		if not self.debug_window:
			self.debug_window = MyDebugWindow(self)
		self.debug_window.RaiseToTop()

		# if not self.debug_window:
		# 	self.debug_window = Toplevel(self)
		# 	self.debug_window.title("New Window")
		# 	width = 400
		# 	height = 500
		# 	xoffset = self.master.winfo_x() + self.master.winfo_width()/2 - width/2
		# 	yoffset = self.master.winfo_y() + self.master.winfo_height()/2 - height/2
		# 	geometry = '%dx%d+%d+%d' % (width, height, xoffset, yoffset)
		# 	print('geometry='+geometry)
		# 	self.debug_window.geometry(geometry)
		# self.debug_window.focus_force()
		# self.debug_window.attributes('-topmost', True)
		# self.debug_window.after_idle(self.debug_window.attributes,'-topmost',False)

#=========================
	# SetProcInfo
	def SetProcInfo(self, cmd, result):
		# Remember most recently received proc info
		info = json.loads(result)
		self.last_info = info

		# Loop over top-level keys in the JSON record and any that
		# have a corresponding entry in the labmap member of the
		# class, set the value of the label.
		labkeys = self.labmap.keys()
		for k,v in info.items():
			if k in labkeys: self.labmap[k].set(v)

	#=========================
	# SetFactoryList
	def SetFactoryList(self, cmd, result):
		s = json.loads( result )
		if 'factories' in s :
			for finfo in s['factories']:
				# Make additional entries into the dictionary that use the column names so we can upsrt the listbox
				finfo['FactoryName'] = finfo['factory_name']
				finfo['FactoryTag'] = finfo['factory_tag']
				finfo['plugin'] = finfo['plugin_name']
				finfo['DataType'] = finfo['object_name']
				self.factory_info._upsrt_row(finfo)

	#=========================
	# TimerUpdate
	#
	# This method is run in a separate thread and continuously loops to
	# update things outside of the mainloop.
	def TimerUpdate(self):
		icntr = 0  # keeps track of iteration modulo 1000 so tasks can be performed every Nth iteration
		while not DONE:

			# Add some periodic tasks
			self.cmd_queue.append( ('get_status', self.SetProcInfo) )
			if icntr%4 == 0 : self.cmd_queue.append( ('get_factory_list', self.SetFactoryList) )

			# Send any queued commands
			while len(self.cmd_queue)>0:
				try:
					(cmd,callback) = self.cmd_queue.pop(0)
					try:
						SOCKET.send( cmd.encode() )
						message = SOCKET.recv()
						callback(cmd, message.decode())
						# print('command: ' + cmd + '   -- result: ' + message.decode())
					except:
						pass
				except:
					# self.cmd_queue.pop(0) raised exception, possibly due to having no more items
					break  # break while len(self.cmd_queue)>0 loop

			# Update icntr
			icntr += 1
			icntr = (icntr+1)%1000

			# Limit rate through loop
			time.sleep(0.5)

#-----------------------------------------------------------------------------
# MyDebugWindow   (The (optional) Debug GUI)
class MyDebugWindow(ttk.Frame):

	#=========================
	# __init__
	def __init__(self, parent):
		master = Toplevel(parent.master)
		ttk.Frame.__init__(self, master)
		self.master = master
		self.parent = parent
		self.master.title("JANA Debugger")

		# Center debug window on main GUI
		width = 800
		height = 600
		xoffset = self.parent.master.winfo_x() + self.parent.master.winfo_width()/2 - width/2
		yoffset = self.parent.master.winfo_y() + self.parent.master.winfo_height()/2 - height/2
		geometry = '%dx%d+%d+%d' % (width, height, xoffset, yoffset)
		self.master.geometry(geometry)
		self.init_window()
		self.cmd_queue = []  # List of commands to send during TimerUpdate call
		self.cmd_queue.append( ('debug_mode 1', self.PrintResult) ) # switch to debug mode when window is first created

		# Crete thread to call our TimerUpdate method
		t = threading.Thread(target=self.TimerUpdate)
		t.start()
		threads.append(t)  # append to global thread list (currently unused)

	#=========================
	# init_window
	def init_window(self):

		self.master.title('JANA Debugger')
		self.grid(row=0, column=0, sticky=N+S+E+W ) # Fill root window
		Grid.rowconfigure(self.master, 0, weight=1)
		Grid.columnconfigure(self.master, 0, weight=1)

		self.labelsFont = self.parent.labelsFont
		self.bannerFont = self.parent.bannerFont
		self.labmap = {}

		#----- Banner
		bannerframe = ttk.Frame(self.master)
		bannerframe.grid( row=0, sticky=E+W)
		ttk.Label(bannerframe, anchor='center', text='JANA Debugger', font=self.bannerFont, background='white').grid(row=0, column=0, sticky=E+W)
		Grid.columnconfigure(bannerframe, 0, weight=1)

		#----- Factory Info
		infoframe = ttk.Frame(self.master)
		infoframe.grid( row=1, sticky=N+S+E+W)
		facinfoframe = ttk.Frame(infoframe)
		facinfoframe.grid( row=0, column=0, sticky=N+S+E+W, padx=30, ipadx=10 )
		self.facinfo_columns = ['FactoryName', 'FactoryTag', 'DataType', 'plugin', 'Num. Objects']
		self.factory_info = MultiColumnListbox(facinfoframe, columns=self.facinfo_columns, title='Factories')

		#----- GUI controls (Bottom)
		guicontrolframe = ttk.Frame(self.master)
		guicontrolframe.grid( row=10, sticky=E+W )
		guicontrolframe.columnconfigure(0, weight=1)

		closeButton = ttk.Button(guicontrolframe, text="Close",command=self.HideWindow)
		closeButton.grid( row=0, column=0, sticky=E)
		closeButton.columnconfigure(0, weight=1)

		#===== Configure weights of layout
		Grid.rowconfigure(self.master, 0, weight=1 )  # bannerframe
		Grid.rowconfigure(self.master, 1, weight=10)  # infoframe
		Grid.rowconfigure(self.master, 3, weight=1 )  # guicontrolframe
		Grid.columnconfigure(self.master, 0, weight=1)

	#=========================
	# HideWindow
	def HideWindow(self):
		self.master.withdraw()

	#=========================
	# RaiseToTop
	def RaiseToTop(self):
		self.master.deiconify()
		self.master.focus_force()
		self.master.attributes('-topmost', True)
		self.after_idle(self.master.attributes,'-topmost',False)

	#=========================
	# TimerUpdate
	#
	# This method is run in a separate thread and continuously loops to
	# update things outside of the mainloop.
	def TimerUpdate(self):
		icntr = 0  # keeps track of iteration modulo 1000 so tasks can be performed every Nth iteration
		while not DONE:

			# Add some periodic tasks
			if icntr%2 == 0 : self.cmd_queue.append( ('get_object_count', self.PrintResult) )

			# Send any queued commands
			while len(self.cmd_queue)>0:
				try:
					(cmd,callback) = self.cmd_queue.pop(0)
					try:
						SOCKET.send( cmd.encode() )
						message = SOCKET.recv()
						callback(cmd, message.decode())
					# print('command: ' + cmd + '   -- result: ' + message.decode())
					except:
						pass
				except:
					# self.cmd_queue.pop(0) raised exception, possibly due to having no more items
					break  # break while len(self.cmd_queue)>0 loop

			# Update icntr
			icntr += 1
			icntr = (icntr+1)%1000

			# Limit rate through loop
			time.sleep(0.5)

	#=========================
	# PrintResult
	#
	# This is used as the callback for simple commands that don't need to process
	# the return values, but would like a success or failure message to be
	# printed.
	def PrintResult(self, cmd, result):
		print('command: ' + cmd + '   -- result: ' + result)

	#=========================
	# SetFactoryObjectCountList
	def SetFactoryObjectCountList(self, cmd, result):
		s = json.loads( result )
		if 'factories' in s :
			for finfo in s['factories']:
				# Make additional entries into the dictionary that use the column names so we can upsrt the listbox
				finfo['FactoryName'] = finfo['factory_name']
				finfo['FactoryTag'] = finfo['factory_tag']
				finfo['plugin'] = finfo['plugin_name']
				finfo['DataType'] = finfo['object_name']
				finfo['Num. Objects'] = finfo['nobjects']
				self.factory_info._upsrt_row(finfo)


#=============================================================================
#------------------- main  (rest of script is in global scope) ---------------

os.environ['TK_SILENCE_DEPRECATION'] = '1'  # Supress warning on Mac OS X about tkinter going away

# Create window
root = Tk()
style = ttk.Style()
style.theme_use('aqua')
root.geometry("900x700+2500+0")
Grid.rowconfigure(root, 0, weight=1)
Grid.columnconfigure(root, 0, weight=1)
app = MyWindow(root)

#  Only single 0MQ context is needed
context = zmq.Context()
connstr = "tcp://" + HOST + ":" + str(PORT)
print('Connecting to ' + connstr )
SOCKET = context.socket( zmq.REQ )
SOCKET.connect(connstr)

# Create a thread for periodic updates of main GUI window
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
