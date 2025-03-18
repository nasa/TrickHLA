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
import sys
sys.path.append('../../../')

# Conversions
import math
deg2rad = math.pi / 180.0

# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

def print_usage_message( ):

   print(' ')
   print('SpaceFOM Reference Frame Simulation Command Line Configuration Options:')
   print('  -e --express [frame] : Set express frame for the relative state.')
   print('  -f --frame [frame]   : Set the native frame for the entity.')
   print('  -h --help            : Print this help message.')
   print('  --stop [time]        : Time to stop simulation, default is 0.5 seconds.')
   print('  --nostop             : Set no stop time on simulation.')
   print('  --verbose [on|off]   : on: Show verbose messages (Default), off: disable messages.')
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
   global frames_list
   global express_frame
   global native_frame
   
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
         
      elif ((str(argv[index]) == '-e') | (str(argv[index]) == '--express')):
         index = index + 1
         if (index < argc):
            if str(argv[index]) in frames_list:
               express_frame = str(argv[index])
            else:
               print('ERROR: Unknown express frame: ' + str(argv[index]))
               print_usage = True
         else:
            print('ERROR: Missing express frame name.')
            print_usage = True
         
      elif ((str(argv[index]) == '-f') | (str(argv[index]) == '--frame')):
         index = index + 1
         if (index < argc):
            if str(argv[index]) in frames_list:
               native_frame = str(argv[index])
            else:
               print('ERROR: Unknown native entity frame: ' + str(argv[index]))
               print_usage = True
         else:
            print('ERROR: Missing express frame name.')
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
run_duration = 0.5

# Default is to NOT show verbose messages.
verbose = False

# Create reference frame set.
frames_list = [
   'SolarSystemBarycentricInertial',
   'SunCentricInertial',
   'EarthMoonBarycentricInertial',
   'EarthCentricInertial',
   'EarthCentricFixed',
   'MoonCentricInertial',
   'MoonCentricFixed',
   'MarsCentricInertial',
   'MarsCentricFixed'
   ]

# Set the default frames.
express_frame = frames_list[3]
native_frame = frames_list[4]

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
trick.checkpoint_pre_init(1)
trick.checkpoint_post_init(1)
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
#federate.set_ExCO_S_define_name( 'THLA_INIT.ExCO' )

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
# Set up the frame states.
#---------------------------------------------------------------------------
# Solar system barycenter (Not propagated and ALWAYS zero.)
ssbary_frame.frame.data.name = frames_list[0]
ssbary_frame.frame.data.parent_name = ""
ssbary_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
ssbary_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
ssbary_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
ssbary_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "ssbary_frame.print_state", 1, False )

#
# Sun
#
sun_frame.frame.data.name = frames_list[1]
sun_frame.frame.data.parent_name = frames_list[0]
sun_frame.frame.data.state.pos = [0.1, 0.0, 0.0]
sun_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
sun_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
sun_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "sun_frame.print_state", 1, False )

#
# Earth-Moon system
#
embary_frame.frame.data.name = frames_list[2]
embary_frame.frame.data.parent_name = frames_list[1]
embary_frame.frame.data.state.pos = [0.0, 10.0, 0.0]
embary_frame.frame.data.state.vel = [0.01, 0.0, 0.0]
embary_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
embary_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "embary_frame.print_state", 1, False )

# Earth inertial
earth_inertial_frame.frame.data.name = frames_list[3]
earth_inertial_frame.frame.data.parent_name = frames_list[2]
earth_inertial_frame.frame.data.state.pos = [1.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
earth_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "earth_inertial_frame.print_state", 1, True )

# Earth fixed
earth_fixed_frame.frame.data.name = frames_list[4]
earth_fixed_frame.frame.data.parent_name = frames_list[3]
earth_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
earth_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.001 * deg2rad]
# Set the environmental acceleration values.
earth_fixed_frame.frame.accel_env = [0.0, 0.0, 0.0]
earth_fixed_frame.frame.ang_accel_env = [0.0, 0.0, 0.0000 * deg2rad]
# Control print job.
trick.exec_set_job_onoff( "earth_fixed_frame.print_state", 1, True )

# Moon inertial
moon_inertial_frame.frame.data.name = frames_list[5]
moon_inertial_frame.frame.data.parent_name = frames_list[2]
moon_inertial_frame.frame.data.state.pos = [-1.0, 0.0, 0.0]
moon_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
moon_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "moon_inertial_frame.print_state", 1, False )

# Moon fixed
moon_fixed_frame.frame.data.name = frames_list[6]
moon_fixed_frame.frame.data.parent_name = frames_list[5]
moon_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
moon_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
moon_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "moon_fixed_frame.print_state", 1, False )

#
# Mars system
#
# Mars inertial
mars_inertial_frame.frame.data.name = frames_list[7]
mars_inertial_frame.frame.data.parent_name = frames_list[2]
mars_inertial_frame.frame.data.state.pos = [0.0, 0.0, 20.0]
mars_inertial_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
mars_inertial_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
mars_inertial_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "mars_inertial_frame.print_state", 1, False )

# Moon fixed
mars_fixed_frame.frame.data.name = frames_list[8]
mars_fixed_frame.frame.data.parent_name = frames_list[7]
mars_fixed_frame.frame.data.state.pos = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.vel = [0.0, 0.0, 0.0]
mars_fixed_frame.frame.data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
mars_fixed_frame.frame.data.state.ang_vel = [0.0, 0.0, 0.0]
# Control print job.
trick.exec_set_job_onoff( "mars_fixed_frame.print_state", 1, False )


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
# Set up the dynamics parameters for the vehicle test entity.
#---------------------------------------------------------------------------
vehicle.entity.pe_data.name         = 'Voyager'
vehicle.entity.pe_data.type         = 'Intrepid-class Starship'
vehicle.entity.pe_data.status       = 'Lost'
vehicle.entity.pe_data.parent_frame = native_frame

# Initial translational state.
vehicle.entity.pe_data.state.pos = [ 1.0, 0.0, 0.0 ]
vehicle.entity.pe_data.state.vel = [ 0.0, 0.0, 0.0 ]

# Initial rotational state.
vehicle.entity.pe_data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
vehicle.entity.pe_data.state.ang_vel = [ 0.0, 0.0, 0.0 ]

#
# Basic mass properties.
#
vehicle_mass = 100.0
vehicle.entity.pe_data.cm        = [0.0, 0.0, 0.0]
vehicle.entity.de_data.mass      = vehicle_mass
vehicle.entity.de_data.mass_rate = 0.0
vehicle.entity.pe_data.body_wrt_struct.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )

# Principal inertia of a solid sphere.
vehicle_radius = 1.0
Ixx = Iyy = Izz = (2.0 / 5.0) * vehicle_mass * vehicle_radius * vehicle_radius
vehicle.entity.de_data.inertia[0] = [ Ixx, 0.0, 0.0 ]
vehicle.entity.de_data.inertia[1] = [ 0.0, Iyy, 0.0 ]
vehicle.entity.de_data.inertia[2] = [ 0.0, 0.0, Izz ]
vehicle.entity.de_data.inertia_rate[0] = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.inertia_rate[1] = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.inertia_rate[2] = [ 0.0, 0.0, 0.0 ]

# Base propagation parameters.
vehicle.entity.de_data.force  = [ 0.0, 0.0, 0.0 ]
vehicle.entity.de_data.torque = [ 0.0, 0.0, 0.0 ]

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
trick.exec_set_job_onoff( "ref_frame_tree.frame_tree.print_tree", 1, True )


#---------------------------------------------------------------------------
# Set up the relative state object.
#---------------------------------------------------------------------------
rel_test.rel_state.debug = True
rel_test.ref_entity = vehicle.entity.pe_data
if express_frame == frames_list[0]:
   rel_test.ref_frame  = solar_system_barycenter.frame_packing
elif express_frame == frames_list[1]:
   rel_test.ref_frame  = sun_inertial.frame_packing
elif express_frame == frames_list[2]:
   rel_test.ref_frame  = earth_moon_barycenter.frame_packing
elif express_frame == frames_list[3]:
   rel_test.ref_frame  = earth_centered_inertial.frame_packing
elif express_frame == frames_list[4]:
   rel_test.ref_frame  = earth_centered_fixed.frame_packing
elif express_frame == frames_list[5]:
   rel_test.ref_frame  = moon_centered_inertial.frame_packing
elif express_frame == frames_list[6]:
   rel_test.ref_frame  = moon_centered_fixed.frame_packing
elif express_frame == frames_list[7]:
   rel_test.ref_frame  = mars_centered_inertial.frame_packing
elif express_frame == frames_list[8]:
   rel_test.ref_frame  = mars_centered_fixed.frame_packing


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

