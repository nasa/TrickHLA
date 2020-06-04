##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Space Reference FOM
#     example Pacing federate run.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Does not discover a SpaceFOM Reference Frame root frame.)
#     (Instantiated the default TrickHLATimeOfDayTimeline based CTE time
#      line for testing.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
import sys
sys.path.append('../../../')
# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *
# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

def print_usage_message( ):

   print ' '
   print 'TrickHLA SpaceFOM Master Simulation Command Line Configuration Options:'
   print '   -h          : Print this help message.'
   print '   -help       : Print this help message.'
   print '   -stop [time]: Time to stop simulation.'
   print '   -nostop     : Set no stop time on simulation.'
   print ' '

   return


def process_command_line_args():
   # Iterate through to test the command line options.
   for arg in argv :
      
      if str(arg) == '-stop' :
         run_duration = float(argv.next())

      if str(arg) == '-nostop' :
         run_duration = None

      if (str(arg) == '-h') | (str(arg) == '-help'):
         print_usage = True
         print_usage_message()
         trick.exec_terminate_with_return( -1,
                                           sys._getframe(0).f_code.co_filename,
                                           sys._getframe(0).f_lineno,
                                           'Print usage message.')

   return


# Get the Trick command line arguments.
argc = trick.command_line_args_get_argc()
argv = trick.command_line_args_get_argv()

print_usage = False

# Set the default run duration.
run_duration = 10.0

# Process the command line options.
process_command_line_args()

#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe(True)
#trick.checkpoint_pre_init(1)
trick.checkpoint_post_init(1)
#trick.add_read(0.0 , '''trick.checkpoint('chkpnt_point')''')

# Setup for Trick real time execution.  This is the "Pacing" function.
execfile( "Modified_data/trick/realtime.py" )

trick.exec_set_enable_freeze(False)
trick.exec_set_freeze_command(False)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(True)


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.ExCO,
                                   'SpaceFOM_Roles_Test',
                                   'Pacing',
                                   True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO.  This is only needed for input file configuration.
#federate.set_ExCO_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
#federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_8_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False ) # This is NOT the Master federate.
federate.set_pacing_role( True )  # This is the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in know required federates.
#--------------------------------------------------------------------------
# This is the Pacing federate.
# It doesn't really need to know about any other federates.
#federate.add_known_fededrate( True, str(federate.federate.name) )
#federate.add_known_fededrate( True, 'Master' )
#federate.add_known_fededrate( True, 'RRFP' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
#THLA.federate.local_settings = 'crcHost = 10.8.0.161\n crcPort = 8989'
# Mak specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'

#--------------------------------------------------------------------------
# Set up federate related time related parameters.
#--------------------------------------------------------------------------
# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( 0.250 )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )

#--------------------------------------------------------------------------
# Set up CTE time line.
#--------------------------------------------------------------------------
# By setting this we are specifying the use of Common Timing Equipment (CTE)
# for controlling the Mode Transitions for all federates using CTE.
# Don't really need CTE for RRFP.
THLA.execution_control.cte_timeline = trick.sim_services.alloc_type( 1, 'TrickHLA::CTETimelineBase' )


#------------------------------------------------------------------------------
# Disable all reference frame related SimObjects.
#------------------------------------------------------------------------------
# Turn off all the vehicle and vehicle systems simulation objects.
trick.exec_set_sim_object_onoff( 'ref_frame_tree' , False )
trick.exec_set_sim_object_onoff( 'root_ref_frame' , False )
trick.exec_set_sim_object_onoff( 'ref_frame_A' , False )

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )


#---------------------------------------------------------------------------
# Make sure that the Python federate configuration object is initialized.
#---------------------------------------------------------------------------
#federate.disable()
federate.initialize()


#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
if run_duration:
   trick.sim_services.exec_set_terminate_time( run_duration )
