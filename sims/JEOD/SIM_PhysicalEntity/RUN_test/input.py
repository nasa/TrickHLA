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

# Load the SpaceFOM/JEOD specific reference frame tree configuration object.
from Modified_data.JEOD.JEODRefFrameTreeObject import *

# Load the SpaceFOM vehicle PhysicalEntity configuration object.
from Modified_data.SpaceFOM.SpaceFOMPhysicalEntityObject import *

# Load the SpaceFOM vehicle PhysicalInterface configuration object.
from Modified_data.SpaceFOM.SpaceFOMPhysicalInterfaceObject import *

def print_usage_message( ):

   print(' ')
   print('TrickHLA SpaceFOM JEOD Master Simulation Command Line Configuration Options:')
   print('  -a --active [name]    : Name of the active vehicle, default is Lander.')
   print('  -h --help             : Print this help message.')
   print('  -f --fed_name [name]  : Name of the Federate, default is EntityTest.')
   print('  -fe --fex_name [name] : Name of the Federation Execution, default is SpaceFOM_JEOD_Test.')
   print('  -m --master [name]    : Name of the Master federate, default is Master.')
   print('  --nostop              : Set no stop time on simulation.')
   print('  -p --passive[name]    : Name of the passive vehicle, default is Station.')
   print('  -s --stop [time]      : Time to stop simulation, default is 10.0 seconds.')
   print('  --verbose [on|off]    : on: Show verbose messages (Default), off: disable messages.')
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
   global federation_name
   global federate_name
   global master_name
   global active_entity_name
   global active_interface_name
   global passive_entity_name
   global passive_interface_name
   
   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()
   
   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while (index < argc):
         
      if ((str(argv[index]) == '-a') | (str(argv[index]) == '--active')):
         index = index + 1
         if (index < argc):
            active_entity_name = str(argv[index])
         else:
            print('ERROR: Missing --active [name] argument.')
            print_usage = True
            
      elif ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')):
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
      
      elif ((str(argv[index]) == '-m') | (str(argv[index]) == '--master')):
         index = index + 1
         if (index < argc):
            master_name = str(argv[index])
         else:
            print('ERROR: Missing --master [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-p') | (str(argv[index]) == '--passive')):
         index = index + 1
         if (index < argc):
            passive_entity_name = str(argv[index])
         else:
            print('ERROR: Missing --passive [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-s') | (str(argv[index]) == '--stop')):
         index = index + 1
         if (index < argc):
            run_duration = float(str(argv[index]))
         else:
            print('ERROR: Missing -stop [time] argument.')
            print_usage = True
            
      elif (str(argv[index]) == '--nostop'):
         run_duration = None
      
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

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_JEOD_Test'

# Set the default Federate name.
federate_name = 'EntityTest'

# Set the default Master federate name.
master_name = 'Master'

# Set the default DynamicalEntity instance name.
active_entity_name = 'Lander'

# Set the active interface name.
active_interface_name = active_entity_name + '.docking_port'

# Set the default PhysicalEntity instance name.
passive_entity_name = 'Station'

# Set the passive interface name.
passive_interface_name = passive_entity_name + '.docking_port'

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

# Setup the integrator.
dynamics.dyn_manager_init.sim_integ_opt = trick.sim_services.Runge_Kutta_4
dynamics.dyn_manager.deriv_ephem_update = True

#
# Setup the active vehicle.
#
# Setup radiation pressure.
active_vehicle.rad_surface.albedo = 1.0
active_vehicle.rad_surface.diffuse = .27
active_vehicle.rad_surface.temperature = 0.0
active_vehicle.rad_surface.thermal.active = False
active_vehicle.rad_surface.thermal.thermal_power_dump = 0.0
active_vehicle.rad_surface.thermal.emissivity = 1.0e-12
active_vehicle.rad_surface.thermal.heat_capacity = 0.0
active_vehicle.rad_surface.cx_area = trick.attach_units( "m2",2.1432)

# Setup atmosphere model.
# No atmosphere for this problem.
active_vehicle.aero_drag.active = False
active_vehicle.atmos_state.active = False

# Setup gravity model.
active_vehicle.sun_grav_control.source_name = "Sun"
active_vehicle.sun_grav_control.active      = True
active_vehicle.sun_grav_control.spherical   = True
active_vehicle.sun_grav_control.gradient    = False

active_vehicle.earth_grav_control.source_name = "Earth"
active_vehicle.earth_grav_control.active      = True
active_vehicle.earth_grav_control.spherical   = True
active_vehicle.earth_grav_control.gradient    = False

active_vehicle.moon_grav_control.source_name = "Moon"
active_vehicle.moon_grav_control.active      = True
active_vehicle.moon_grav_control.spherical   = False
active_vehicle.moon_grav_control.gradient    = False
active_vehicle.moon_grav_control.degree      = 60
active_vehicle.moon_grav_control.order       = 60

active_vehicle.mars_grav_control.source_name = "Mars"
active_vehicle.mars_grav_control.active      = False
active_vehicle.mars_grav_control.spherical   = True
active_vehicle.mars_grav_control.gradient    = False

active_vehicle.dyn_body.grav_interaction.add_control(active_vehicle.sun_grav_control)
active_vehicle.dyn_body.grav_interaction.add_control(active_vehicle.earth_grav_control)
active_vehicle.dyn_body.grav_interaction.add_control(active_vehicle.moon_grav_control)
active_vehicle.dyn_body.grav_interaction.add_control(active_vehicle.mars_grav_control)

# Setup mass properties.
#defaults units are SI unless otherwise stated.
active_vehicle.mass_init.set_subject_body( active_vehicle.dyn_body.mass )
active_vehicle.mass_init.properties.mass = 424.0
active_vehicle.mass_init.properties.pt_orientation.data_source =    trick.Orientation.InputEigenRotation
active_vehicle.mass_init.properties.pt_orientation.eigen_angle = 0.0
active_vehicle.mass_init.properties.pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]
active_vehicle.mass_init.properties.position    = [ 0.0, 0.0, 0.0]
active_vehicle.mass_init.properties.inertia[0]  = [ 1.0, 0.0, 0.0]
active_vehicle.mass_init.properties.inertia[1]  = [ 0.0, 1.0, 0.0]
active_vehicle.mass_init.properties.inertia[2]  = [ 0.0, 0.0, 1.0]

# Create the docking port mass point interface.
active_vehicle.mass_init.set_subject_body( active_vehicle.dyn_body.mass )
active_vehicle.mass_init.allocate_points(1)

active_vehicle.mass_init.get_mass_point(0).set_name("Active docking port")
active_vehicle.mass_init.get_mass_point(0).pt_frame_spec              = trick.MassPointInit.StructToBody
active_vehicle.mass_init.get_mass_point(0).position                   = trick.attach_units( "m",[ 1.0, 0.0, 0.0])
active_vehicle.mass_init.get_mass_point(0).pt_orientation.data_source = trick.Orientation.InputEigenRotation

active_vehicle.mass_init.get_mass_point(0).pt_orientation.eigen_angle = trick.attach_units( "degree",0.0)
active_vehicle.mass_init.get_mass_point(0).pt_orientation.eigen_axis  = [ 0.0, 0.0, 1.0]

# Set initial state.
active_vehicle.pfix.reference_name     = "Moon"
active_vehicle.lvlh.reference_name     = "Moon"
active_vehicle.orb_elem.reference_name = "Moon"

active_vehicle.trans_init.set_subject_body( active_vehicle.dyn_body )
active_vehicle.trans_init.reference_ref_frame_name = "Moon.inertial"
active_vehicle.trans_init.body_frame_id            = "composite_body"

active_vehicle.trans_init.position = trick.attach_units( "km",[  1296.944012, -1060.824450, 2522.289146])
active_vehicle.trans_init.velocity = trick.attach_units( "km/s",[ -.930578, -.439312, .862075])

active_vehicle.lvlh_init.set_subject_body( active_vehicle.dyn_body )
active_vehicle.lvlh_init.planet_name                = "Moon"
active_vehicle.lvlh_init.body_frame_id              = "composite_body"
active_vehicle.lvlh_init.orientation.data_source    = trick.Orientation.InputEulerRotation
active_vehicle.lvlh_init.orientation.euler_sequence = trick.Orientation.Yaw_Pitch_Roll
active_vehicle.lvlh_init.orientation.euler_angles   = [ 0.0, 0.0, 0.0]
active_vehicle.lvlh_init.ang_velocity               = [ 0.0, 0.0, 0.0]

# Configure vehicle integration information.
active_vehicle.dyn_body.set_name( active_entity_name )
active_vehicle.dyn_body.integ_frame_name       = "Moon.inertial"
active_vehicle.dyn_body.translational_dynamics = True
active_vehicle.dyn_body.rotational_dynamics    = True

# Setup Dynamics Manager info.
dynamics.dyn_manager.add_body_action(active_vehicle.mass_init)
dynamics.dyn_manager.add_body_action(active_vehicle.trans_init)
dynamics.dyn_manager.add_body_action(active_vehicle.lvlh_init)

#
# Setup the passive vehicle.
#
# Setup radiation pressure.
passive_vehicle.rad_surface.albedo = 1.0
passive_vehicle.rad_surface.diffuse = .27
passive_vehicle.rad_surface.temperature = 0.0
passive_vehicle.rad_surface.thermal.active = False
passive_vehicle.rad_surface.thermal.thermal_power_dump = 0.0
passive_vehicle.rad_surface.thermal.emissivity = 1.0e-12
passive_vehicle.rad_surface.thermal.heat_capacity = 0.0
passive_vehicle.rad_surface.cx_area  = trick.attach_units( "m2",2.1432)

# Setup atmosphere model.
# No atmosphere for this problem.
passive_vehicle.aero_drag.active = False
passive_vehicle.atmos_state.active = False

# Setup gravity model.
passive_vehicle.sun_grav_control.source_name = "Sun"
passive_vehicle.sun_grav_control.active      = True
passive_vehicle.sun_grav_control.spherical   = True
passive_vehicle.sun_grav_control.gradient    = False

passive_vehicle.earth_grav_control.source_name = "Earth"
passive_vehicle.earth_grav_control.active      = True
passive_vehicle.earth_grav_control.spherical   = True
passive_vehicle.earth_grav_control.gradient    = False

passive_vehicle.moon_grav_control.source_name = "Moon"
passive_vehicle.moon_grav_control.active      = True
passive_vehicle.moon_grav_control.spherical   = False
passive_vehicle.moon_grav_control.gradient    = False
passive_vehicle.moon_grav_control.degree      = 60
passive_vehicle.moon_grav_control.order       = 60

passive_vehicle.mars_grav_control.source_name = "Mars"
passive_vehicle.mars_grav_control.active      = False
passive_vehicle.mars_grav_control.spherical   = True
passive_vehicle.mars_grav_control.gradient    = False

passive_vehicle.dyn_body.grav_interaction.add_control(passive_vehicle.sun_grav_control)
passive_vehicle.dyn_body.grav_interaction.add_control(passive_vehicle.earth_grav_control)
passive_vehicle.dyn_body.grav_interaction.add_control(passive_vehicle.moon_grav_control)
passive_vehicle.dyn_body.grav_interaction.add_control(passive_vehicle.mars_grav_control)

# Setup mass properties.
#defaults units are SI unless otherwise stated.
passive_vehicle.mass_init.set_subject_body( passive_vehicle.dyn_body.mass )
passive_vehicle.mass_init.properties.mass = 424.0
passive_vehicle.mass_init.properties.pt_orientation.data_source =    trick.Orientation.InputEigenRotation
passive_vehicle.mass_init.properties.pt_orientation.eigen_angle = 0.0
passive_vehicle.mass_init.properties.pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]
passive_vehicle.mass_init.properties.position    = [ 0.0, 0.0, 0.0]
passive_vehicle.mass_init.properties.inertia[0]  = [ 1.0, 0.0, 0.0]
passive_vehicle.mass_init.properties.inertia[1]  = [ 0.0, 1.0, 0.0]
passive_vehicle.mass_init.properties.inertia[2]  = [ 0.0, 0.0, 1.0]

# Create the docking port mass point interface.
passive_vehicle.mass_init.set_subject_body( passive_vehicle.dyn_body.mass )
passive_vehicle.mass_init.allocate_points(1)

passive_vehicle.mass_init.get_mass_point(0).set_name("Passive docking port")
passive_vehicle.mass_init.get_mass_point(0).pt_frame_spec              = trick.MassPointInit.StructToBody
passive_vehicle.mass_init.get_mass_point(0).position                   = trick.attach_units( "m",[ 0.0, 1.0, 0.0])
passive_vehicle.mass_init.get_mass_point(0).pt_orientation.data_source = trick.Orientation.InputEigenRotation

passive_vehicle.mass_init.get_mass_point(0).pt_orientation.eigen_angle = trick.attach_units( "degree",0.0)
passive_vehicle.mass_init.get_mass_point(0).pt_orientation.eigen_axis  = [ 0.0, 1.0, 0.0]

# Set initial state.
passive_vehicle.pfix.reference_name     = "Moon"
passive_vehicle.lvlh.reference_name     = "Moon"
passive_vehicle.orb_elem.reference_name = "Moon"

passive_vehicle.trans_init.set_subject_body( passive_vehicle.dyn_body )
passive_vehicle.trans_init.reference_ref_frame_name = "Moon.inertial"
passive_vehicle.trans_init.body_frame_id            = "composite_body"

passive_vehicle.lvlh_init.set_subject_body( passive_vehicle.dyn_body )
passive_vehicle.lvlh_init.planet_name                = "Moon"
passive_vehicle.lvlh_init.body_frame_id              = "composite_body"
passive_vehicle.lvlh_init.orientation.data_source    = trick.Orientation.InputEulerRotation
passive_vehicle.lvlh_init.orientation.euler_sequence = trick.Orientation.Yaw_Pitch_Roll
passive_vehicle.lvlh_init.orientation.euler_angles   = [ 0.0, 0.0, 0.0]
passive_vehicle.lvlh_init.ang_velocity               = [ 0.0, 0.0, 0.0]

passive_vehicle.trans_init.position = trick.attach_units( "km",[  1296.944012, -1060.824450, 2522.289146])
passive_vehicle.trans_init.velocity = trick.attach_units( "km/s",[ -.930578, -.439312, .862075])

# Configure vehicle integration information.
passive_vehicle.dyn_body.set_name( passive_entity_name )
passive_vehicle.dyn_body.integ_frame_name       = "Moon.inertial"
passive_vehicle.dyn_body.translational_dynamics = True
passive_vehicle.dyn_body.rotational_dynamics    = True

# Setup Dynamics Manager info.
dynamics.dyn_manager.add_body_action(passive_vehicle.mass_init)
dynamics.dyn_manager.add_body_action(passive_vehicle.trans_init)
dynamics.dyn_manager.add_body_action(passive_vehicle.lvlh_init)


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
federate.set_master_role( False ) # This is NOT the Master federate.
federate.set_pacing_role( False ) # This is NOT the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_federate( True, str(federate.federate.name) )
federate.add_known_federate( True, master_name )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'

#--------------------------------------------------------------------------
# Set up federate related time related parameters.
#--------------------------------------------------------------------------
# Pull the scenario timeline epoch from JEOD.
federate.set_scenario_timeline_epoch( jeod_time.time_tt.trunc_julian_time * 86400.0 )

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
THLA.execution_control.cte_timeline = trick.sim_services.alloc_type( 1, 'TrickHLA::CTETimelineBase' )


#---------------------------------------------------------------------------
# Set up the Reference Frame objects.
#---------------------------------------------------------------------------
frame_tree = JEODRefFrameTreeObject(
   federate_instance    = federate,
   tree_instance        = ref_frame_tree,
   create_frame_objects = False,
   lag_comp_type        = trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )

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


#---------------------------------------------------------------------------
# Set up the lander PhysicalEntity object.
#---------------------------------------------------------------------------
lander = SpaceFOMPhysicalEntityObject(
   create_entity_object          = True,
   entity_instance_name          = active_entity_name,
   entity_S_define_instance      = active_physical_entity.entity_packing,
   entity_S_define_instance_name = 'active_physical_entity.entity_packing' )

# Set the debug flag for the active vehicle.
active_physical_entity.entity_packing.debug = verbose

# Add this vehicle to the list of managed object.
federate.add_fed_object( lander )

# FIXME: For now, let's add the data tags.  Later this will come from the DynBody.
active_physical_entity.entity_packing.set_type( 'NASA Lander' )
active_physical_entity.entity_packing.set_status( 'Approach' )
active_physical_entity.entity_packing.set_parent_frame( 'MoonCentricInertial' )


#---------------------------------------------------------------------------
# Set up the lander PhysicalInterface object.
#---------------------------------------------------------------------------
lander_dockport = SpaceFOMPhysicalInterfaceObject(
   create_interface_object          = True,
   interface_instance_name          = active_interface_name,
   interface_S_define_instance      = active_physical_interface.interface_packing,
   interface_S_define_instance_name = 'active_physical_interface.interface_packing' )

# Set the JEOD vehicle point ID associated with this interface.
active_physical_interface.interface_packing.set_vehicle_point_id('Active docking port')

# Set the debug flag for the active vehicle.
active_physical_interface.interface_packing.debug = verbose

# Add this vehicle to the list of managed object.
federate.add_fed_object( lander_dockport )

# FIXME: For now, let's add the data tags.  Later this will come from the vehicle point.
active_physical_interface.interface_packing.set_parent( active_entity_name )


#---------------------------------------------------------------------------
# Set up the station PhysicalEntity object.
#---------------------------------------------------------------------------
station = SpaceFOMPhysicalEntityObject(
   create_entity_object          = True,
   entity_instance_name          = passive_entity_name,
   entity_S_define_instance      = passive_physical_entity.entity_packing,
   entity_S_define_instance_name = 'passive_physical_entity.entity_packing' )

# Set the debug flag for the passive vehicle.
passive_physical_entity.entity_packing.debug = verbose

# Add this vehicle to the list of managed object.
federate.add_fed_object( station )

# FIXME: For now, let's add the data tags.  Later this will come from the DynBody.
passive_physical_entity.entity_packing.set_type( 'Gateway' )
passive_physical_entity.entity_packing.set_status( 'NRHO' )
passive_physical_entity.entity_packing.set_parent_frame( 'MoonCentricInertial' )


#---------------------------------------------------------------------------
# Set up the station PhysicalInterface object.
#---------------------------------------------------------------------------
station_dockport = SpaceFOMPhysicalInterfaceObject(
   create_interface_object          = True,
   interface_instance_name          = passive_interface_name,
   interface_S_define_instance      = passive_physical_interface.interface_packing,
   interface_S_define_instance_name = 'passive_physical_interface.interface_packing' )

# Set the JEOD vehicle point ID associated with this interface.
passive_physical_interface.interface_packing.set_vehicle_point_id('Passive docking port')

# Set the debug flag for the active vehicle.
passive_physical_interface.interface_packing.debug = verbose

# Add this vehicle to the list of managed object.
federate.add_fed_object( station_dockport )

# FIXME: For now, let's add the data tags.  Later this will come from the vehicle point.
passive_physical_interface.interface_packing.set_parent( passive_entity_name )


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
federate.add_sim_object( active_physical_entity )
federate.add_sim_object( active_physical_interface )
federate.add_sim_object( passive_physical_entity )
federate.add_sim_object( passive_physical_interface )


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
