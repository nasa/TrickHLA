##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Analytic federate for
#     the HLA time overhead example that uses the Space Reference FOM
#     configured to be the Master, Pacing, and Root Reference Frame Publisher
#     (RRFP) roles.)
#
# REFERENCE:
#    (Trick 19 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.))
#     ((Dan Dexter) (NASA/ER6) (Sept 2026) (--) (SpaceFOM HLA time overhead example.)))
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
from TrickHLA_data.SpaceFOM.SpaceFOMFederateConfig2 import *

# Load the SpaceFOM specific reference frame configuration object.
from TrickHLA_data.SpaceFOM.SpaceFOMRefFrameObject import *


def print_usage_message():

   print( ' ' )
   print( 'TrickHLA Sine Wave SpaceFOM Simulation Command Line Configuration Options:' )
   print( '  -h --help              : Print this help message.' )
   print( '  -f --fed_name [name]   : Name of the Federate, default is Fed1.' )
   print( '  -fe --fex_name [name]  : Name of the Federation Execution, default is SpaceFOM_hla_time_overhead.' )
   print( '  --freeze [on|off]      : on: Start in freeze, off: Run and no sim-control (Default).' )
   print( '  --log-time-stats       : Log TrickHLA time statistics.' )
   print( '  --nostop               : Set no stop time on simulation.' )
   print( '  --realtime [on|off]    : on: Enable realtime, off: disable realtime (Default).' )
   print( '  -r --root_frame [name] : Name of the root reference frame, default is RootFrame.' )
   print( '  -s --stop [time]       : Time to stop simulation, default is 10.0 seconds.' )
   print( '  --verbose              : Show verbose messages.' )
   print( ' ' )

   trick.exec_terminate_with_return( -1,
                                     sys._getframe( 0 ).f_code.co_filename,
                                     sys._getframe( 0 ).f_lineno,
                                     'Print usage message.' )
   return


def parse_command_line():

   global print_usage
   global run_duration
   global verbose
   global federate_name
   global federation_name
   global root_frame_name
   global realtime_enabled
   global freeze_enabled
   global log_time_stats

   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()

   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while ( index < argc ):

      if ( ( str( argv[index] ) == '-h' ) | ( str( argv[index] ) == '--help' ) ):
         print_usage = True

      elif ( ( str( argv[index] ) == '-f' ) | ( str( argv[index] ) == '--fed_name' ) ):
         index = index + 1
         if ( index < argc ):
            federate_name = str( argv[index] )
         else:
            print( 'ERROR: Missing --fed_name [name] argument.' )
            print_usage = True

      elif ( ( str( argv[index] ) == '-fe' ) | ( str( argv[index] ) == '--fex_name' ) ):
         index = index + 1
         if ( index < argc ):
            federation_name = str( argv[index] )
         else:
            print( 'ERROR: Missing --fex_name [name] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--freeze' ):
         index = index + 1
         if ( index < argc ):
            if ( str( argv[index] ) == 'on' ):
               freeze_enabled = True
            elif ( str( argv[index] ) == 'off' ):
               freeze_enabled = False
            else:
               print( 'ERROR: Unknown --freeze argument: ' + str( argv[index] ) )
               print_usage = True
         else:
            print( 'ERROR: Missing --freeze [on|off] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--log-time-stats' ):
         log_time_stats = True

      elif ( ( str( argv[index] ) == '-r' ) | ( str( argv[index] ) == '--root_frame' ) ):
         index = index + 1
         if ( index < argc ):
            root_frame_name = str( argv[index] )
         else:
            print( 'ERROR: Missing --root_frame [name] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--realtime' ):
         index = index + 1
         if ( index < argc ):
            if ( str( argv[index] ) == 'on' ):
               realtime_enabled = True
            elif ( str( argv[index] ) == 'off' ):
               realtime_enabled = False
            else:
               print( 'ERROR: Unknown --realtime argument: ' + str( argv[index] ) )
               print_usage = True
         else:
            print( 'ERROR: Missing --realtime [on|off] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--nostop' ):
         run_duration = None

      elif ( ( str( argv[index] ) == '-s' ) | ( str( argv[index] ) == '--stop' ) ):
         index = index + 1
         if ( index < argc ):
            run_duration = float( str( argv[index] ) )
         else:
            print( 'ERROR: Missing -stop [time] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--verbose' ):
         verbose = True

      elif ( ( str( argv[index] ) == '-d' ) ):
         # Pass this on to Trick.
         break

      else:
         print( 'ERROR: Unknown command line argument ' + str( argv[index] ) )
         print_usage = True

      index = index + 1
   return


# Default: Don't show usage.
print_usage = False

# Set the default run duration.
run_duration = 10000.0

# Set the default to enable realtime.
realtime_enabled = False

# Start in freeze mode and a simulation control panel.
freeze_enabled = False

# Default is to NOT show verbose messages.
verbose = False

# Default is to NOT log TrickHLA time statistics.
log_time_stats = False

# Set the default Federate name.
federate_name = 'Fed1'

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_hla_time_overhead'

# Set the default Root Reference Frame name.
root_frame_name = 'RootFrame'

parse_command_line()

if ( print_usage == True ):
   print_usage_message()

#---------------------------------------------
# Set up the core simulation parameters.
#---------------------------------------------
core_frame_time = 1.000


#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
# instruments.echo_jobs.echo_jobs_on()
# trick.checkpoint_pre_init( 1 )
# trick.checkpoint_post_init( 1 )
# trick.add_read( 0.0 , '''trick.checkpoint('chkpnt_point')''' )
# trick.checkpoint_end( 1 )

# Import and configure the TrickHLA base Simulation Configuration class.
# Setup for Trick real time execution. This is the "Pacing" function.
from TrickHLA_data.TrickHLA.TrickHLASimConfig import *
sim_config = TrickHLASimConfig( 'HLA-time-overhead' )
sim_config.realtime( software_frame_time = core_frame_time )
sim_config.sim_control_panel()
if ( freeze_enabled ):
   sim_config.start_in_freeze()
else:
	sim_config.start_in_freeze( False )

if ( realtime_enabled ):
   trick.real_time_enable()
else:
   trick.real_time_disable()


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig2(
   thla_federate        = THLA.federate,
   thla_control         = THLA.execution_control,
   thla_config          = THLA.ExCO,
   thla_federation_name = federation_name,
   thla_federate_name   = federate_name,
   thla_enabled         = True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
# federate.set_config_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if ( verbose == True ):
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
else:
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( True )  # This is the Master federate.
federate.set_pacing_role( True )  # This is the Pacing federate.
federate.set_RRFP_role( True )    # This is the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_federate( True, str( federate.federate.name ) )
federate.add_known_federate( True, 'Fed2' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost:8989'

#--------------------------------------------------------------------------
# Set up federate time related parameters.
#--------------------------------------------------------------------------
# Set the simulation timeline to be used for time computations.
THLA.execution_control.sim_timeline = THLA_INIT.sim_timeline

# Set the scenario timeline to be used for configuring federation freeze times.
THLA.execution_control.scenario_timeline = THLA_INIT.scenario_timeline

# Specify the HLA base time unit (default: trick.HLA_BASE_TIME_MICROSECONDS)
# and scale the Trick time tics value.
federate.set_HLA_base_time_unit_and_scale_trick_tics( trick.HLA_BASE_TIME_MICROSECONDS )

# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( core_frame_time )

# Must specify the Least Common Time Step for all federates in the
# federation execution.
federate.set_least_common_time_step( core_frame_time )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )


#---------------------------------------------
# Set up data to record.
#---------------------------------------------
# Enable the collection of TrickHLA time statistics.
THLA.federate.enable_time_statistics( True )

if log_time_stats:
   # Import the TrickHLA Time Statistics Data Recording Group class.
   from TrickHLA_data.TrickHLA.TrickHLATimeStatsDRG import TrickDataRecordingGroup, TrickHLATimeStatsDRG

   # Create the TrickHLA Time Statistics Data Recording Group.
   thla_time_drg = TrickHLATimeStatsDRG( core_frame_time )

   # Initialize all the Data Recording Groups.
   TrickDataRecordingGroup.initialize_groups()


#---------------------------------------------------------------------------
# Set up for Root Reference Frame data.
#---------------------------------------------------------------------------
ref_frame_tree.root_frame_data.name = root_frame_name
ref_frame_tree.root_frame_data.parent_name = ''

ref_frame_tree.root_frame_data.state.pos[0] = 0.0
ref_frame_tree.root_frame_data.state.pos[1] = 0.0
ref_frame_tree.root_frame_data.state.pos[2] = 0.0
ref_frame_tree.root_frame_data.state.vel[0] = 0.0
ref_frame_tree.root_frame_data.state.vel[1] = 0.0
ref_frame_tree.root_frame_data.state.vel[2] = 0.0
ref_frame_tree.root_frame_data.state.att.scalar = 1.0
ref_frame_tree.root_frame_data.state.att.vector[0] = 0.0
ref_frame_tree.root_frame_data.state.att.vector[1] = 0.0
ref_frame_tree.root_frame_data.state.att.vector[2] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[0] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[1] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[2] = 0.0
ref_frame_tree.root_frame_data.state.time = 0.0

ref_frame_tree.leaf_frame_data.name = 'FrameA'
ref_frame_tree.leaf_frame_data.parent_name = root_frame_name

ref_frame_tree.leaf_frame_data.state.pos[0] = 10.0
ref_frame_tree.leaf_frame_data.state.pos[1] = 10.0
ref_frame_tree.leaf_frame_data.state.pos[2] = 10.0
ref_frame_tree.leaf_frame_data.state.vel[0] = 0.0
ref_frame_tree.leaf_frame_data.state.vel[1] = 0.1
ref_frame_tree.leaf_frame_data.state.vel[2] = 0.0
ref_frame_tree.leaf_frame_data.state.att.scalar = 1.0
ref_frame_tree.leaf_frame_data.state.att.vector[0] = 0.0
ref_frame_tree.leaf_frame_data.state.att.vector[1] = 0.0
ref_frame_tree.leaf_frame_data.state.att.vector[2] = 0.0
ref_frame_tree.leaf_frame_data.state.ang_vel[0] = 0.0
ref_frame_tree.leaf_frame_data.state.ang_vel[1] = 0.1
ref_frame_tree.leaf_frame_data.state.ang_vel[2] = 0.0
ref_frame_tree.leaf_frame_data.state.time = 0.0

#---------------------------------------------------------------------------
# Set up the Root Reference Frame object for discovery.
# If it is the RRFP, it will publish the frame.
# If it is NOT the RRFP, it will subscribe to the frame.
#---------------------------------------------------------------------------
root_frame = SpaceFOMRefFrameObject( 
   create_frame_object          = federate.is_RRFP,
   frame_instance_name          = root_frame_name,
   frame_S_define_instance      = root_ref_frame.frame_packing,
   frame_S_define_instance_name = 'root_ref_frame.frame_packing',
   frame_conditional            = root_ref_frame.conditional )

# Set the debug flag for the root reference frame.
root_ref_frame.frame_packing.debug = verbose

# Set the root frame for the federate.
federate.set_root_frame( root_frame )

# Set the lag compensation parameters.
# NOTE: The ROOT REFERENCE FRAME never needs to be compensated!

#---------------------------------------------------------------------------
# Set up an alternate vehicle reference frame object for discovery.
#---------------------------------------------------------------------------
frame_A = SpaceFOMRefFrameObject( 
   create_frame_object          = True,
   frame_instance_name          = 'FrameA',
   frame_S_define_instance      = leaf_ref_frame.frame_packing,
   frame_S_define_instance_name = 'leaf_ref_frame.frame_packing',
   parent_S_define_instance     = root_ref_frame.frame_packing,
   parent_name                  = root_frame_name,
   frame_conditional            = leaf_ref_frame.conditional,
   frame_lag_comp               = leaf_ref_frame.lag_compensation,
   frame_ownership              = leaf_ref_frame.ownership_handler,
   frame_deleted                = leaf_ref_frame.deleted_callback )

# Set the debug flag for the root reference frame.
leaf_ref_frame.frame_packing.debug = verbose

# Add this reference frame to the list of managed object.
federate.add_fed_object( frame_A )

# Set the lag compensation parameters.
# The reality is that the ROOT REFERENCE FRAME never needs to be compensated!
leaf_ref_frame.lag_compensation.debug = False
leaf_ref_frame.lag_compensation.set_integ_tolerance( 1.0e-6 )
leaf_ref_frame.lag_compensation.set_integ_dt( 0.025 )

# frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_NONE )
frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
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
if run_duration:
   trick.sim_services.exec_set_terminate_time( run_duration )
