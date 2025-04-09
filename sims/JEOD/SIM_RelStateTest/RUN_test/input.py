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

# Load the SpaceFOM/JEOD specific reference frame tree configuration object.
from Modified_data.JEOD.JEODRefFrameTreeObject import *

# Load the SpaceFOM vehicle PhysicalEntity configuration object.
from Modified_data.SpaceFOM.SpaceFOMPhysicalEntityObject import *

def print_usage_message( ):

   print(' ')
   print('SpaceFOM Reference Frame Simulation Command Line Configuration Options:')
   print('  -c --case [name]     : Set the name for the case definition file.')
   print('  -d                   : Pass the input file debug option on to Trick.')
   print('  -e --express [frame] : Set express frame for the relative state.')
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
   global frames_dict
   global express_frame
   global native_frame
   
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
            if str(argv[index]) in frames_dict.keys():
               express_frame = str(argv[index])
            else:
               print('ERROR: Express frame \'' + str(argv[index]) + '\' not found!')
               print_usage = True
         else:
            print('ERROR: Missing express frame name.')
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
            if str(argv[index]) in frames_dict.keys():
               native_frame = str(argv[index])
            else:
               print('ERROR: Native frame \'' + str(argv[index]) + '\' not found!')
               print_usage = True
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
run_duration = 2.0

# Default is to NOT show verbose messages.
verbose = False

# Default Trick run configuration directory.
config_dir = 'Modified_data'

# Create SpaceFOM to JEOD reference frame dictionary.
frames_dict = {
   'SolarSystemBarycentricInertial': 'SSBary.inertial',
   'SunCentricInertial': 'Sun.inertial',
   'EarthMoonBarycentricInertial': 'EMBary.inertial',
   'EarthCentricInertial': 'Earth.inertial',
   'EarthCentricFixed': 'Earth.pfix',
   'MoonCentricInertial': 'Moon.inertial',
   'MoonCentricFixed': 'Moon.pfix',
   'MarsCentricInertial': 'Mars.inertial',
   'MarsCentricFixed': 'Mars.pfix'
   }

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

parse_command_line()

if (print_usage == True) :
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
# Set up the JEOD environment.
# =========================================================================
jeod_time.time_manager_init.initializer = "UTC"
jeod_time.time_manager_init.sim_start_format = trick.TimeEnum.calendar

jeod_time.time_utc.calendar_year   = 2027
jeod_time.time_utc.calendar_month  =    8
jeod_time.time_utc.calendar_day    =   17
jeod_time.time_utc.calendar_hour   =   12
jeod_time.time_utc.calendar_minute =    0
jeod_time.time_utc.calendar_second =  0.0

jeod_time.time_tai.initialize_from_name = "UTC"
jeod_time.time_tt.initialize_from_name  = "TAI"

jeod_time.time_tai.update_from_name = "Dyn"
jeod_time.time_utc.update_from_name = "TAI"
jeod_time.time_tt.update_from_name  = "TAI"

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
federate = SpaceFOMFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.ExCO,
                                   'SpaceFOM_JEOD_Test',
                                   'JEODRefFrames',
                                   True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
#federate.set_ExCO_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if (verbose == True) : 
   #federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_6_TRACE )
   federate.set_debug_source( trick.TrickHLA.DEBUG_SOURCE_ALL_MODULES )
   #federate.set_debug_source( trick.TrickHLA.DEBUG_SOURCE_OBJECT + trick.TrickHLA.DEBUG_SOURCE_ATTRIBUTE )
else :
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )
   #federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )

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
# Perform some sanity checks.
#---------------------------------------------------------------------------
if not native_frame in frames_dict.keys():
   print('ERROR: Native frame \'' + native_frame + '\' not found!')
   print_usage_message()
   
if not express_frame in frames_dict.keys():
   print('ERROR: Express frame \'' + express_frame + '\' not found!')
   print_usage_message()
   

#---------------------------------------------------------------------------
# Set up the Reference Frame objects.
#---------------------------------------------------------------------------
frame_tree = JEODRefFrameTreeObject(
   federate_instance    = federate,
   tree_instance        = ref_frame_tree,
   create_frame_objects = True,
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
# Set up the Reference Frame Tree
#---------------------------------------------------------------------------
ref_frame_tree.frame_tree.debug = True
trick.exec_set_job_onoff( "ref_frame_tree.frame_tree.print_tree", 1, print_tree )


#---------------------------------------------------------------------------
# Set up the test vehicle PhysicalEntity object.
#---------------------------------------------------------------------------
if os.path.exists( vehicle_file ):
   exec(open(vehicle_file).read())
else:
   print('ERROR: Vehicle file not found: ' + vehicle_file)
   print_usage_message()
   
pe_object = SpaceFOMPhysicalEntityObject(
   create_entity_object          = True,
   entity_instance_name          = vehicle_name,
   entity_S_define_instance      = veh_physical_entity.entity_packing,
   entity_S_define_instance_name = 'veh_physical_entity.entity_packing' )

# Set the debug flag for the active vehicle.
veh_physical_entity.entity_packing.debug = verbose

# Add this vehicle to the list of managed object.
federate.add_fed_object( pe_object )


#---------------------------------------------------------------------------
# Set up the JEOD relative state object.
#---------------------------------------------------------------------------
rel_state_name = vehicle_name + '_RelState'
subject_frame_name = vehicle_name + '.composite_body'
relstate.debug_rel_state = True
relstate.debug_dyn_body  = True
relstate.debug_frames    = True
relstate.rel_state.set_name( rel_state_name )
relstate.rel_state.subject_frame_name = subject_frame_name
relstate.rel_state.target_frame_name  = frames_dict[express_frame]
relstate.rel_state.direction_sense    = trick.RelativeDerivedState.ComputeSubjectStateinTarget


#---------------------------------------------------------------------------
# Set up the SpaceFOM relative state object.
#---------------------------------------------------------------------------
rel_test.debug_rel_state    = True
rel_test.debug_entity_state = False
rel_test.debug_frames       = False
rel_test.rel_state.debug    = False
if express_frame == 'SolarSystemBarycentricInertial':
   rel_test.rel_state_frame  = solar_system_barycenter.frame_packing
elif express_frame == 'SunCentricInertial':
   rel_test.rel_state_frame  = sun_inertial.frame_packing
elif express_frame == 'EarthMoonBarycentricInertial':
   rel_test.rel_state_frame  = earth_moon_barycenter.frame_packing
elif express_frame == 'EarthCentricInertial':
   rel_test.rel_state_frame  = earth_centered_inertial.frame_packing
elif express_frame == 'EarthCentricFixed':
   rel_test.rel_state_frame  = earth_centered_fixed.frame_packing
elif express_frame == 'MoonCentricInertial':
   rel_test.rel_state_frame  = moon_centered_inertial.frame_packing
elif express_frame == 'MoonCentricFixed':
   rel_test.rel_state_frame  = moon_centered_fixed.frame_packing
elif express_frame == 'MarsCentricInertial':
   rel_test.rel_state_frame  = mars_centered_inertial.frame_packing
elif express_frame == 'MarsCentricFixed':
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
federate.add_sim_object( veh_physical_entity )


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
