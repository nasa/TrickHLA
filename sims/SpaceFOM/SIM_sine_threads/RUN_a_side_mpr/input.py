##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Analytic federate for
#     the sine wave example that uses Trick child threads for a Space
#     Reference FOM configured to be the Master, Pacing, and Root Reference
#     Frame Publisher (RRFP) roles.)
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
#     ((Dan Dexter) (NASA/ER6) (Mar 2024) (--) (SpaceFOM sine example.)))
##############################################################################
import sys
sys.path.append('../../../')

# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

# Load the sine specific Sine object.
from Modified_data.sine.SineObject import *


def print_usage_message( ):

   print(' ')
   print('TrickHLA Sine Wave SpaceFOM Simulation Command Line Configuration Options:')
   print('  -h --help              : Print this help message.')
   print('  -f --fed_name [name]   : Name of the Federate, default is A-side-Federate.')
   print('  -fe --fex_name [name]  : Name of the Federation Execution, default is SpaceFOM_sine.')
   print('  --nostop               : Set no stop time on simulation.')
   print('  -r --root_frame [name] : Name of the root reference frame, default is RootFrame.')
   print('  -s --stop [time]       : Time to stop simulation, default is 10.0 seconds.')
   print('  --verbose [on|off]     : on: Show verbose messages (Default), off: disable messages.')
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.' )
   return


def parse_command_line( ):
   
   global print_usage
   global run_duration
   global verbose
   global federate_name
   global federation_name
   global root_frame_name
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc):
      
      if ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')):
         print_usage = True
      
      elif ((str(argv[index]) == '-f') | (str(argv[index]) == '--fed_name')):
         index = index + 1
         if (index < argc):
            federate_name = str(argv[index])
         else:
            print('ERROR: Missing --fed_name [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-fe') | (str(argv[index]) == '--fex_name')):
         index = index + 1
         if (index < argc):
            federation_name = str(argv[index])
         else:
            print('ERROR: Missing --fex_name [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-r') | (str(argv[index]) == '--root_frame')):
         index = index + 1
         if (index < argc):
            root_frame_name = str(argv[index])
         else:
            print('ERROR: Missing --root_frame [name] argument.')
            print_usage = True
            
      elif (str(argv[index]) == '--nostop'):
         run_duration = None
      
      elif ((str(argv[index]) == '-s') | (str(argv[index]) == '--stop')):
         index = index + 1
         if (index < argc):
            run_duration = float(str(argv[index]))
         else:
            print('ERROR: Missing -stop [time] argument.')
            print_usage = True
      
      elif (str(argv[index]) == '--verbose'):
         index = index + 1
         if (index < argc):
            if (str(argv[index]) == 'on'):
               verbose = True
            elif (str(argv[index]) == 'off'):
               verbose = False
            else:
               print('ERROR: Unknown --verbose argument: ' + str(argv[index]))
               print_usage = True
         else:
            print('ERROR: Missing --verbose [on|off] argument.')
            print_usage = True
         
      elif ((str(argv[index]) == '-d')):
         # Pass this on to Trick.
         break
            
      else:
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
         
      index = index + 1
   return

# Default: Don't show usage.
print_usage = False

# Set the default run duration.
run_duration = 10.0

# Default is to NOT show verbose messages.
verbose = False

# Set the default Federate name.
federate_name = 'A-side-Federate'

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_sine_threads'

# Set the default Root Reference Frame name.
root_frame_name = 'RootFrame'


parse_command_line()

if ( print_usage == True ):
   print_usage_message()

#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe( True )
#trick.checkpoint_pre_init( 1 )
#trick.checkpoint_post_init( 1 )
#trick.add_read( 0.0 , '''trick.checkpoint('chkpnt_point')''' )
#trick.checkpoint_end( 1 )

# Setup for Trick real time execution. This is the "Pacing" function.
exec( open( "Modified_data/trick/realtime.py" ).read() )

trick.exec_set_enable_freeze( True )
trick.exec_set_freeze_command( True )
trick.exec_set_stack_trace( False )

trick.var_server_set_port( 7000 )
#trick.var_server_set_source_address( "127.0.0.1" )
trick.sim_control_panel_set_enabled( True )

#simControlPanel = trick.SimControlPanel()
#simControlPanel.set_host( "localhost" )
#trick.add_external_application( simControlPanel )

#---------------------------------------------
# Set up data to record.
#---------------------------------------------
exec(open( "Log_data/log_sine_states.py" ).read())
log_sine_states( 'A', 0.250 )
log_sine_states( 'P', 0.250 )


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( thla_federate        = THLA.federate,
                                   thla_manager         = THLA.manager,
                                   thla_control         = THLA.execution_control,
                                   thla_config          = THLA.ExCO,
                                   thla_federation_name = federation_name,
                                   thla_federate_name   = federate_name,
                                   thla_enabled         = True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
#federate.set_config_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if ( verbose == True ): 
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
else:
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( True ) # This is the Master federate.
federate.set_pacing_role( True ) # This is the Pacing federate.
federate.set_RRFP_role( True )   # This is the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_federate( True, str(federate.federate.name) )
federate.add_known_federate( True, 'P-side-Federate' )
federate.add_known_federate( False, 'Other' )

#--------------------------------------------------------------------------
# Configure the FOM modules.
#--------------------------------------------------------------------------
federate.add_FOM_module( 'FOMs/sine/Sine_FOM.xml' )

#--------------------------------------------------------------------------
# Configure the multiphase initialization sync-points.
#--------------------------------------------------------------------------
federate.add_multiphase_init_sync_point( 'Analytic_init_phase' )
federate.add_multiphase_init_sync_point( 'Propagated_init_phase' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'

#--------------------------------------------------------------------------
# Set up federate time related parameters.
#--------------------------------------------------------------------------
# Set the simulation timeline to be used for time computations.
THLA.execution_control.sim_timeline = THLA_INIT.sim_timeline

# Set the scenario timeline to be used for configuring federation freeze times.
THLA.execution_control.scenario_timeline = THLA_INIT.scenario_timeline

# Specify the HLA base time units (default: trick.HLA_BASE_TIME_MICROSECONDS).
federate.set_HLA_base_time_units( trick.HLA_BASE_TIME_MICROSECONDS )

# Scale the Trick Time Tic value based on the HLA base time units.
federate.scale_trick_tics_to_base_time_units()

# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( 0.250 )

# Must specify the Least Common Time Step for all federates in the
# federation execution.
federate.set_least_common_time_step( 0.250 )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )


#---------------------------------------------
# Set up the initial Sine states
#---------------------------------------------
exec(open( "Modified_data/sine_init.py" ).read())

# Example of a 1-dimensional dynamic array.
A.packing.buff_size = 10
A.packing.buff = trick.sim_services.alloc_type( A.packing.buff_size, 'unsigned char' )
P.packing.buff_size = 10
P.packing.buff = trick.sim_services.alloc_type( P.packing.buff_size, 'unsigned char' )

# We are taking advantage of the input file to specify a unique name for the
# sim-data name field for the A-side federate.
A.sim_data.name = 'A.sim_data.name.A-side'
P.sim_data.name = 'P.sim_data.name.A-side'

# We are taking advantage of the input file to specify a unique name and
# message for the A-side federate interaction handler.
A.interaction_handler.name = 'A-side: A.interaction_handler.name'
P.interaction_handler.name = 'A-side: P.interaction_handler.name'

A.interaction_handler.message = 'A-side: A.interaction_handler.message'
P.interaction_handler.message = 'A-side: P.interaction_handler.message'


#---------------------------------------------------------------------------
# Set up for Sine data.
#---------------------------------------------------------------------------

sine_A = SineObject( sine_create_object      = True,
                     sine_obj_instance_name  = 'A-side-Federate.Sine',
                     sine_trick_sim_obj_name = 'A',
                     sine_packing            = A.packing,
                     sine_conditional        = A.conditional,
                     sine_lag_comp           = A.lag_compensation,
                     sine_lag_comp_type      = trick.TrickHLA.LAG_COMPENSATION_NONE,
                     sine_ownership          = A.ownership_handler,
                     sine_deleted            = A.obj_deleted,
                     sine_thread_IDs         = '1' ) # Trick child thread 1.

trick.exec_set_thread_process_type( 1 , trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 1 , 0.250 )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_A )

sine_P = SineObject( sine_create_object      = False,
                     sine_obj_instance_name  = 'P-side-Federate.Sine',
                     sine_trick_sim_obj_name = 'P',
                     sine_packing            = P.packing,
                     sine_conditional        = P.conditional,
                     sine_lag_comp           = P.lag_compensation,
                     sine_lag_comp_type      = trick.TrickHLA.LAG_COMPENSATION_NONE,
                     sine_deleted            = P.obj_deleted,
                     sine_thread_IDs         = '2' ) # Trick child thread 2.

trick.exec_set_thread_process_type( 2 , trick.PROCESS_TYPE_AMF_CHILD )
trick.exec_set_thread_amf_cycle_time( 2 , 0.250 )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_P )


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
ref_frame_tree.root_frame_data.state.att.scalar  = 1.0
ref_frame_tree.root_frame_data.state.att.vector[0] = 0.0
ref_frame_tree.root_frame_data.state.att.vector[1] = 0.0
ref_frame_tree.root_frame_data.state.att.vector[2] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[0] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[1] = 0.0
ref_frame_tree.root_frame_data.state.ang_vel[2] = 0.0
ref_frame_tree.root_frame_data.state.time = 0.0


ref_frame_tree.vehicle_frame_data.name = 'FrameA'
ref_frame_tree.vehicle_frame_data.parent_name = root_frame_name
                                        
ref_frame_tree.vehicle_frame_data.state.pos[0] = 10.0
ref_frame_tree.vehicle_frame_data.state.pos[1] = 10.0
ref_frame_tree.vehicle_frame_data.state.pos[2] = 10.0
ref_frame_tree.vehicle_frame_data.state.vel[0] = 0.0
ref_frame_tree.vehicle_frame_data.state.vel[1] = 0.1
ref_frame_tree.vehicle_frame_data.state.vel[2] = 0.0
ref_frame_tree.vehicle_frame_data.state.att.scalar  = 1.0
ref_frame_tree.vehicle_frame_data.state.att.vector[0] = 0.0
ref_frame_tree.vehicle_frame_data.state.att.vector[1] = 0.0
ref_frame_tree.vehicle_frame_data.state.att.vector[2] = 0.0
ref_frame_tree.vehicle_frame_data.state.ang_vel[0] = 0.0
ref_frame_tree.vehicle_frame_data.state.ang_vel[1] = 0.1
ref_frame_tree.vehicle_frame_data.state.ang_vel[2] = 0.0
ref_frame_tree.vehicle_frame_data.state.time = 0.0


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
   frame_S_define_instance      = ref_frame_A.frame_packing,
   frame_S_define_instance_name = 'ref_frame_A.frame_packing',
   parent_S_define_instance     = root_ref_frame.frame_packing,
   parent_name                  = root_frame_name,
   frame_conditional            = ref_frame_A.conditional,
   frame_lag_comp               = ref_frame_A.lag_compensation,
   frame_ownership              = ref_frame_A.ownership_handler,
   frame_deleted                = ref_frame_A.deleted_callback )

# Set the debug flag for the root reference frame.
ref_frame_A.frame_packing.debug = verbose

# Add this reference frame to the list of managed object.
federate.add_fed_object( frame_A )

# Set the lag compensation parameters.
# The reality is that the ROOT REFERENCE FRAME never needs to be compensated!
ref_frame_A.lag_compensation.debug = False
ref_frame_A.lag_compensation.set_integ_tolerance( 1.0e-6 )
ref_frame_A.lag_compensation.set_integ_dt( 0.025 )

#frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_NONE )
frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
federate.add_sim_object( root_ref_frame )
federate.add_sim_object( ref_frame_A )
federate.add_sim_object( A )
federate.add_sim_object( P )


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
