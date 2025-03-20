##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the Space Reference FOM
#     example early joiner federate run.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.)
#     (Uses the SpaceFOMPhysicalEntityObject Python class.)
#     (Uses the SpaceFOMDynamicalEntityObject Python class.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (July 2023) (--) (SpaceFOM support and testing.)))
##############################################################################
import sys
sys.path.append('../../../')

# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *

# Load the SpaceFOM specific entity configuration objects.
from Modified_data.SpaceFOM.SpaceFOMPhysicalEntityObject import *
from Modified_data.SpaceFOM.SpaceFOMPhysicalInterfaceObject import *
from Modified_data.SpaceFOM.SpaceFOMDynamicalEntityObject import *

def print_usage_message( ):

   print(' ')
   print('TrickHLA SpaceFOM Other Simulation Command Line Configuration Options:')
   print('  -h --help             : Print this help message.')
   print('  -f --fed_name [name]  : Name of the Federate, default is PhysicalEntity.')
   print('  -fe --fex_name [name] : Name of the Federation Execution, default is SpaceFOM_Roles_Test.')
   print('  -de [name]            : Name of the DynamicalEntity, default is Voyager.')
   print('  --nostop              : Set no stop time on simulation.')
   print('  -pe [name]            : Name of the PhysicalEntity, default is Enterprise.')
   print('  -pi [name]            : Name of the PhysicalInterface, default is Enterprise.dockingport.')
   print('  --phy_fed [name]      : Name of the PhysicalEntity federate, default is PhysicalEntity.')
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
   global federate_name
   global federation_name
   global dyn_entity_name
   global phy_entity_name
   global phy_interface_name
   global phy_interface_name
   global phy_federate_name
   
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
            print('ERROR: Missing -stop [time] argument.')
            print_usage = True
            
      elif (str(argv[index]) == '--nostop'):
         run_duration = None
         
      elif ((str(argv[index]) == '-h') | (str(argv[index]) == '--help')):
         print_usage = True
      
      elif ((str(argv[index]) == '-de')):
         index = index + 1
         if (index < argc):
            dyn_entity_name = str(argv[index])
         else:
            print('ERROR: Missing -de [name] argument.')
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
      
      elif ((str(argv[index]) == '-pe')):
         index = index + 1
         if (index < argc):
            phy_entity_name = str(argv[index])
         else:
            print('ERROR: Missing -pe [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '-pi')):
         index = index + 1
         if (index < argc):
            phy_interface_name = str(argv[index])
         else:
            print('ERROR: Missing -pi [name] argument.')
            print_usage = True
      
      elif ((str(argv[index]) == '--phy_fed')):
         index = index + 1
         if (index < argc):
            phy_federate_name = str(argv[index])
         else:
            print('ERROR: Missing --phy_fed [name] argument.')
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
federate_name = 'DynamicalEntity'

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_Roles_Test'

# Set the default DynamicalEntity instance name.
dyn_entity_name = 'Voyager'

# Set the default PhysicalEntity instance name.
phy_entity_name = 'Enterprise'

# Set the default PhysicalInterface instance name.
phy_interface_name = phy_entity_name + '.dockingport'

# Set the default PhysicalEntity federate name.
phy_federate_name = 'PhysicalEntity'


parse_command_line()

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

trick.exec_set_enable_freeze(False)
trick.exec_set_freeze_command(False)
trick.sim_control_panel_set_enabled(False)
trick.exec_set_stack_trace(False)

#---------------------------------------------
# Setup the integrators
#---------------------------------------------
pe_integloop.getIntegrator( trick.Euler, 13 )
#pe_integloop.getIntegrator( trick.Runge_Kutta_4, 13 )
de_integloop.getIntegrator( trick.Euler, 13 )
#de_integloop.getIntegrator( trick.Runge_Kutta_4, 13 )

#---------------------------------------------------------------------------
# Set up the dynamics parameters for the PhysicalEntity test entity.
#---------------------------------------------------------------------------
pe_dynamics.entity.pe_data.name         = phy_entity_name
pe_dynamics.entity.pe_data.type         = 'Constellation-class Starship'
pe_dynamics.entity.pe_data.status       = 'Mothballed'
pe_dynamics.entity.pe_data.parent_frame = 'RootFrame'

# Initial translational state.
pe_dynamics.entity.pe_data.state.pos = [ 0.0, 0.0, 0.0 ]
pe_dynamics.entity.pe_data.state.vel = [ 0.0, 0.0, 0.0 ]

# Initial rotational state.
pe_dynamics.entity.pe_data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
pe_dynamics.entity.pe_data.state.ang_vel = [ 0.0, 0.0, 0.0 ]

#
# Basic mass properties.
#
pe_mass = 100.0
pe_dynamics.entity.pe_data.cm        = [0.0, 0.0, 0.0]
pe_dynamics.entity.de_data.mass      = pe_mass
pe_dynamics.entity.de_data.mass_rate = 0.0
pe_dynamics.entity.pe_data.body_wrt_struct.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )

# Principal inertia of a solid sphere.
de_radius = 1.0
Ixx = Iyy = Izz = (2.0 / 5.0) * pe_mass * de_radius * de_radius
pe_dynamics.entity.de_data.inertia[0] = [ Ixx, 0.0, 0.0 ]
pe_dynamics.entity.de_data.inertia[1] = [ 0.0, Iyy, 0.0 ]
pe_dynamics.entity.de_data.inertia[2] = [ 0.0, 0.0, Izz ]
pe_dynamics.entity.de_data.inertia_rate[0] = [ 0.0, 0.0, 0.0 ]
pe_dynamics.entity.de_data.inertia_rate[1] = [ 0.0, 0.0, 0.0 ]
pe_dynamics.entity.de_data.inertia_rate[2] = [ 0.0, 0.0, 0.0 ]

# Base propagation parameters.
pe_dynamics.entity.de_data.force  = [ 0.0, 0.0, 0.0 ]
pe_dynamics.entity.de_data.torque = [ 0.0, 0.0, 0.0 ]


#---------------------------------------------------------------------------
# Set up the dynamics parameters for the DynamicalEntity test entity.
#---------------------------------------------------------------------------
de_dynamics.entity.pe_data.name         = dyn_entity_name
de_dynamics.entity.pe_data.type         = 'Intrepid-class Starship'
de_dynamics.entity.pe_data.status       = 'Lost'
de_dynamics.entity.pe_data.parent_frame = 'FrameA'

# Initial translational state.
de_dynamics.entity.pe_data.state.pos = [ 0.0, 0.0, 0.0 ]
de_dynamics.entity.pe_data.state.vel = [ 0.0, 0.0, 0.0 ]

# Initial rotational state.
de_dynamics.entity.pe_data.state.att.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )
de_dynamics.entity.pe_data.state.ang_vel = [ 0.0, 0.0, 0.0 ]

#
# Basic mass properties.
#
de_mass = 100.0
de_dynamics.entity.pe_data.cm        = [0.0, 0.0, 0.0]
de_dynamics.entity.de_data.mass      = de_mass
de_dynamics.entity.de_data.mass_rate = 0.0
de_dynamics.entity.pe_data.body_wrt_struct.set_from_Euler_deg( trick.Roll_Pitch_Yaw, [0.0, 0.0, 0.0] )

# Principal inertia of a solid sphere.
de_radius = 1.0
Ixx = Iyy = Izz = (2.0 / 5.0) * de_mass * de_radius * de_radius
de_dynamics.entity.de_data.inertia[0] = [ Ixx, 0.0, 0.0 ]
de_dynamics.entity.de_data.inertia[1] = [ 0.0, Iyy, 0.0 ]
de_dynamics.entity.de_data.inertia[2] = [ 0.0, 0.0, Izz ]
de_dynamics.entity.de_data.inertia_rate[0] = [ 0.0, 0.0, 0.0 ]
de_dynamics.entity.de_data.inertia_rate[1] = [ 0.0, 0.0, 0.0 ]
de_dynamics.entity.de_data.inertia_rate[2] = [ 0.0, 0.0, 0.0 ]

# Base propagation parameters.
de_dynamics.entity.de_data.force  = [ 0.1, 0.1, 0.1 ]
de_dynamics.entity.de_data.torque = [ 0.01, 0.01, 0.01 ]


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.ExCO,
                                   federation_name,
                                   federate_name,
                                   True )

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

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False ) # This is NOT the Master federate.
federate.set_pacing_role( False ) # This is NOT the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
# This is the PhysicalEntity test federate.
# It doesn't really need to know about any other federates.
federate.add_known_federate( True, str(federate.federate.name) )
federate.add_known_federate( True, 'MPR' )
federate.add_known_federate( True, phy_federate_name )

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

# Must specify a Trick software frame that meets the time constraints
# for the Least Common Time Step (LCTS) value set in the ExCO by the
# Master federate. (LCTS >= RT) && (LCTS % RT = 0)
trick.exec_set_software_frame( 0.250 )

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
ref_frame_A.lag_compensation.debug = verbose
ref_frame_A.lag_compensation.set_integ_tolerance( 1.0e-6 )
ref_frame_A.lag_compensation.set_integ_dt( 0.025 )

#frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_NONE )
frame_A.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )


#---------------------------------------------------------------------------
# Set up the PhysicalEntity object for discovery.
#---------------------------------------------------------------------------
phy_entity = SpaceFOMPhysicalEntityObject(
   create_entity_object          = False,
   entity_instance_name          = phy_entity_name,
   entity_S_define_instance      = physical_entity.entity_packing,
   entity_S_define_instance_name = 'physical_entity.entity_packing',
   entity_conditional            = physical_entity.conditional,
   entity_lag_comp               = physical_entity.lag_compensation,
   entity_ownership              = physical_entity.ownership_handler,
   entity_deleted                = physical_entity.deleted_callback )

# Set the debug flag for the Entity.
physical_entity.entity_packing.debug = verbose

# Add this Entity to the list of managed object.
federate.add_fed_object( phy_entity )

# Set the lag compensation parameters.
physical_entity.lag_compensation.debug = verbose
physical_entity.lag_compensation.set_integ_tolerance( 1.0e-6 )
physical_entity.lag_compensation.set_integ_dt( 0.025 )

#phy_entity.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_NONE )
phy_entity.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )


#---------------------------------------------------------------------------
# Set up the PhysicalInterface object for discovery.
#---------------------------------------------------------------------------
phy_interface = SpaceFOMPhysicalInterfaceObject(
   create_interface_object          = False,
   interface_instance_name          = phy_interface_name,
   interface_S_define_instance      = physical_interface.interface_packing,
   interface_S_define_instance_name = 'physical_interface.interface_packing',
   interface_conditional            = physical_interface.conditional,
   interface_ownership              = physical_interface.ownership_handler,
   interface_deleted                = physical_interface.deleted_callback )

# Set the debug flag for the Entity.
physical_interface.interface_packing.debug = verbose

# Add this Entity to the list of managed object.
federate.add_fed_object( phy_interface )


#---------------------------------------------------------------------------
# Set up the DynamicalEntity object for discovery.
#---------------------------------------------------------------------------
dyn_entity = SpaceFOMDynamicalEntityObject(
   create_entity_object          = True,
   entity_instance_name          = dyn_entity_name,
   entity_S_define_instance      = dynamical_entity.entity_packing,
   entity_S_define_instance_name = 'dynamical_entity.entity_packing',
   entity_conditional            = dynamical_entity.conditional,
   entity_lag_comp               = dynamical_entity.lag_compensation,
   entity_ownership              = dynamical_entity.ownership_handler,
   entity_deleted                = dynamical_entity.deleted_callback )

# Set the debug flag for the Entity.
dynamical_entity.entity_packing.debug = verbose

# Add this Entity to the list of managed object.
federate.add_fed_object( dyn_entity )

# Set the lag compensation parameters.
dynamical_entity.lag_compensation.debug = verbose
dynamical_entity.lag_compensation.set_integ_tolerance( 1.0e-6 )
dynamical_entity.lag_compensation.set_integ_dt( 0.025 )

#phy_entity.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_SEND_SIDE )
dyn_entity.set_lag_comp_type( trick.TrickHLA.LAG_COMPENSATION_RECEIVE_SIDE )


#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
federate.add_sim_object( root_ref_frame )
federate.add_sim_object( ref_frame_A )
federate.add_sim_object( physical_entity )
federate.add_sim_object( dynamical_entity )


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
#trick.add_read(3.0 , """THLA.manager.send_MTR_interaction( trick.SpaceFOM.MTR_GOTO_SHUTDOWN )""")
#trick.add_read(3.0 , """THLA.manager.send_MTR_interaction( trick.SpaceFOM.MTR_GOTO_FREEZE )""")
