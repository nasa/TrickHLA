##############################################################################
#
# @file Trick/DataRecordingGroup.py
# @ingroup Trick
# @brief Definition of the Trick Data Recording Group Python abstract base
# class.
# 
# This is the base implementation for the Trick Data Recording Group.  This
# Python class is used to define a Trick simulation data recording group
# instance in the Trick input file.
# 
# @copyright Copyright 2026 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Other Rights Reserved.
# 
# \par<b>Responsible Organization</b>
# Simulation and Graphics Branch, Mail Code ER7\n
# Software, Robotics & Simulation Division\n
# NASA, Johnson Space Center\n
# # 2101 NASA Parkway, Houston, TX  77058
# 
# @python_module{Trick}
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA JSC, TrickHLA, January 2026, --, Initial version. }
# @revs_end
#
##############################################################################

import os
import sys
import trick
from enum import IntEnum
from abc import ABC, abstractmethod

# Find the TrickHLA home location and append the path.
trickhla_home = os.environ.get( "TRICKHLA_HOME" )
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
   
class TrickDRFormat( IntEnum ):
   ASCII  = 0
   BINARY = 1
   HDF5   = 2
   
class TrickDRWhen( IntEnum ):
   ALWAYS       = trick.DR_Always
   CHANGES      = trick.DR_Changes
   CHANGES_STEP = trick.DR_Changes_Step
   
class TrickDRBuffer( IntEnum ):
   BUFFER      = trick.DR_Buffer
   NO_BUFFER   = trick.DR_No_Buffer
   RING_BUFFER = trick.DR_Ring_Buffer

class TrickDataRecordingGroup( ABC ):

   # A dictionary of all instantiated data recording groups.
   groups = {}

   @abstractmethod
   # Class constructor.
   def __init__( self,
                 name,
                 rate,
                 format     = TrickDRFormat.ASCII,
                 when       = TrickDRWhen.ALWAYS,
                 change_var = None,
                 buffer     = TrickDRBuffer.BUFFER,
                 enabled    = True ):

      # Set the class variables.
      self.name       = name
      self.rate       = rate
      self.format     = format
      self.when       = when
      self.change_var = change_var
      self.buffer     = buffer
      self.enabled    = enabled
      
      # Check to make sure this group name isn't already taken.
      if name in TrickDataRecordingGroup.groups:
         raise ValueError( 'TrickDataRecordingGroup:__init__: \'' + name + '\' already exists!' )
         return
      
      # Check for change variable if not always.
      if (self.when != TrickDRWhen.ALWAYS) & (self.change_var == None ):
         raise ValueError( 'TrickDataRecordingGroup:__init__: Selected on change with no change variable!' )
         return

      # Instantiate the formatted Data Recording Group
      match self.format:
         case TrickDRFormat.ASCII:
            self.drg = trick.DRAscii( self.name )
         case TrickDRFormat.BINARY:
            self.drg = trick.DRBinary( self.name )
         case TrickDRFormat.HDF5:
            self.drg = trick.HDF5( self.name )
         case _:
            raise ValueError( 'TrickDataRecordingGroup:__init__: Unknown Trick Data Recording Format!' )
            return
         
      # Set the rate for the Data Recording Group.
      self.drg.set_cycle( self.rate )
      
      # Set when this Data Recording Group records data (frequency).
      self.drg.set_freq( self.when )
      
      # Set change variable if appropriate.
      if self.change_var != None :
         self.drg.add_change_variable( self.change_var )
         
      # Set the selected buffer type.
      self.drg.set_buffer_type( self.buffer )
      
      # Set the Data Recording Group enabled state.
      if self.enabled:
         self.drg.enable()
      else:
         self.drg.disable()
         
      # Add the newly created Data Recording Group to the collection.
      TrickDataRecordingGroup.groups.update( { self.name : self } )
            
      return
   
   
   # A static method to initialize the Data Recording Group collection.
   # This method should be called after all Data Recording Groups have been
   # created and configured.
   @staticmethod
   def initialize_groups():
      
      # Iterate through the Data Recording Group Collection
      for name, dr_group in TrickDataRecordingGroup.groups.items():
         trick.add_data_record_group( dr_group.drg, dr_group.buffer )
      
      return
   
   
   # A static method to retrieve a named Data Recording Group from the collection.
   @staticmethod
   def find_drg( group_name ):
      
      # Check to see if the Trick Data Recording Group is in the collection.
      if group_name not in TrickDataRecordingGroup.groups:
         raise ValueError( 'TrickDataRecordingGroup:find_trick_drg: \'' + name + '\' NOT found!' )
         return None
      
      # Return the Trick Data Recording Group in the collection.
      return TrickDataRecordingGroup.groups.get( group_name )
   
   
   # A static method to retrieve a named Trick Data Recording Group from the collection.
   @staticmethod
   def find_trick_drg( group_name ):
      
      # Find the Data Recording Group by name.
      dr_group = find_drg( group_name )
      
      # Get the associated Trick Data Recording Group.
      if dr_group != None:
         return dr_group.drg
      
      # Return the Trick Data Recording Group in the collection.
      return None
   
   
   # A class method to return the raw Trick Data Recording Group.
   def get_trick_drg( self ):
      return self.drg
   
   
   # A class method to enable or disable this Data Recording Group.
   def enable( self, state ):
      # Check to make sure that this.
      if not isinstance( state, bool ):
         raise TypeError( 'TrickDataRecordingGroup:enable: argument is not a boolean!' )
         return
      
      # Set the enable state.
      if state:
         drg.enable()
      else:
         drg.disable()
      
      return
   
   
   # A class variable to add a recording variable to this Data Recording Group.
   def add_variable( self, trick_variable ):
      
      # Check to make sure that the Trick variable is a string.
      if not isinstance( trick_variable, str ):
         raise TypeError( 'TrickDataRecordingGroup:add_variable: argument is not a string!' )
         return
   
      # Add the variable to the Trick Data Recording Group.
      self.drg.add_variable( trick_variable )
      
      return
   
   