##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Propagated federate
#     for the sine wave example that uses the Space Reference FOM.)
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
   print('TrickHLA SpaceFOM Roles Test Simulation Command Line Configuration Options:')
   print('  -h --help             : Print this help message.')
   print('  -f --fed_name [name]  : Name of the Federate, default is P-side-Federate.')
   print('  -fe --fex_name [name] : Name of the Federation Execution, default is SpaceFOM_sine.')
   print('  --nostop              : Set no stop time on simulation.')
   print('  -s --stop [time]      : Time to stop simulation, default is 10.0 seconds.')
   print('  --verbose [on|off]    : on: Show verbose messages (Default), off: disable messages.')
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

      elif (str(argv[index]) == '-nostop'):
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
federate_name = 'P-side-Federate'

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_sine_zero_lookahead'


parse_command_line()

if (print_usage == True):
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

trick.exec_set_enable_freeze( False )
trick.exec_set_freeze_command( False )
trick.sim_control_panel_set_enabled( False )
trick.exec_set_stack_trace( False )

#---------------------------------------------
# Set up data to record.
#---------------------------------------------
exec(open( "Log_data/log_sine_states.py" ).read())
log_sine_states( 'AZ', 0.250 )
log_sine_states( 'PZ', 0.250 )
log_sine_states( 'AC', 0.250 )
log_sine_states( 'PC', 0.250 )


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
federate.set_master_role( False ) # This is NOT the Master federate.
federate.set_pacing_role( False ) # This is NOT the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_federate( True, 'A-side-Federate' )
federate.add_known_federate( True, str(federate.federate.name) )

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
# Set up federate related time related parameters.
#--------------------------------------------------------------------------
# Set the simulation timeline to be used for time computations.
THLA.execution_control.sim_timeline = THLA_INIT.sim_timeline

# Set the scenario timeline to be used for configuring federation freeze times.
THLA.execution_control.scenario_timeline = THLA_INIT.scenario_timeline

# Specify the HLA base time units (default: trick.HLA_BASE_TIME_MICROSECONDS).
federate.set_HLA_base_time_units( trick.HLA_BASE_TIME_MICROSECONDS )

# Scale the Trick Time Tic value based on the HLA base time units.
federate.scale_trick_tics_to_base_time_units()

# Zero-lookhead requires a federate HLA lookahead value of zero.
federate.set_lookahead_time( 0.0 )

# Must specify a Trick software frame that meets the time constraints
# for the Least Common Time Step (LCTS) value set in the ExCO by the
# Master federate. (LCTS >= RT) && (LCTS % RT = 0)
trick.exec_set_software_frame( 0.250 )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )


#---------------------------------------------
# Set up the initial Sine states
#---------------------------------------------
exec(open( "Modified_data/sine_init.py" ).read())

# Example of a 1-dimensional dynamic array.
AZ.packing.buff_size = 10
AZ.packing.buff = trick.sim_services.alloc_type( AZ.packing.buff_size, 'unsigned char' )
PZ.packing.buff_size = 10
PZ.packing.buff = trick.sim_services.alloc_type( PZ.packing.buff_size, 'unsigned char' )
AC.packing.buff_size = 10
AC.packing.buff = trick.sim_services.alloc_type( AC.packing.buff_size, 'unsigned char' )
PC.packing.buff_size = 10
PC.packing.buff = trick.sim_services.alloc_type( PC.packing.buff_size, 'unsigned char' )

# We are taking advantage of the input file to specify a unique name for the
# sim-data name field for the P-side federate.
AZ.sim_data.name = 'AZ.sim_data.name.P-side.zero'
PZ.sim_data.name = 'PZ.sim_data.name.P-side.zero'
AC.sim_data.name = 'AC.sim_data.name.P-side.cyclic'
PC.sim_data.name = 'PC.sim_data.name.P-side.cyclic'


#---------------------------------------------------------------------------
# Set up for Sine data.
#---------------------------------------------------------------------------

sine_AZ = SineObject( sine_create_object      = False,
                      sine_obj_instance_name  = 'A-side-Federate.Sine.zero',
                      sine_trick_sim_obj_name = 'AZ',
                      sine_packing            = AZ.packing,
                      sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AZ )

sine_PZ = SineObject( sine_create_object      = True,
                      sine_obj_instance_name  = 'P-side-Federate.Sine.zero',
                      sine_trick_sim_obj_name = 'PZ',
                      sine_packing            = PZ.packing,
                      sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_PZ )

sine_AC = SineObject( sine_create_object      = False,
                      sine_obj_instance_name  = 'A-side-Federate.Sine.cyclic',
                      sine_trick_sim_obj_name = 'AC',
                      sine_packing            = AC.packing )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AC )

sine_PC = SineObject( sine_create_object      = True,
                      sine_obj_instance_name  = 'P-side-Federate.Sine.cyclic',
                      sine_trick_sim_obj_name = 'PC',
                      sine_packing            = PC.packing )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_PC )

#---------------------------------------------------------------------------
# Set up the Root Reference Frame object for discovery.
# If it is the RRFP, it will publish the frame.
# If it is NOT the RRFP, it will subscribe to the frame.
#---------------------------------------------------------------------------
root_frame = SpaceFOMRefFrameObject(
   create_frame_object          = federate.is_RRFP,
   frame_instance_name          = 'RootFrame',
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
   create_frame_object          = False,
   frame_instance_name          = 'FrameA',
   frame_S_define_instance      = ref_frame_A.frame_packing,
   frame_S_define_instance_name = 'ref_frame_A.frame_packing',
   parent_S_define_instance     = root_ref_frame.frame_packing,
   parent_name                  = 'RootFrame',
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
federate.add_sim_object( AZ )
federate.add_sim_object( PZ )
federate.add_sim_object( AC )
federate.add_sim_object( PC )


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


#---------------------------------------------------------------------------
# Send a timed MTR request to the Master federate.
#---------------------------------------------------------------------------
# Send an interaction.
#trick.add_read(3.0 , """THLA.manager.send_MTR_interation( trick.SpaceFOM.MTR_GOTO_SHUTDOWN )""")
#trick.add_read(3.0 , """THLA.manager.send_MTR_interation( trick.SpaceFOM.MTR_GOTO_FREEZE )""")
