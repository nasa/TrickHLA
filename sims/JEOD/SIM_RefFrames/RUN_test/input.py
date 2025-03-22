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
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
import sys
sys.path.append('../../../')

# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

def print_usage_message( ):

   print(' ')
   print('JEOD-based SpaceFOM Reference Frame Simulation Command Line Configuration Options:')
   print('  -h --help          : Print this help message.')
   print('  --stop [time]      : Time to stop simulation, default is 10.0 seconds.')
   print('  --nostop           : Set no stop time on simulation.')
   print('  --verbose [on|off] : on: Show verbose messages (Default), off: disable messages.')
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.')
   return


def parse_command_line( ):
   
   global print_usage
   global run_duration
   global verbose
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc):
      
      if (str(argv[index]) == '--stop'):
         index = index + 1
         if (index < argc):
            run_duration = float(str(argv[index]))
         else:
            print('ERROR: Missing --stop [time] argument.')
            print_usage = True
            
      elif (str(argv[index]) == '--nostop'):
         run_duration = None
         
      elif ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')):
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
      
      elif (str(argv[index]) == '-d'):
         # Catch the Trick debug command line option an do NOT terminate.
         print('DEBUG: Specified input file debug uption to Trick.')
         
      else:
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
         
      index = index + 1
   return

# Default: Don't show usage.
print_usage = False

# Set the default run duration.
run_duration = 2.0

# Default is to NOT show verbose messages.
verbose = False

parse_command_line()

if (print_usage == True):
   print_usage_message()


#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe(True)
#trick.checkpoint_pre_init(1)
#trick.checkpoint_post_init(1)
#trick.add_read(0.0 , '''trick.checkpoint('chkpnt_point')''')
#trick.checkpoint_end(1)

trick.exec_set_software_frame(0.250)
trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(False)


# =========================================================================
# Set up the JEOD environment.
# =========================================================================
#jeod_time.time_manager_init.initializer = "TAI"
#jeod_time.time_manager_init.sim_start_format = trick.TimeEnum.truncated_julian;
#jeod_time.time_tai.trunc_julian_time = 12345.6789

jeod_time.time_utc.initialize_from_name = "TAI"
#jeod_time.time_tt.initialize_from_name = "TAI"

#jeod_time.time_tai.update_from_name = "Dyn"
jeod_time.time_utc.update_from_name = "TAI"
#jeod_time.time_tt.update_from_name  = "TAI"

# Configure the ephemeris model
env.de4xx.set_model_number(440)
dynamics.dyn_manager_init.mode = trick.DynManagerInit.EphemerisMode_Ephemerides

# Setup the integrator.
dynamics.dyn_manager_init.sim_integ_opt = trick.sim_services.Runge_Kutta_4
dynamics.dyn_manager.deriv_ephem_update = True


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( thla_federate        = THLA.federate,
                                   thla_manager         = THLA.manager,
                                   thla_control         = THLA.execution_control,
                                   thla_config          = THLA.ExCO,
                                   thla_federation_name = 'SpaceFOM_JEOD_Test',
                                   thla_federate_name   = 'JEODRefFrames',
                                   thla_enabled         = True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
#federate.set_config_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if (verbose == True):
   #federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_6_TRACE )
   federate.set_debug_source( trick.TrickHLA.DEBUG_SOURCE_ALL_MODULES )
   #federate.set_debug_source( trick.TrickHLA.DEBUG_SOURCE_OBJECT + trick.TrickHLA.DEBUG_SOURCE_ATTRIBUTE )
else:
   #federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_1_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False ) # This is the Master federate.
federate.set_pacing_role( False ) # This is the Pacing federate.
federate.set_RRFP_role( True )    # This is the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
# This is the RRFP federate.
# It doesn't really need to know about any other federates.
federate.add_known_federate( True, str(federate.federate.name) )
federate.add_known_federate( True, 'Master' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
#THLA.federate.local_settings = 'crcHost = 10.8.0.161\n crcPort = 8989'
# MAK specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'

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

#--------------------------------------------------------------------------
# Set up CTE time line.
#--------------------------------------------------------------------------
# By setting this we are specifying the use of Common Timing Equipment (CTE)
# for controlling the Mode Transitions for all federates using CTE.
# Don't really need CTE for RRFP.
#THLA.execution_control.cte_timeline = trick.sim_services.alloc_type( 1, 'TrickHLA::CTETimelineBase' )


#---------------------------------------------------------------------------
# Set up the Reference Frame objects.
#---------------------------------------------------------------------------

# Set the debug flag for the reference frames.
solar_system_barycenter.frame_packing.debug = verbose
sun_inertial.frame_packing.debug            = verbose
earth_moon_barycenter.frame_packing.debug   = verbose
earth_centered_inertial.frame_packing.debug = verbose
moon_centered_inertial.frame_packing.debug  = verbose
mars_centered_inertial.frame_packing.debug  = verbose
earth_centered_fixed.frame_packing.debug    = verbose
moon_centered_fixed.frame_packing.debug     = verbose
mars_centered_fixed.frame_packing.debug     = verbose

# Mark the frames as published.
solar_system_barycenter.frame_packing.publish();
sun_inertial.frame_packing.publish();
earth_moon_barycenter.frame_packing.publish();
earth_centered_inertial.frame_packing.publish();
moon_centered_inertial.frame_packing.publish();
mars_centered_inertial.frame_packing.publish();
earth_centered_fixed.frame_packing.publish();
moon_centered_fixed.frame_packing.publish();
mars_centered_fixed.frame_packing.publish();

#---------------------------------------------------------------------------
# Set up the Reference Frame Tree
#---------------------------------------------------------------------------
ref_frame_tree.frame_tree.debug = True


#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
federate.add_sim_object( ref_frame_tree )
federate.add_sim_object( solar_system_barycenter )
federate.add_sim_object( sun_inertial )
federate.add_sim_object( earth_moon_barycenter )
federate.add_sim_object( earth_centered_inertial )
federate.add_sim_object( moon_centered_inertial )
federate.add_sim_object( mars_centered_inertial )
federate.add_sim_object( earth_centered_fixed )
federate.add_sim_object( moon_centered_fixed )
federate.add_sim_object( mars_centered_fixed )


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
