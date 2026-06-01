#!/usr/bin/env python3
# @file thlamcp.py
# @brief A simple Python TK interface for testing TrickHLA Master control.
#
# This is a Python program used to used for simple control and testing of
# TrickHLA 
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, June 2026, --, Initial creation.}
# @revs_end
#
import os
import sys

import argparse
from tkinter import *
from tkinter import ttk

# Check to see if the $TRICKHLA_HOME environment variable is defined and set.
trick_home = os.environ.get( 'TRICK_HOME' )
if trick_home is None:
   sys.exit( '\033[91m'\
            +'Environment variable TRICK_HOME is not defined!'\
            +'\033[0m\n' )
else:
   if os.path.isdir( trick_home ) is False:
      sys.exit( '\033[91m'\
               +'TRICKHLA_HOME not found: '\
               +trick_home\
               +'\033[0m\n' )
      
# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trick_home not in sys.path :
   sys.path.append( trick_home + '/share/trick/pymods/trick' )

# Check to see if the $TRICKHLA_HOME environment variable is defined and set.
trickhla_home = os.environ.get( 'TRICKHLA_HOME' )
if trickhla_home is None:
   sys.exit( '\033[91m'\
            +'Environment variable TRICKHLA_HOME is not defined!'\
            +'\033[0m\n' )
else:
   if os.path.isdir( trickhla_home ) is False:
      sys.exit( '\033[91m'\
               +'TRICKHLA_HOME not found: '\
               +trickhla_home\
               +'\033[0m\n' )

# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trickhla_home not in sys.path :
   sys.path.append( trickhla_home )

# Import the Trick Variable Server connection module.
from variable_server import *

# Configure the commman line argument parser.
parser = argparse.ArgumentParser( description='TrickHLA Master Control Input Arguments',\
                                  formatter_class=argparse.RawTextHelpFormatter)

parser.add_argument( '-a', '--auto',\
                     help='Auto connect to simulation at startup. (default = False)',
                     action = "store_true" )

parser.add_argument( '-c', '--host',\
                     help='Define the host the simulation is running on. (default = None)',
                     default= None )

parser.add_argument( '-p', '--port',\
                     help = 'Define the port the simulation is running on. (default = None)',
                     default = 0 )

inputArgs = parser.parse_args()

# The main TrickHLA Master Control Panel class
class MasterControl( Frame ):

   variable_server = None
   hostname = None
   port = 0
   sim_connected = False
   sim_mode = -1
   prev_sim_mode = -1
   time_tics = 0
   time_tic_value = 1000000
   sim_time = 0.0
   prev_sim_time = -0.1
   scenario_time = 0.0
   hla_logical_time = 0

   # The Connect button activation function
   def connectToSim( self ):

      # Pull the host and port information from the entry fields.
      self.hostname = self.host_var.get()
      if self.hostname == 'localhost' :
         self.hostname = '127.0.0.1'
      try:
         self.port = int(self.port_var.get())
      except Exception as e:
         root.bell()
         print( e )
         self.port = 0
         self.port_var.set( str(self.port) )

      # Try to make a connection to a Trick Variable Server.
      try:

         # Check for three connection cases:
         # 1. Nothing is set: Find any simulation (first found).
         # 2. Host is set: Find the simulation running in the host.
         # 3. Host and Port set: Connect to simulation in specific host and port.
         # NOTE: Setting a port without a host is ignored.
         if ( self.hostname ):
           if (self.port != 0):
               print( 'Connecting to host:port: ' + self.hostname + ':' + str( self.port ) )
               self.variable_server = find_simulation( host = self.hostname, 
                                                       port = self.port,
                                                       timeout = 10.0 )
           else:
               print( 'Connecting to host: ' + self.hostname  )
               self.variable_server = find_simulation( host = self.hostname,
                                                       timeout = 10.0 )
         else:
            print( 'Connecting to any simulation: ' )
            self.variable_server = find_simulation( timeout = 10.0 )
           
      except Exception as e:
         print( e )
         root.bell()
         if ( self.hostname ):
            if (self.port != 0):
               print(   'Connection timed out to host:port: ' + self.hostname
                      + ':' + str( self.port) + '!' )
            else:
               print( 'Connection timed out to host: ' + self.hostname + '!' )
         else:
            print( 'Connection timed out to any simulation!' )
      
      if self.variable_server != None :

         # Disable the connect and enable the disconnect.
         self.connect_button.config( state = 'disabled' )
         self.disconnect_button.config( state = 'active', fg = 'red' )

         # If I don't do this, sometimes I don't get button state updates.
         root.update_idletasks()

      else:
         print( 'Did not connect to sim.')

         # Mark as NOT connected to simulation.
         self.sim_connected = False
         self.connect_button['state']    = 'active'
         self.disconnect_button['state'] = 'disabled'

         return

      # Update the variable server information.
      self.updateVariableServerInfo()

      # Mark as connected to simulation.
      self.sim_connected = True

      # Let users know how we connected.
      print( 'Connected to host:port: ' + self.hostname + ':' + str( self.port ) )

      # Update the simulation execution state information.
      self.updateSimState()

      return
   

   # The Connect button activation function
   def disconnectFromSim( self ):
      self.closeVarServerConnection()
      return

   #
   def updateVariableServerInfo( self ):
      if not self.variable_server :
         print( 'No variable server connection!')
         return
      
      try:
         vs_host_addr = self.variable_server.get_value( 'trick_vs.vs.listen_thread._requested_source_address' )
      except Exception as e:
         root.bell()
         print( e )
      
      try:
         vs_port = self.variable_server.get_value( 'trick_vs.vs.listen_thread._requested_port' )
      except Exception as e:
         root.bell()
         print( e )
      
      # Compare specified host name to Trick VS host name.
      if ( vs_host_addr != self.hostname ) :
         self.hostname = vs_host_addr
         self.host_var.set( self.hostname )

      # Check the port address.
      if ( self.port == 0 ):
         self.port = int( vs_port )
         self.port_var.set( str(self.port) )
      elif ( self.port != int( vs_port ) ):
         self.port = int( vs_port )
         self.port_var.set( str(self.port) )
      
      return


   #
   def updateSimState( self ):

      # Don't do anything if we don't have a simulation connection.
      if not self.variable_server :
         self.connect_button['state'] = 'active'
         # Reschedule the updateTimeScales function.
         self.after( 250, self.updateSimState )
         return
      
      # Get the current simulation execution mode.
      try:
         self.sim_mode = self.variable_server.get_value( 'trick_sys.sched.mode', type_=int )
      except Exception as e:
         root.bell()
         print( e )
         # Reschedule the updateSimState function.
         self.after( 250, self.updateSimState )
         return
      
      # There's latency in the sim_mode update.  So, update the time in freeze even
      # if the mode change has already been registered.
      if ( self.prev_sim_time != self.sim_time ) :
         self.prev_sim_time = self.sim_time
         if ( self.sim_mode == 1 ):
            # Set the default Save label to the time.
            # FIXME: This needs to be the SST
            self.save_label_var.set( self.set_str_var.get() )

      # Check for change in simulation execution mode.
      if ( self.prev_sim_mode == self.sim_mode ):
         # Reschedule the updateSimState function.
         self.after( 250, self.updateSimState )
         return
      else:
         self.prev_sim_mode = self.sim_mode

      # Act on changes in simulation mode:
      match self.sim_mode:
         case 0: # Initialization
            self.run_button['state']      = 'disabled'
            self.freeze_button['state']   = 'disabled'
            self.shutdown_button['state'] = 'disabled'
            self.save_button['state']     = 'disabled'

         case 1: # Freeze
            self.run_button['state']      = 'active'
            self.freeze_button['state']   = 'disabled'
            self.shutdown_button['state'] = 'active'
            self.save_button['state']     = 'active'

         case 4: # Step
            self.run_button['state']      = 'active'
            self.freeze_button['state']   = 'disabled'
            self.shutdown_button['state'] = 'disabled'
            self.save_button['state']     = 'disabled'

         case 5: # Run
            self.run_button['state']      = 'disabled'
            self.freeze_button['state']   = 'active'
            self.shutdown_button['state'] = 'disabled'
            self.save_button['state']     = 'disabled'

         case 6: # Shutdown
            print( 'Simulation in SIM_MODE::ExitMode.')
            self.run_button['state']        = 'disabled'
            self.freeze_button['state']     = 'disabled'
            self.shutdown_button['state']   = 'disabled'
            self.save_button['state']       = 'disabled'
            self.connect_button['state']    = 'active'
            self.disconnect_button['state'] = 'disabled'

         case _:
            print( 'Unknown simulation execution mode: ' + str(self.sim_mode) )
            self.run_button['state']      = 'disabled'
            self.freeze_button['state']   = 'disabled'
            self.shutdown_button['state'] = 'disabled'
            self.save_button['state']     = 'disabled'

      # If I don't do this, sometimes I don't get button state updates.
      root.update_idletasks()

      # Reschedule the updateSimState function.
      self.after( 250, self.updateSimState )

      return

   #
   def updateTimeScales( self ):
      if not self.variable_server :
         # Reschedule the updateTimeScales function.
         self.after( 250, self.updateTimeScales )
         return
      
      try:
         self.time_tics = self.variable_server.get_value( 'trick_sys.sched.time_tics', type_=int )
      except Exception as e:
         root.bell()
         print( e )
         self.time_tics = 0

      try:
         self.time_tic_value = self.variable_server.get_value( 'trick_sys.sched.time_tic_value', type_=int )
      except Exception as e:
         root.bell()
         print( e )
         self.time_tic_value = 200000

      self.sim_time = self.time_tics / self.time_tic_value
      self.set_str_var.set( self.sim_time )

      # Reschedule the updateTimeScales function.
      self.after( 250, self.updateTimeScales )

      return

   #
   def goToRun( self ):
      try:
         self.variable_server.freeze( freeze=False )
      except Exception as e:
         root.bell()
         print( e )
   
      return

   #
   def goToFreeze( self ):
      try:
         self.variable_server.freeze( freeze=True )
      except Exception as e:
         root.bell()
         print( e )
   
      return

   #
   def federationSave( self ):

      # Construct the save command.
      save_label_str = '\"' + self.save_label_var.get() + '\"'
      save_command_str = 'THLA.federate.save(' + save_label_str + ')'
      try:
         print( 'MasterControl::federationSave: ' + save_command_str )
         self.variable_server.send( save_command_str )

      except Exception as e:
         root.bell()
         print( e )
   
      return

   #
   def federationShutdown( self ):
      print( 'Shutdown the federation execution!')
      try:
         self.variable_server.send( 'trick.stop()' )
      except Exception as e:
         root.bell()
         print( e )

      # Close the variable server connection.
      self.closeVarServerConnection()
   
      return

   #
   def applicationExit( self ):
      
      # Need to do some cleanup first
      self.closeVarServerConnection()
   
      # Quit the application.
      self.quit()

      return

   #
   def closeVarServerConnection( self ):

      # Just return if the sim is not connected.
      if ( not self.sim_connected ):
         return

      # Close the connection to the simulation variable server.
      if self.variable_server :
         print(   'Disconnecting from sim on host:port: ' + self.hostname
                + ':' + str( self.port) + '!' )
         self.variable_server.close()
         self.variable_server = None
      
      # Reset the interface state for reconnection.
      self.run_button['state']        = 'disabled'
      self.freeze_button['state']     = 'disabled'
      self.shutdown_button['state']   = 'disabled'
      self.save_button['state']       = 'disabled'
      self.connect_button['state']    = 'active'
      self.disconnect_button['state'] = 'disabled'

      # If I don't do this, sometimes I don't get button state updates.
      root.update_idletasks()

      # Reset the hostname and port.
      self.hostname = ""
      self.port = 0

      # Mark as not connected.
      self.sim_connected = False
      self.prev_sim_mode = -1
   
      return

   #
   def createVariables(self):

      # Connection variables
      self.host_var = StringVar(self)
      self.port_var = StringVar(self)

      # Set the default values for the connection
      if self.hostname:
         self.host_var.set( self.hostname )
      else:
         self.host_var.set( '' )
      self.port_var.set( str(self.port) )

      # HLA Save/Restore variables
      self.save_label_var = StringVar(self)

      # Time variables
      self.set_str_var = StringVar(self)
      self.set_str_var.set( str( self.sim_time ) )
      self.sst_str_var = StringVar(self)
      self.sst_str_var.set( str( self.scenario_time ) )
      self.hlt_str_var = StringVar(self)
      self.hlt_str_var.set( str( self.scenario_time ) )

   #
   def createWidgets(self):

      # Instantiate the content frames.
      self.action_frame  = Frame( self )
      self.time_frame    = Frame( self )
      self.simctrl_frame = Frame( self )

      # Connection widgets
      self.connect_button            = Button( self.action_frame )
      self.connect_button['text']    = 'Connect'
      self.connect_button['command'] = self.connectToSim
      self.connect_button['state']   = 'active'

      self.disconnect_button            = Button( self.action_frame )
      self.disconnect_button['fg']      = 'red'
      self.disconnect_button['text']    = 'Disconnect'
      self.disconnect_button['command'] = self.disconnectFromSim
      self.disconnect_button['state']   = 'disable'

      self.host_label = Label( self.action_frame, text = 'Host:' )
      self.host_entry = Entry( self.action_frame, textvariable = self.host_var )
      self.port_label = Label( self.action_frame, text = 'Port:' )
      self.port_entry = Entry( self.action_frame, textvariable = self.port_var )

      # Save widgets
      self.save_sep = ttk.Separator( self.action_frame, orient='horizontal' )
      self.save_button = Button( self.action_frame )
      self.save_button['text'] = 'Save'
      self.save_button['state'] = 'disabled'
      self.save_button['command'] = self.federationSave
      self.save_label = Label( self.action_frame, text = 'Label:' )
      self.save_entry = Entry( self.action_frame, textvariable = self.save_label_var )

      # Time separator
      self.time_sep = ttk.Separator( self, orient='vertical' )

      # Time fields
      self.time_label = Label( self.time_frame, text = 'Federation Times' )

      self.set_label = Label( self.time_frame, text = 'SET:' )
      self.set_value = Label( self.time_frame, bd=5, width=5 )
      self.set_value['textvariable'] = self.set_str_var

      self.sst_label = Label( self.time_frame, text = 'SST:' )
      self.sst_value = Label( self.time_frame, bd=5, width=5 )
      self.sst_value['textvariable'] = self.sst_str_var

      self.hlt_label = Label( self.time_frame, text = 'HLT:' )
      self.hlt_value = Label( self.time_frame, bd=5, width=5 )
      self.hlt_value['textvariable'] = self.hlt_str_var

      # Button separator
      self.simctrl_sep = ttk.Separator( self, orient='horizontal' )

      # Run button
      self.run_button = Button(self.simctrl_frame)
      self.run_button['text'] = 'Run'
      self.run_button['state'] = 'disabled'
      self.run_button['command'] = self.goToRun

      # Freeze button
      self.freeze_button = Button(self.simctrl_frame)
      self.freeze_button['text'] = 'Freeze'
      self.freeze_button['fg']   = 'blue'
      self.freeze_button['state'] = 'disabled'
      self.freeze_button['command'] = self.goToFreeze

      # Shutdown button
      self.shutdown_button = Button(self.simctrl_frame)
      self.shutdown_button['text'] = 'Shutdown'
      self.shutdown_button['state'] = 'disabled'
      self.shutdown_button['command'] = self.federationShutdown

      # Quit button
      self.quit_button = Button(self.simctrl_frame)
      self.quit_button['text'] = 'QUIT'
      self.quit_button['fg']   = 'red'
      self.quit_button['command'] =  self.applicationExit

   #
   def gridWidgets(self):

      # Setup for resize
      self.grid_columnconfigure( 0, weight=1 )
      self.grid_rowconfigure( 0, weight=1 )
      self.columnconfigure( 0, weight = 1 )
      self.columnconfigure( 1, weight = 3 )
      #self.columnconfigure( 2, weight = 1 )
      self.columnconfigure( 3, weight = 1 )
      self.columnconfigure( 4, weight = 3 )

      # Geometry for Action widgets
      self.action_frame.grid( row = 0, column = 0, sticky = (N,S,E,W)  )
      self.action_frame.columnconfigure( 0, weight = 1 )
      self.action_frame.columnconfigure( 1, weight = 1 )
      self.action_frame.rowconfigure( 0, weight = 1 )
      self.action_frame.rowconfigure( 1, weight = 1 )
      self.action_frame.rowconfigure( 2, weight = 1 )
      self.action_frame.rowconfigure( 3, weight = 1 )
      self.action_frame.rowconfigure( 4, weight = 1 )
      self.action_frame.rowconfigure( 5, weight = 1 )

      action_options = dict( sticky=(N,W) )
      self.connect_button.grid( row = 0, column = 0, **action_options )
      self.disconnect_button.grid( row = 0, column = 1, **action_options )
      self.host_label.grid( row = 1, column = 0, **action_options )
      self.host_entry.grid( row = 1, column = 1, **action_options )
      self.port_label.grid( row = 2, column = 0, **action_options )
      self.port_entry.grid( row = 2, column = 1, **action_options )

      self.save_sep.grid( row = 3, column = 0, columnspan = 3, sticky = (E,W) )

      self.save_button.grid( row = 4, column = 0, **action_options )
      self.save_label.grid( row = 5, column = 0, **action_options )
      self.save_entry.grid( row = 5, column = 1, **action_options )

      # Geometry for Time widgets
      self.time_sep.grid( row = 0, column = 2, rowspan = 3, sticky = (N,S))

      self.time_frame.grid( row = 0, column = 3, sticky = (N,S,E,W)  )
      self.time_frame.columnconfigure( 3, weight = 1 )
      self.time_frame.columnconfigure( 4, weight = 1 )
      self.time_frame.rowconfigure( 0, weight = 1 )
      self.time_frame.rowconfigure( 1, weight = 1 )
      self.time_frame.rowconfigure( 2, weight = 1 )
      self.time_frame.rowconfigure( 3, weight = 1 )
      self.time_frame.rowconfigure( 4, weight = 1 )

      time_options = dict( sticky=(N,W) )
      self.time_label.grid( row = 0, column = 3, columnspan = 4, **time_options )
      self.set_label.grid( row = 1, column = 3, **time_options )
      self.set_value.grid( row = 1, column = 4, **time_options )
      self.sst_label.grid( row = 2, column = 3, **time_options )
      self.sst_value.grid( row = 2, column = 4, **time_options )
      self.hlt_label.grid( row = 3, column = 3, **time_options )
      self.hlt_value.grid( row = 3, column = 4, **time_options )

      # Geometry for simulation control widgets
      self.simctrl_sep.grid( row = 6, column = 0, columnspan = 5, sticky = (N,S,E,W) )

      self.simctrl_frame.grid( row = 7, column = 0, columnspan = 5, sticky = (N,S,E,W) )
      self.simctrl_frame.columnconfigure( 0, weight = 1 )
      self.simctrl_frame.columnconfigure( 1, weight = 1 )
      self.simctrl_frame.columnconfigure( 2, weight = 1 )
      self.simctrl_frame.columnconfigure( 3, weight = 1 )
      self.simctrl_frame.columnconfigure( 4, weight = 1 )

      self.run_button.grid( row = 7, column = 0, sticky = (S,W) )
      self.freeze_button.grid( row = 7, column = 1, sticky = (S,W) )
      self.shutdown_button.grid( row = 7, column = 3, sticky = (S,W) )
      self.quit_button.grid( row = 7, column = 4, sticky = (S,E) )

   #
   def __init__( self, master=None ):
      Frame.__init__(self, master)

      self.prev_sim_mode = self.sim_mode

      # Get the input arguments.
      if ( inputArgs.host ):
         if inputArgs.host == 'localhost' :
            self.hostname = '127.0.0.1'
         else:
            self.hostname = inputArgs.host
      else:
         self.hostname = None

      if ( inputArgs.port ) :
         self.port = int( inputArgs.port )
      else :
         self.port = 0

      self.createVariables()
      self.createWidgets()
      self.gridWidgets()
      self.grid_columnconfigure(0, weight=1)
      self.grid_rowconfigure(0, weight=1)

      # Check for auto connect.
      if inputArgs.auto :
         self.connectToSim()



# Run the Control Panel Application
NoDefaultRoot()
root = Tk()
app = MasterControl( master = root )
app.grid( sticky = (N,S,E,W) )
app.master.title( 'TrickHLA Master Control' )
root.grid_columnconfigure( 0, weight=1 )
root.grid_rowconfigure( 0, weight=1 )
root.resizable( True, True )
app.after( 250, app.updateTimeScales )
app.after( 250, app.updateSimState )
root.mainloop()
app.closeVarServerConnection()
root.destroy()
