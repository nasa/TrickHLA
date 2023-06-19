##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Space Reference FOM
#     example vehicle federate run.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.)
#     (Instantiated the default TrickHLATimeOfDayTimeline based CTE time
#      line for testing.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (June 2023) (--) (JEOD support and testing.)))
##############################################################################
import sys
sys.path.append('../../../')
# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *
# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

def print_usage_message( ):

   print(' ')
   print('TrickHLA SpaceFOM JEOD Master Simulation Command Line Configuration Options:')
   print('  -h -help         : Print this help message.')
   print('  -stop [time]     : Time to stop simulation, default is 10.0 seconds.')
   print('  -nostop          : Set no stop time on simulation.')
   print('  -verbose [on|off]: on: Show verbose messages (Default), off: disable messages.')
   print(' ')

   trick.exec_terminate_with_return( -1,
                                     sys._getframe(0).f_code.co_filename,
                                     sys._getframe(0).f_lineno,
                                     'Print usage message.')
   return


def parse_command_line( ) :
   
   global print_usage
   global run_duration
   global verbose
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc) :
      
      if (str(argv[index]) == '-stop') :
         index = index + 1
         if (index < argc) :
            run_duration = float(str(argv[index]))
         else :
            print('ERROR: Missing -stop [time] argument.')
            print_usage = True
            
      elif (str(argv[index]) == '-nostop') :
         run_duration = None
         
      elif ((str(argv[index]) == '-h') | (str(argv[index]) == '-help')) :
         print_usage = True
      
      elif (str(argv[index]) == '-verbose') :
         index = index + 1
         if (index < argc) :
            if (str(argv[index]) == 'on') :
               verbose = True
            elif (str(argv[index]) == 'off') :
               verbose = False
            else :
               print('ERROR: Unknown -verbose argument: ' + str(argv[index]))
               print_usage = True
         else :
            print('ERROR: Missing -verbose [on|off] argument.')
            print_usage = True
      
      elif (str(argv[index]) == '-d') :
         # Catch the Trick debug command line option an do NOT terminate.
         print('DEBUG: Specified input file debug uption to Trick.')
         
      else :
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

if (print_usage == True) :
   print_usage_message()


#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
#instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe(True)
#trick.checkpoint_pre_init(1)
trick.checkpoint_post_init(1)
#trick.add_read(0.0 , '''trick.checkpoint('chkpnt_point')''')

trick.real_time_enable()
trick.exec_set_software_frame(0.250)
trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(True)


# =========================================================================
# Set up the JEOD environment.
# =========================================================================
jeod_time.time_manager_init.initializer = "UTC"
jeod_time.time_manager_init.sim_start_format = trick.TimeEnum.calendar

jeod_time.time_utc.calendar_year   = 1969
jeod_time.time_utc.calendar_month  =    7
jeod_time.time_utc.calendar_day    =   20
jeod_time.time_utc.calendar_hour   =   12
jeod_time.time_utc.calendar_minute =    0
jeod_time.time_utc.calendar_second =  0.0

jeod_time.time_utc.initialize_from_name = "TAI"
jeod_time.time_tt.initialize_from_name  = "TAI"

jeod_time.time_tai.update_from_name = "Dyn"
jeod_time.time_utc.update_from_name = "TAI"
jeod_time.time_tt.update_from_name  = "TAI"

# Configure the ephemeris model
env.de4xx.set_model_number(440)

# Setup for Lunar orbit.
earth.rnp.rnp_type = trick.PlanetRNP.RotationOnly
earth.rnp.enable_polar = False

# Setup radiation pressure.
vehicle.rad_surface.albedo = 1.0
vehicle.rad_surface.diffuse = .27
vehicle.rad_surface.temperature = 0.0
vehicle.rad_surface.thermal.active = False
vehicle.rad_surface.thermal.thermal_power_dump = 0.0
vehicle.rad_surface.thermal.emissivity = 1.0e-12
vehicle.rad_surface.thermal.heat_capacity = 0.0
vehicle.rad_surface.cx_area  = trick.attach_units( "m2",2.1432)

# Setup atmosphere model.
# No atmosphere for this problem.
vehicle.aero_drag.active = False
vehicle.atmos_state.active = False

# Setup gravity model.
vehicle.sun_grav_control.source_name = "Sun"
vehicle.sun_grav_control.active      = True
vehicle.sun_grav_control.spherical   = True
vehicle.sun_grav_control.gradient    = False

vehicle.earth_grav_control.source_name = "Earth"
vehicle.earth_grav_control.active      = True
vehicle.earth_grav_control.spherical   = True
vehicle.earth_grav_control.gradient    = False

vehicle.moon_grav_control.source_name = "Moon"
vehicle.moon_grav_control.active      = True
vehicle.moon_grav_control.spherical   = False
vehicle.moon_grav_control.gradient    = False
vehicle.moon_grav_control.degree      = 60
vehicle.moon_grav_control.order       = 60

vehicle.mars_grav_control.source_name = "Mars"
vehicle.mars_grav_control.active      = False
vehicle.mars_grav_control.spherical   = True
vehicle.mars_grav_control.gradient    = False

vehicle.dyn_body.grav_interaction.add_control(vehicle.sun_grav_control)
vehicle.dyn_body.grav_interaction.add_control(vehicle.earth_grav_control)
vehicle.dyn_body.grav_interaction.add_control(vehicle.moon_grav_control)
vehicle.dyn_body.grav_interaction.add_control(vehicle.mars_grav_control)

# Setup mass properties.
#defaults units are SI unless otherwise stated.
vehicle.mass_init.set_subject_body( vehicle.dyn_body.mass )
vehicle.mass_init.properties.mass = 424.0
vehicle.mass_init.properties.pt_orientation.data_source =    trick.Orientation.InputEigenRotation
vehicle.mass_init.properties.pt_orientation.eigen_angle = 0.0
vehicle.mass_init.properties.pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]
vehicle.mass_init.properties.position    = [ 0.0, 0.0, 0.0]
vehicle.mass_init.properties.inertia[0]  = [ 1.0, 0.0, 0.0]
vehicle.mass_init.properties.inertia[1]  = [ 0.0, 1.0, 0.0]
vehicle.mass_init.properties.inertia[2]  = [ 0.0, 0.0, 1.0]

# Set initial state.
vehicle.pfix.reference_name     = "Moon"
vehicle.lvlh.reference_name     = "Moon"
vehicle.orb_elem.reference_name = "Moon"

vehicle.trans_init.set_subject_body( vehicle.dyn_body )
vehicle.trans_init.reference_ref_frame_name = "Moon.inertial"
vehicle.trans_init.body_frame_id            = "composite_body"

vehicle.lvlh_init.set_subject_body( vehicle.dyn_body )
vehicle.lvlh_init.planet_name                = "Moon"
vehicle.lvlh_init.body_frame_id              = "composite_body"
vehicle.lvlh_init.orientation.data_source    = trick.Orientation.InputEulerRotation
vehicle.lvlh_init.orientation.euler_sequence = trick.Orientation.Yaw_Pitch_Roll
vehicle.lvlh_init.orientation.euler_angles   = [ 0.0, 0.0, 0.0]
vehicle.lvlh_init.ang_velocity               = [ 0.0, 0.0, 0.0]

vehicle.trans_init.position  = trick.attach_units( "km",[  1296.944012, -1060.824450, 2522.289146])
vehicle.trans_init.velocity  = trick.attach_units( "km/s",[ -.930578, -.439312, .862075])

# Setup Dynamics Manager info.
dynamics.dyn_manager.add_body_action(vehicle.mass_init)
dynamics.dyn_manager.add_body_action(vehicle.trans_init)
dynamics.dyn_manager.add_body_action(vehicle.lvlh_init)

# Configure vehicle integration information.
vehicle.dyn_body.set_name( "PhysicalEntity" )
vehicle.dyn_body.integ_frame_name       = "Moon.inertial"
vehicle.dyn_body.translational_dynamics = True
vehicle.dyn_body.rotational_dynamics    = True

# Setup the integrator.
dynamics.dyn_manager_init.sim_integ_opt = trick.sim_services.Runge_Kutta_4
dynamics.dyn_manager.deriv_ephem_update = True


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.ExCO,
                                   'SpaceFOM_JEOD_Test',
                                   'Vehicle',
                                   True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
#federate.set_ExCO_S_define_name( 'THLA_INIT.ExCO' )


# Set the debug output level.
if (verbose == True) : 
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
#   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_9_TRACE )
else :
#   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_1_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False ) # This is NOT the Master federate.
federate.set_pacing_role( False ) # This is NOT the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_fededrate( True, str(federate.federate.name) )
federate.add_known_fededrate( True, 'Master' )
federate.add_known_fededrate( True, 'JEODRefFrames' )

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
# Pull the scenario timeline epoch from JEOD.
federate.set_scenario_timeline_epoch( jeod_time.time_tt.trunc_julian_time * 86400.0 )

# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( 0.250 )

# Must specify the Least Common Time Step for all federates in the
# federation execution.
federate.set_least_common_time_step( 250000 )

# Set the amount of seconds used to 'pad' mode transitions.
federate.set_time_padding( 1.0 )

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


#---------------------------------------------------------------------------
# Set up the Reference Frame objects.
#---------------------------------------------------------------------------

# Set the debug flag for the reference frames.
solar_system_barycenter.frame_packing.debug = True
sun_inertial.frame_packing.debug = verbose
earth_moon_barycenter.frame_packing.debug = verbose
earth_centered_inertial.frame_packing.debug = True
moon_centered_inertial.frame_packing.debug = verbose
mars_centered_inertial.frame_packing.debug = verbose
earth_centered_fixed.frame_packing.debug = True
moon_centered_fixed.frame_packing.debug = verbose
mars_centered_fixed.frame_packing.debug = verbose


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

# FIXME: Override the object count.
THLA.manager.obj_count = 9


#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
if run_duration:
   trick.sim_services.exec_set_terminate_time( run_duration )
