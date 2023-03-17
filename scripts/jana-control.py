#!/usr/bin/env python3
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
import time
from tkinter import *
import tkinter.ttk as ttk
import tkinter.font as tkFont
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
    def __init__(self, parent, columns, title=None, titlevar=None):
        self.parent = parent
        self.columns = columns
        self.item_map = {} # key=hostname val=item returned by insert
        self.tree = None
        self._setup_widgets(title, titlevar)
        self._build_tree()

    def _setup_widgets(self, title, titlevar):
        self.parent.columnconfigure(0, weight=1)
        self.parent.rowconfigure(0, weight=1) # Title
        self.parent.rowconfigure(1, weight=10) # TreeView, scrollbars

        if title:
            msg = ttk.Label(self.parent, wraplength="4i", justify="left", anchor="n", padding=(10, 2, 10, 6), text=title, width=55)
        elif titlevar:
            msg = ttk.Label(self.parent, wraplength="4i", justify="left", anchor="n", padding=(10, 2, 10, 6), textvariable=titlevar, width=55)
        if 'msg' in locals() : msg.grid(row=0, column=0, sticky=W+E)

        # Container to hold treeview and scrollbars
        container = ttk.Frame(self.parent)
        container.grid(row=1, column=0, sticky=N+S+W+E)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(0, weight=1)

        # create a treeview with dual scrollbars
        self.tree = ttk.Treeview(container,columns=self.columns, height=15, show="headings")
        vsb = ttk.Scrollbar(container,orient="vertical", command=self.tree.yview)
        hsb = ttk.Scrollbar(container,orient="horizontal", command=self.tree.xview)
        self.tree.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        self.tree.grid(column=0, row=0, sticky='nsew', in_=container)
        vsb.grid(column=1, row=0, sticky='ns', in_=container)
        hsb.grid(column=0, row=1, sticky='ew', in_=container)

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
        if len(item) > 1 : mykey += ':' + item[1]
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

    def clear(self):
        self.tree.delete(*self.tree.get_children()) # delete all items in tree
        self.item_map={}

#-----------------------------------------------------------------------------
# MyWindow   (The Main GUI)
class MyWindow(ttk.Frame):

    #=========================
    # __init__
    def __init__(self, master=None):
        ttk.Frame.__init__(self, master)
        self.master = master
        self.init_window()
        self.cmd_queue = []  # List of commands to send during TimerUpdate call
        self.debug_window = None

    #=========================
    # init_window
    def init_window(self):
	 
        # Find a font family for the banner
        preferred_fonts = ['Comic Sans MS', 'URW Gothic', 'Courier']
        available_fonts=list(tkFont.families())
        self.bannerFont = None
        for f in preferred_fonts:
            for af in available_fonts:
                if af.startswith( f ):
                    self.bannerFont = tkFont.Font(family=af, size=36)
                    break
            if self.bannerFont : break
        if self.bannerFont == None : self.bannerFont = tkFont.Font(family=preferred_fonts[-1], size=36)
            
        self.labelsFont = tkFont.Font(family="Helvetica", size=16)
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
        # but4 = ttk.Button(controlframe, text='test', command=self.Test)
        # but4.grid(row=1, column=0, sticky=W)

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
        # This is trickier than you might first guess. If the debug window has been
        # opened, then the remote process may be in "debug mode" so event processing
        # is blocked. In this case, we want to release it before we disconnect by
        # sending it a message to go out of debug mode. Messages are sent by placing
        # them on the command_queue and letting TimerThread handle it. If we are not
        # actually connected to a remote process, then the ThreadTimer routine is
        # likely blocking on the SOCKET.recv() and will only continue if we close the
        # socket. This boils down to wanting to either send a message on the socket
        # if it is connected or close the socket if it is not. Unfortunately, zmq
        # does not provide a way to check this (it kind of goes against the some core
        # design of the REQ-REP model). Thus, what we do to handle all cases is
        # queue up a message to be sent and arrange for the Cleanup method to be called
        # after a short time in hopes that the message makes it out in time
        print('Quitting ...')
        if self.debug_window:
            self.cmd_queue.append( ('debug_mode 0', self.PrintResult) )
            self.after(2000, self.Cleanup)
        else:
            self.Cleanup()
        # time.sleep(0.75) # give TimerThread time to do one iteration so command to exit debug mode is sent to remote process
        # Tell TimerThread method to exit at next iteration.
        # Note that we defer stopping the main loop until the end of ThreadTimer
        # and defer actually destroying the window until the main loop is stopped.
        # DONE=True
        # print('Quitting ...')

    #=========================
    # Cleanup
    def Cleanup(self):
        global DONE, SOCKET
        # This is called to force the TimerThread routine to quit its loop and exit cleanly.
        print('  - closing ZMQ socket ...')
        DONE=True
        if SOCKET:
            SOCKET.close(linger=0)    # This will un-block the recv call if it is stuck there
            context.destroy(linger=0)
            SOCKET = None

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
        self.cmd_queue.append( ('debug_mode 1', self.PrintResult) ) # Go into debug mode

    #=========================
    # Test
    def Test(self):
        self.cmd_queue.append( ('debug_mode 1', self.PrintResult) )
        self.cmd_queue.append( ('next_event', self.PrintResult) )
        self.cmd_queue.append( ('get_object_count', self.PrintResult) )

        if not self.debug_window:
            self.debug_window = MyDebugWindow(self)
        self.debug_window.RaiseToTop()

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
            if self.debug_window :
                if icntr%2 == 0 : self.cmd_queue.append( ('get_object_count', self.debug_window.SetFactoryObjectCountList) )

            # Send any queued commands
            while len(self.cmd_queue)>0:
                try:
                    (cmd,callback) = self.cmd_queue.pop(0)
                    try:
                        SOCKET.send( cmd.encode() )
                        message = SOCKET.recv()
                        callback(cmd, message.decode())
                    except:
                        pass
                except:
                    # self.cmd_queue.pop(0) raised exception, possibly due to having no more items
                    break  # break while len(self.cmd_queue)>0 loop

            # Update icntr
            icntr += 1
            icntr = (icntr+1)%1000

            # Limit rate through loop
            if DONE: break
            time.sleep(0.5)

        # Done with timer thread which must mean we are exiting. Tell mainloop() to exit.
        root.quit()

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
        width = 1200
        height = 700
        xoffset = self.parent.master.winfo_x() + self.parent.master.winfo_width()/2 - width/2
        yoffset = self.parent.master.winfo_y() + self.parent.master.winfo_height()/2 - height/2
        geometry = '%dx%d+%d+%d' % (width, height, xoffset, yoffset)
        self.master.geometry(geometry)
        self.labmap = {}
        self.init_window()

        self.Stop() # switch to debug mode when window is first created

    #=========================
    # init_window
    def init_window(self):

        self.master.title('JANA Debugger')
        self.master.rowconfigure(0, weight=1)
        self.master.columnconfigure(0, weight=1)
        self.grid(row=0, column=0, sticky=N+S+E+W , ipadx=10, ipady=10, padx=10, pady=10) # Fill root window
        self.rowconfigure(0, weight=1 )  # bannerframe
        self.rowconfigure(1, weight=1)   # eventinfoframe
        self.rowconfigure(2, weight=50)  # infoframe
        self.rowconfigure(3, weight=1)   # controlframe
        self.rowconfigure(10, weight=1 )  # guicontrolframe
        self.columnconfigure(0, weight=1)

        self.labelsFont = self.parent.labelsFont
        self.bannerFont = self.parent.bannerFont

        #----- Banner
        bannerframe = ttk.Frame(self)
        bannerframe.grid( row=0, column=0, sticky=E+W)
        bannerframe.columnconfigure( 0, weight=1)
        ttk.Label(bannerframe, anchor='center', text='JANA Debugger', font=self.bannerFont, background='white').grid(row=0, column=0, sticky=E+W)

        #----- Event Info
        eventinfoframe = ttk.Frame(self)
        eventinfoframe.grid( row=1, column=0, sticky=N+S+E+W)

        # Process Info
        labels = { # keys are names in JSON record, vales are text labels for GUI
            'program':'Program Name',
            'host'   :'host',
            'PID'    :'PID',
            'run_number': 'Run Number',
            'event_number': 'Event Number',
        }
        procinfoframe = ttk.Frame(eventinfoframe)
        procinfoframe.grid( row=0, column=0, sticky=N+S+E+W, padx=30 )
        for i,(varname,label) in enumerate(labels.items()):
            self.labmap[varname] = StringVar()
            ttk.Label(procinfoframe, anchor='e', text=label+': ', font=self.labelsFont).grid(row=i, column=0, sticky=E+W)
            ttk.Label(procinfoframe, anchor='w', textvariable=self.labmap[varname]  ).grid(row=i, column=1, sticky=E+W)

        # Initialize all labels
        for key in labels.keys(): self.labmap[key].set('---')

        #----- Factory Info
        infoframe = ttk.Frame(self)
        infoframe.grid(row=2, column=0, sticky=N+S+E+W, ipadx=5)
        infoframe.rowconfigure(0, weight=1)
        infoframe.columnconfigure(0, weight=1)  # facinfoframe
        infoframe.columnconfigure(1, weight=1)   # objinfoframe
        infoframe.columnconfigure(2, weight=1)  # objfieldsframe

        facinfoframe = ttk.Frame(infoframe, style='FrameYellow.TFrame')
        facinfoframe.grid( row=0, column=0, sticky=N+S+E+W)
        self.facinfo_columns = ['FactoryName', 'FactoryTag', 'DataType', 'plugin', 'Num. Objects']
        self.factory_info = MultiColumnListbox(facinfoframe, columns=self.facinfo_columns, title='Factories')
        self.factory_info.tree.bind("<<TreeviewSelect>>", self.OnSelectFactory)

        # Object Info
        objinfoframe = ttk.Frame(infoframe)
        objinfoframe.grid( row=0, column=1, sticky=N+S+E+W)
        self.labmap['object_type'] = StringVar()
        self.objinfo_columns = ['address']
        self.object_info = MultiColumnListbox(objinfoframe, columns=self.objinfo_columns, titlevar=self.labmap['object_type'])
        self.object_info.tree.bind("<<TreeviewSelect>>", self.OnSelectObject)

        # Object fields
        objfieldsframe = ttk.Frame(infoframe)
        objfieldsframe.grid( row=0, column=2, sticky=N+S+E+W)
        self.labmap['field_object_addr'] = StringVar()
        self.objfields_columns = ['name','value', 'type']
        self.object_fields = MultiColumnListbox(objfieldsframe, columns=self.objfields_columns, titlevar=self.labmap['field_object_addr'])
        self.object_fields.tree.column(0, anchor=E)  # Make name column right justified
        self.object_fields.tree.column(1, anchor=E)  # Make value column right justified

        #----- Control Section (Middle)
        controlframe = ttk.Frame(self)
        controlframe.grid( row=3, column=0, sticky=N+S+E+W)
        controlframe.columnconfigure(0, weight=1)

        # Run/Stop
        runstopframe = ttk.Frame(controlframe)
        runstopframe.grid( row=0, column=0, sticky=E+W)
        ttk.Label(runstopframe, anchor='center', text='Run/Stop', background='white').grid(row=0, column=0, columnspan=2, sticky=E+W)
        self.labmap['runstop'] = StringVar()
        self.runstoplab = ttk.Label(runstopframe, anchor='center', textvariable=self.labmap['runstop'], background='#FF0000')
        self.runstoplab.grid(row=1, column=0, columnspan=2, sticky=E+W)
        ttk.Button(runstopframe, text='Stop', command=self.Stop).grid(row=2, column=0)
        ttk.Button(runstopframe, text='Run',  command=self.Run ).grid(row=2, column=1)
        ttk.Button(runstopframe, text='Next Event',  command=self.NextEvent ).grid(row=3, column=0, columnspan=2)

        #----- GUI controls (Bottom)
        guicontrolframe = ttk.Frame(self)
        guicontrolframe.grid( row=10, column=0, sticky=E+W )
        guicontrolframe.columnconfigure(0, weight=1)

        closeButton = ttk.Button(guicontrolframe, text="Close",command=self.HideWindow)
        closeButton.grid( row=0, column=0, sticky=E)
        closeButton.columnconfigure(0, weight=1)

    #=========================
    # HideWindow
    def HideWindow(self):
        self.parent.cmd_queue.append( ('debug_mode 0', self.parent.PrintResult) )
        self.master.withdraw()

    #=========================
    # RaiseToTop
    def RaiseToTop(self):
        self.master.deiconify()
        self.master.focus_force()
        self.master.attributes('-topmost', True)
        self.after_idle(self.master.attributes,'-topmost',False)

    #=========================
    # Run
    def Run(self):
        self.parent.cmd_queue.append( ('debug_mode 0', self.parent.PrintResult) )
        self.running = True
        self.labmap['runstop'].set('running')
        self.runstoplab.config(background='#00FF00')

    #=========================
    # Stop
    def Stop(self):
        self.parent.cmd_queue.append( ('debug_mode 1', self.parent.PrintResult) )
        self.running = False
        self.labmap['runstop'].set('stopped')
        self.runstoplab.config(background='#FF0000')

    #=========================
    # NextEvent
    def NextEvent(self):
        self.parent.cmd_queue.append( ('next_event', self.parent.PrintResult) )

    #=========================
    # OnSelectFactory
    def OnSelectFactory(self, event):
        for item in self.factory_info.tree.selection():
            row = self.factory_info.tree.item(item)
            if row :
                vals = row['values']
                self.labmap['object_type'].set(vals[2])
                cmd = 'get_objects %s %s %s' % (vals[2], vals[0], vals[1]) #  object_name factory_name factory_tag
                print('cmd='+cmd)
                self.parent.cmd_queue.append( (cmd, self.SetFactoryObjectList) )
                self.object_fields.clear() # clear object fields 

    #=========================
    # OnSelectObject
    def OnSelectObject(self, event):
        for item in self.object_info.tree.selection():
            row = self.object_info.tree.item(item)
            if row:
                addr = row['values'][0]
                label = addr + ' (' + self.labmap['object_type'].get() + ')'
                self.labmap['field_object_addr'].set(label)
                s = self.last_object_list
                if addr in s['objects'].keys():
                    fields = s['objects'][addr]
                    self.object_fields.clear()
                    for row in fields:
                        self.object_fields._upsrt_row(row)

    #=========================
    # SetFactoryObjectCountList
    #
    # This is called when data is received that contains a list of factories and their
    # object counts for a specific event. Other metadata like run, event, procid, and
    # host (if it is included) is used to update the labels at the top of the debugger
    # window.
    #
    # This also checks if any of run,event, or procid has changed and if so, calls
    # UpdateNewEvent() to update other TreeViews.
    def SetFactoryObjectCountList(self, cmd, result):
        s = json.loads( result )

        # Set host/proc/event info/ (assume any JSON keys that match keys in labmap should be set)
        labkeys = self.labmap.keys()
        run_changed = False
        event_changed = False
        procid_changed = False
        for k,v in s.items():
            if k == 'run_number': run_changed = (v != self.labmap[k].get())
            if k == 'event_number': event_changed = (v != self.labmap[k].get())
            if k == 'procid': procid_changed = (v != self.labmap[k].get())
            if k in labkeys: self.labmap[k].set(v)

        if 'factories' in s :
            for finfo in s['factories']:
                # Make additional entries into the dictionary that use the column names so we can upsrt the listbox
                finfo['FactoryName'] = finfo['factory_name']
                finfo['FactoryTag'] = finfo['factory_tag']
                finfo['plugin'] = finfo['plugin_name']
                finfo['DataType'] = finfo['object_name']
                finfo['Num. Objects'] = finfo['nobjects']
                self.factory_info._upsrt_row(finfo)

        # Call UpdateNewEvent if any of run,event, or procid changed
        if run_changed or event_changed or procid_changed: self.UpdateNewEvent()

    #=========================
    # UpdateNewEvent
    #
    # This is called when a new run/event/procid combination is received and identified
    # in SetFactoryObjectCountList(). Its purpose is to update the various TreeViews
    def UpdateNewEvent(self):
        # Clear the object field TreeView
        self.object_fields.clear()
        self.last_object_list = None

        # Only clear object addresses if there is no factory selected. If one is selected, then
        # we want to preserve any selected object address so the fields can be updated later when
        # SetFactoryObjectList() gets called after a new set of objects is received. A request
        # for new objects is automatically issued when self.factory_info.tree.selection_set(item)
        # triggers OnSelectFactory() above.
        self.object_info.clear()
        # if len(self.factory_info.tree.selection()) == 0:
        # 	self.object_info.clear()

        # For any currently selected factory, re-select it so the OnSelectFactory() gets called
        # the objects list gets updated
        for item in self.factory_info.tree.selection():
            self.factory_info.tree.selection_set(item)

    #=========================
    # SetFactoryObjectList
    #
    # This fills the TreeView of object addresses. If an address is currently selected that
    # is not in the list of addresses we are inserting, then the object_fields TreeView
    # is cleared. If it is, then the object is re-selected to trigger re-displaying the
    # object fields. This is needed in the case that an address is recycled and the contents
    # have actually changed.
    def SetFactoryObjectList(self, cmd, result):
        # Check if there is a currently selected address and remember it if there is
        addr = None
        items = self.object_info.tree.selection()
        if items:
            addr = self.object_info.tree.item(items[0])['values'][0] # copy value of first selected item

        try:
            s = json.loads( result )
        except json.decoder.JSONDecodeError as e:
            print('ERROR decoding JSON: ' + e.msg)
        self.last_object_list = s
        self.object_info.clear() # delete all items in tree
        for a in s['objects'].keys():
            row = {'address':a}
            self.object_info._upsrt_row(row)
            if a == addr:
                # Address was already selected. Re-select it now
                item_to_select = self.object_info.tree.get_children()[-1]
        if item_to_select: self.object_info.tree.selection_set(item_to_select)

#=========================
# Usage
def Usage():
    mess='''
    Usage:
            jana-control.py [--help] [--host HOST] [--port PORT]
    
This script will open a GUI window that will monitor a running JANA process.
The process can be running on either the local node or a remote node. For
this to work, the following criteria must be met:

1. JANA must have been compiled with ZEROMQ support. (This relies
   on cmake find_package(ZEROMQ) successfully finding it when camke
   is run.)

2. The python3 environment must me present and have zmq installed.
   (e.g. pip3 install zmq)

3. The JANA process must have been started with the janacontrol plugin.
   This should generally be added to the *end* of the plugin list
   like this:
   
      -Pplugins=myplugin1,myplugin2,janacontrol

By default, it will try to attach to port 11238 on the localhost. It
does not matter whether the JANA process is already running or not.
It will automatically connect when it does and reconnect if the process
is restarted.

The following command line options are available:

-h, --help     Print this Usage statement and exit
--host HOST    Set the host of the JANA process to monitor
--port PORT    Set the port on the host to connect to

n.b. To change the port used by the remote JANA process set the
jana:zmq_port configuration parameter.

Debugger
--------------
The GUI can be used to step through events in the JANA process and
view the objects, with some limitations. If the data object inherits
from JObject then it will display fields obtained from the subclass'
Summarize method. (See JObject::Summarize for details). If the data
object inherits from ROOT's TObject then an attempt is made to extract
the data members via the dictionary. Note that this relies on the
dictionary being available in the plugin and there are limitations
to the complexity of the objects that can be displayed.

When the debugger window is opened (by pushing the "Debugger" button
on the main GUI window), it will stall event processing so that single
events can be examined and stepped through. To stall processing on the
very first event, the JANA process should have the jana:debug_mode
config. parameter set to a non-zero value when it is started. e.g.

jana -Pplugins=myplugin1,myplugin2,janacontrol -Pjana:debug_mode=1 file1.dat

Once an event is loaded, click on a factory to display a list of 
objects it created for this event (displayed as the object's hexadecimal
address). Click on an object to display its content summary (if they
are accessible). n.b. clicking on a factory will NOT cause the factory
algorithm to activate and create objects for the event. It will only
display objects created by other plugins having activated the algorithm.
    '''
    return mess


#=============================================================================
#------------------- main  (rest of script is in global scope) ---------------


# Parse the command line parameters
if any(item in ['-h', '-help', '--help'] for item in sys.argv):
    print( Usage() )
    sys.exit()
Nargs = len(sys.argv)
for i,arg in enumerate(sys.argv):
    if i==0 : continue # skip script name
    if (arg=='--host') and (i+1<Nargs): HOST = sys.argv[i+1]
    if (arg=='--port') and (i+1<Nargs): PORT = sys.argv[i+1]

print('\nAttempting connection on host="'+HOST+'" port='+str(PORT))
print('For help, run "jana-control.py --help"\n')

os.environ['TK_SILENCE_DEPRECATION'] = '1'  # Supress warning on Mac OS X about tkinter going away

# Create window
root = Tk()
style = ttk.Style()
style.theme_use('classic')
style.configure('FrameRed.TFrame',   background='red'  )  #useful for debugging
style.configure('FrameGreen.TFrame', background='green')  #useful for debugging
style.configure('FrameBlue.TFrame',  background='blue' )  #useful for debugging
style.configure('FrameCyan.TFrame',  background='cyan' )  #useful for debugging
style.configure('FrameYellow.TFrame',  background='yellow' )  #useful for debugging
style.configure('FrameViolet.TFrame',  background='violet' )  #useful for debugging
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
root.destroy()

#-----------------------------------------------------------------------------
print('GUI finished. Cleaning up ...')

print('  - joining threads ...')
for t in threads: t.join()

print('\nFinished\n')
