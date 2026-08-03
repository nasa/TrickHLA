##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Space Reference FOM
#     example Root Reference Frame Publisher (RRFP) federate run.)
#
# REFERENCE:
#    (Trick 19 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Feb 2025) (--) (SpaceFOM frames testing.)))
##############################################################################
import os
import sys

# Find the TrickHLA home location and append the path.
trickhla_home = os.environ.get( "TRICKHLA_HOME" )
if trickhla_home is None:
   sys.exit( '\033[91m'+'Environment variable TRICKHLA_HOME is not defined!'+'\033[0m\n' )
else:
   if os.path.isdir( trickhla_home ) is False:
      sys.exit( '\033[91m'+'TRICKHLA_HOME not found: '+trickhla_home+'\033[0m\n' )

# Append the path to the top level of the top level TrickHLA directory.
# We need this to locate the TrickHLA_data Python data directory.
if trickhla_home not in sys.path :
   sys.path.append( trickhla_home )

# Load the SpaceFOM specific federate configuration object.
from TrickHLA_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from TrickHLA_data.SpaceFOM.SpaceFOMRefFrameObject import *


def print_usage_message():

   print( ' ' )
   print( 'SpaceFOM Reference Frame Simulation Command Line Configuration Options:' )
   print( '  -d                    : Pass the input file debug option on to Trick.' )
   print( '  --default_data_config : Use default_data to configure the objects.' )
   print( '  -h --help             : Print this help message.' )
   print( '  --nostop              : Set no stop time on simulation.' )
   print( '  --stop [time]         : Time to stop simulation, default is 0.5 seconds.' )
   print( '  -t --tree             : Print the frame tree data.' )
   print( '  --verbose             : Show verbose messages.' )
   print( ' ' )

   trick.exec_terminate_with_return( -1,
                                     sys._getframe( 0 ).f_code.co_filename,
                                     sys._getframe( 0 ).f_lineno,
                                     'Print usage message.' )
   return


def parse_command_line():

   # Execution control parameters
   global print_usage
   global run_duration
   global verbose
   global trick_debug

   # Frame definition parameters
   global print_tree
   global frames_list

   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()

   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while ( index < argc ):

      if ( str( argv[index] ) == '-d' ):
         trick_debug = True
         # Catch the Trick debug command line option an do NOT terminate.
         print( 'DEBUG: Specified input file debug option to Trick.' )

      elif ( str( argv[index] ) == '--default_data_config' ):
         # Catch the default_data configure command line option an do NOT terminate.
         print( 'NOTE: Configuring with default_data option.' )

      elif ( str( argv[index] ) == '--stop' ):
         index = index + 1
         if ( index < argc ):
            run_duration = float( str( argv[index] ) )
         else:
            print( 'input.py ERROR: Missing --stop [time] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--nostop' ):
         run_duration = None

      elif ( ( str( argv[index] ) == '-h' ) | ( str( argv[index] ) == '--help' ) ):
         print_usage = True

      elif ( ( str( argv[index] ) == '-t' ) | ( str( argv[index] ) == '--tree' ) ):
         print_tree = True

      elif ( str( argv[index] ) == '--verbose' ):
         verbose = True

      else:
         print( 'input.py ERROR: Unknown command line argument ' + str( argv[index] ) )
         print_usage = True

      index = index + 1
   return


# Default: Don't show usage.
print_usage = False

# Default: Not in debug mode.
trick_debug = False

# Set the default run duration.
run_duration = 10.0

# Default is to NOT show verbose messages.
verbose = False

# Default Trick run configuration directory.
config_dir = 'Modified_data'

# Default is not to print the frames tree.
print_tree = False

# Parse command line arguments to override defaults.
parse_command_line()

# Check for print usage flag.  Terminates in call.
if ( print_usage == True ):
   print_usage_message()

#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
# instruments.echo_jobs.echo_jobs_on()
# trick.checkpoint_pre_init(1)
# trick.checkpoint_post_init(1)
# trick.add_read(0.0 , '''trick.checkpoint('chkpnt_point')''')
# trick.checkpoint_end(1)

# Import and configure the TrickHLA base Simulation Configuration class.
from TrickHLA_data.TrickHLA.TrickHLASimConfig import *
frames_sim_config = TrickHLASimConfig( 'TestTree' )
frames_sim_config.set_software_and_freeze_frame_time( software_frame_time = 0.250 )
frames_sim_config.start_in_freeze( False )


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig(
   thla_federate        = THLA.federate,
   thla_control         = THLA.execution_control,
   thla_config          = THLA.ExCO,
   thla_federation_name = 'SpaceFOM_Roles_Test',
   thla_federate_name   = 'Simple_Tree',
   thla_enabled         = True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
# federate.set_config_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if ( verbose == True ):
   # federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_6_TRACE )
   federate.set_debug_source( trick.TrickHLA.DEBUG_SRC_ALL_MODULES )
   # federate.set_debug_source( trick.TrickHLA.DEBUG_SRC_OBJECT + trick.TrickHLA.DEBUG_SRC_ATTRIBUTE )
else:
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
   # federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_2_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False )  # This is NOT the Master federate.
federate.set_pacing_role( False )  # This is NOT the Pacing federate.
federate.set_RRFP_role( False )    # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
# This is the RRFP federate.
# It doesn't really need to know about any other federates.
federate.add_known_federate( True, str( federate.federate.name ) )
# federate.add_known_federate( True, 'Master' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost:8989'

#--------------------------------------------------------------------------
# Set up federate related time related parameters.
#--------------------------------------------------------------------------
# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( 0.250 )

# Must specify the Least Common Time Step for all federates in the
# federation execution.
federate.set_least_common_time_step( 0.250 )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )

#---------------------------------------------------------------------------
# Set up CTE time line.
#---------------------------------------------------------------------------
# By setting this we are specifying the use of Common Timing Equipment (CTE)
# for controlling the Mode Transitions for all federates using CTE.
# Don't really need CTE for RRFP.
# THLA.execution_control.cte_timeline = trick.sim_services.alloc_type( 1, 'TrickHLA::TimeOfDayCTETimeline' )

#---------------------------------------------------------------------------
# Setup the frame integrators
#---------------------------------------------------------------------------
leaf_loop.getIntegrator( trick.Runge_Kutta_4, 13 )

#---------------------------------------------------------------------------
# Set up the Reference Frame objects.
#---------------------------------------------------------------------------

if not trick_debug:
   # Check for object allocation.
   # Probably did not trigger the default_data allocation.
   if THLA.federate.get_object_service().obj_count <= 0:
      trick.exec_terminate_with_return( -1,
                                        sys._getframe( 0 ).f_code.co_filename,
                                        sys._getframe( 0 ).f_lineno,
                                        'input.py ERROR: TrickHLA objects have not been allocated! Try running with --default_data_config command line argument.' )
   
   # Set the debug flag for the reference frames.
   root_ref_frame.frame_packing.debug = verbose
   leaf_ref_frame.frame_packing.debug = verbose

   # Mark the frames as subscribed.
   root_ref_frame.frame_packing.subscribe()
   leaf_ref_frame.frame_packing.subscribe()

#---------------------------------------------------------------------------
# Set up the Reference Frame Tree
#---------------------------------------------------------------------------
ref_frame_tree.frame_tree.debug = True
trick.exec_set_job_onoff( "ref_frame_tree.frame_tree.print_tree", 1, print_tree )

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
federate.add_sim_object( ref_frame_tree )
federate.add_sim_object( root_ref_frame )
federate.add_sim_object( leaf_ref_frame )

#---------------------------------------------------------------------------
# Make sure that the Python federate configuration object is initialized.
#---------------------------------------------------------------------------
# federate.disable()
federate.initialize()

#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------

if run_duration != None:
   if run_duration == 0.0:
      trick.stop( 0.0 )
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )
