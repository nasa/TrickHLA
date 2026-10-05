##############################################################################
#
# @file TrickHLATimeStatsDRG.py
# @ingroup TrickHLA
# @brief Definition of the TrickHLA time statistics Data Recording Group.
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
# 2101 NASA Parkway, Houston, TX  77058
# 
# @python_module{Trick}
#
# @revs_title
# @revs_begin
# @rev_entry{ Dan Dexter, NASA JSC, TrickHLA, September 2026, --, Initial version. }
# @revs_end
#
##############################################################################

# Import the TrickHLA base Trick Data Recording Classes.
from TrickHLA_data.Trick.TrickDataRecordingGroup import *

class TrickHLATimeStatsDRG( TrickDataRecordingGroup ):

   # Class constructor.
   def __init__( self,
                 rate,
                 log_min_max = False,
                 base_name   = 'THLA',
                 group_name  = 'THLATimeStats',
                 format      = TrickDRFormat.BINARY,
                 when        = TrickDRWhen.ALWAYS,
                 change_var  = None,
                 buffer      = TrickDRBuffer.BUFFER,
                 enabled     = True ):

      # Set the base name for looking up the state variables.
      self.base_name = base_name

      # Call the TrickDataRecordingGroup constructor.
      TrickDataRecordingGroup.__init__( self,
                                        group_name,
                                        rate,
                                        format,
                                        when,
                                        change_var,
                                        buffer,
                                        enabled )

      # Add the TAG wait time statistics recording variables.
      self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.elapsed_time' )
      self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.elapsed_time_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.elapsed_time_min' )
         self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.elapsed_time_max' )
      self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.jitter' )
      self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.jitter_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.jitter_min' )
         self.drg.add_variable( self.base_name + '.federate.tag_wait_stats.jitter_max' )

      # Add the TAR to TAG elapsed time statistics recording variables.
      self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.elapsed_time' )
      self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.elapsed_time_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.elapsed_time_min' )
         self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.elapsed_time_max' )
      self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.jitter' )
      self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.jitter_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.jitter_min' )
         self.drg.add_variable( self.base_name + '.federate.tar_tag_stats.jitter_max' )

      # Add the TARA to TAG elapsed time statistics recording variables.
      self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.elapsed_time' )
      self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.elapsed_time_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.elapsed_time_min' )
         self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.elapsed_time_max' )
      self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.jitter' )
      self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.jitter_avg' )
      if log_min_max:
         self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.jitter_min' )
         self.drg.add_variable( self.base_name + '.federate.tara_tag_stats.jitter_max' )

      return

