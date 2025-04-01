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
sys.path.append('../../../')

# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

def print_usage_message( ):

   print(' ')
   print('SpaceFOM Reference Frame Simulation Command Line Configuration Options:')
   print('  -c --case [name]     : Set the name for the case definition file.')
   print('  -d                   : Pass the input file debug option on to Trick.')
   print('  -e --express [frame] : Set express frame for the relative state.')
   print('  -f --frames [file]   : Set the name for the frames definition file.')
   print('  -h --help            : Print this help message.')
   print('  -n --native [frame]  : Set the native frame for the entity.')
   print('  --nostop             : Set no stop time on simulation.')
   print('  --stop [time]        : Time to stop simulation, default is 0.5 seconds.')
   print('  -t --tree            : Print the frame tree data.')
   print('  -v --vehicle [file]  : Set the name for the vehicle definition file.')
   print('  --verbose [on|off]   : on: Show verbose messages (Default), off: disable messages.')
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.')
   return


def parse_command_line( ):
   
   # Execution control parameters
   global print_usage
   global run_duration
   global verbose
   global config_dir
   global case_name
   global case_file
   
   # Frame definition parameters
   global print_tree
   global frames_list
   global express_frame
   global native_frame
   global frames_name
   global frames_file
   
   # Vehicle definition parameters
   global vehicle_name
   global vehicle_file
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc):
         
      if ((str(argv[index]) == '-c') | (str(argv[index]) == '--case')):
         index = index + 1
         if (index < argc):
            case_name = str(argv[index])
            case_file = config_dir + "/" + case_name + ".py"
            if not os.path.exists( frames_file ):
               print('ERROR: Frames file not found: ' + frames_file)
               print_usage = True
         else:
            print('ERROR: Missing frames name.')
            print_usage = True
      
      elif (str(argv[index]) == '-d'):
         # Catch the Trick debug command line option an do NOT terminate.
         print('DEBUG: Specified input file debug option to Trick.')
         
      elif ((str(argv[index]) == '-e') | (str(argv[index]) == '--express')):
         index = index + 1
         if (index < argc):
            express_frame = str(argv[index])
         else:
            print('ERROR: Missing express frame name.')
            print_usage = True
         
      elif ((str(argv[index]) == '-f') | (str(argv[index]) == '--frames')):
         index = index + 1
         if (index < argc):
            frames_name = str(argv[index])
            frames_file = config_dir + "/" + frames_name + ".py"
            if not os.path.exists( frames_file ):
               print('ERROR: Frames file not found: ' + frames_file)
               print_usage = True
         else:
            print('ERROR: Missing frames name.')
            print_usage = True
      
      elif (str(argv[index]) == '--stop'):
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
         
      elif ((str(argv[index]) == '-n') | (str(argv[index]) == '--native')):
         index = index + 1
         if (index < argc):
            native_frame = str(argv[index])
         else:
            print('ERROR: Missing entity frame name.')
            print_usage = True
         
      elif ((str(argv[index]) == '-t') | (str(argv[index]) == '--tree')):
         print_tree = True
         
      elif ((str(argv[index]) == '-v') | (str(argv[index]) == '--vehicle')):
         index = index + 1
         if (index < argc):
            vehicle_name = str(argv[index])
            vehicle_file = config_dir + "/" + vehicle_name + ".py"
            if not os.path.exists( vehicle_file ):
               print('ERROR: Vehicle file not found: ' + vehicle_file)
               print_usage = True
         else:
            print('ERROR: Missing vehicle name.')
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
         
      else:
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
         
      index = index + 1
   return

# Default: Don't show usage.
print_usage = False

# Set the default run duration.
run_duration = 0.5

# Default is to NOT show verbose messages.
verbose = False

# Default Trick run configuration directory.
config_dir = 'Modified_data'

# Set the default frames name.
frames_name = 'emm_test_frames'
frames_file = config_dir + '/' + frames_name + '.py'

# Default is not to print the frames tree.
print_tree = False

# Set the default vehicle name.
vehicle_name = 'test_vehicle'
vehicle_file = config_dir + '/' + vehicle_name + '.py'

# Set the default frames.
express_frame = 'EarthCentricFixed'
native_frame  = 'EarthCentricInertial'

# Set the default case name.
case_name = None
case_file = None

# Parse command line arguments to override defaults.
parse_command_line()

# Check for print usage flag.  Terminates in call.
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
trick.exec_set_freeze_command(False)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(False)


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( thla_federate        = THLA.federate,
                                   thla_manager         = THLA.manager,
                                   thla_control         = THLA.execution_control,
                                   thla_config          = THLA.ExCO,
                                   thla_federation_name = 'SpaceFOM_Frames_Test',
                                   thla_federate_name   = 'SpaceFOMRefFrames',
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
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
   #federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_2_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( True ) # This is the Master federate.
federate.set_pacing_role( True ) # This is the Pacing federate.
federate.set_RRFP_role( True )   # This is the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
# This is the RRFP federate.
# It doesn't really need to know about any other federates.
federate.add_known_federate( True, str(federate.federate.name) )
#federate.add_known_federate( True, 'Master' )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'

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
#THLA.execution_control.cte_timeline = trick.sim_services.alloc_type( 1, 'TrickHLA::CTETimelineBase' )


#---------------------------------------------------------------------------
# Load the case file if specified.  Note that you can only override the
# configuration file locations.  Frame and vehicle data should still be
# configured in specified frame and vehicle input data files.
#---------------------------------------------------------------------------
if ( case_file ):
   if os.path.exists( case_file ):
      exec(open(case_file).read())
   else:
      print('ERROR: Case file not found: ' + case_file)
      print_usage_message()


#---------------------------------------------------------------------------
# Set up the frame states.
#---------------------------------------------------------------------------
if os.path.exists( frames_file ):
   exec(open(frames_file).read())
else:
   print('ERROR: Frames file not found: ' + frames_file)
   print_usage_message()

# Do a sanity check on selected frame names.   
if not native_frame in frames_list:
   print('ERROR: Unknown native entity frame: ' + native_frame)
   print_usage_message()
 
if not express_frame in frames_list:
   print('ERROR: Unknown express frame: ' + express_frame)
   print_usage_message()


#---------------------------------------------------------------------------
# Setup the frame integrators
#---------------------------------------------------------------------------
sun_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
embary_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
earth_inertial_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
earth_fixed_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
moon_inertial_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
moon_fixed_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
mars_inertial_loop.getIntegrator( trick.Runge_Kutta_4, 13 )
mars_fixed_loop.getIntegrator( trick.Runge_Kutta_4, 13 )


#---------------------------------------------------------------------------
# Set up the vehicle state.
#---------------------------------------------------------------------------
if os.path.exists( vehicle_file ):
   exec(open(vehicle_file).read())
else:
   print('ERROR: Vehicle file not found: ' + vehicle_file)
   print_usage_message()


#---------------------------------------------------------------------------
# Setup the vehicle integrator
#---------------------------------------------------------------------------
vehicle_loop.getIntegrator( trick.Runge_Kutta_4, 13 )


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
solar_system_barycenter.frame_packing.publish()
sun_inertial.frame_packing.publish()
earth_moon_barycenter.frame_packing.publish()
earth_centered_inertial.frame_packing.publish()
moon_centered_inertial.frame_packing.publish()
mars_centered_inertial.frame_packing.publish()
earth_centered_fixed.frame_packing.publish()
moon_centered_fixed.frame_packing.publish()
mars_centered_fixed.frame_packing.publish()

#---------------------------------------------------------------------------
# Set up the Reference Frame Tree
#---------------------------------------------------------------------------
ref_frame_tree.frame_tree.debug = True
trick.exec_set_job_onoff( "ref_frame_tree.frame_tree.print_tree", 1, print_tree )


#---------------------------------------------------------------------------
# Set up the relative state object.
#---------------------------------------------------------------------------
rel_test.debug_rel_state    = True
rel_test.debug_entity_state = True
rel_test.debug_frames       = False
rel_test.rel_state.debug    = False
if express_frame == frames_list[0]:
   rel_test.rel_state_frame  = solar_system_barycenter.frame_packing
elif express_frame == frames_list[1]:
   rel_test.rel_state_frame  = sun_inertial.frame_packing
elif express_frame == frames_list[2]:
   rel_test.rel_state_frame  = earth_moon_barycenter.frame_packing
elif express_frame == frames_list[3]:
   rel_test.rel_state_frame  = earth_centered_inertial.frame_packing
elif express_frame == frames_list[4]:
   rel_test.rel_state_frame  = earth_centered_fixed.frame_packing
elif express_frame == frames_list[5]:
   rel_test.rel_state_frame  = moon_centered_inertial.frame_packing
elif express_frame == frames_list[6]:
   rel_test.rel_state_frame  = moon_centered_fixed.frame_packing
elif express_frame == frames_list[7]:
   rel_test.rel_state_frame  = mars_centered_inertial.frame_packing
elif express_frame == frames_list[8]:
   rel_test.rel_state_frame  = mars_centered_fixed.frame_packing


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

if run_duration != None:
   if run_duration == 0.0:
      trick.stop(0.0)
   else:
      trick.sim_services.exec_set_terminate_time( run_duration )

