##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring a SpaceFOM wheelbot federate which.)
#    (This wheelbot updates its position by subscribing to the position data from the RTI.)
#    (In this configuration, the wheelbot is replaced by a mouse for cuteness and humor.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.)
#     (Uses the TrickHLAObjectConfig Python class.)
#     (Uses the TrickHLAAttributeConfig Python class.)
#     (Instantiated the default TrickHLATimeOfDayTimeline based CTE time
#      line for testing.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
#    (((Andrew W. Young) (NASA/ER7) (July 2023) (--) (TrickHLA familiarization project.)))
##############################################################################
import sys
sys.path.append('../../../')
# Load the SpaceFOM specific federate configuration object.
from Modified_data.SpaceFOM.SpaceFOMFederateConfig import *
# Load the SpaceFOM specific reference frame configuration object.
from Modified_data.SpaceFOM.SpaceFOMRefFrameObject import *
# Load TrickHLAObjectConfig and TrickHLAAttributeConfig
from Modified_data.TrickHLA.TrickHLAObjectConfig import *
from Modified_data.TrickHLA.TrickHLAAttributeConfig import *

exec(open("Modified_data/realtime.py").read())
exec(open("Modified_data/vehicleState.dr").read())

trick.TMM_reduced_checkpoint(0)

veh_integloop.getIntegrator(trick.Runge_Kutta_4, 8)

def print_usage_message( ):

   print(' ')
   print('TrickHLA SpaceFOM Master Simulation Command Line Configuration Options:')
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
         
      else :
         print('ERROR: Unknown command line argument ' + str(argv[index]))
         print_usage = True
         
      index = index + 1
   return

# Default: Don't show usage.
print_usage = False

# Set the default run duration.
run_duration = 60.0

# Default is to show verbose messages.
verbose = True

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

trick.exec_set_enable_freeze(True)
trick.exec_set_freeze_command(True)
trick.sim_control_panel_set_enabled(True)
trick.exec_set_stack_trace(True)


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig( THLA.federate,
                                   THLA.manager,
                                   THLA.execution_control,
                                   THLA.ExCO,
                                   'Wheelbot_Test',
                                   'Wheelbot-2',
                                   True )

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
#federate.set_ExCO_S_define_name( 'THLA_INIT.ExCO' )


# Set the debug output level.
if (verbose == True) : 
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_11_TRACE )
else :
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False )  # This is NOT the Master federate.
federate.set_pacing_role( False ) # This is NOT the Pacing federate.
federate.set_RRFP_role( False )   # This is NOT the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_fededrate( True, str(federate.federate.name) )
federate.add_known_fededrate( True, 'MPR' )
federate.add_known_fededrate( True, 'Wheelbot-1' )

#==========================================================================
# Configure the CRC.
#==========================================================================
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = js-er7-rti.jsc.nasa.gov\n crcPort = 8989'
#THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'
#THLA.federate.local_settings = 'crcHost = 10.8.0.161\n crcPort = 8989'
# Make specific local settings designator, which is anything from the rid.mtl file:
#THLA.federate.local_settings = '(setqb RTI_tcpForwarderAddr \'192.168.15.3\') (setqb RTI_distributedForwarderPort 5000)'
THLA.federate.lookahead_time = 0.25 # this is THLA_DATA_CYCLE_TIME



#==========================================
#Wheelbot Object Config
#==========================================

# Set initial position for subscribing wheelbot
veh.vehicle.position[0] = -0.8
veh.vehicle.position[1] = 0.0

# Subscribe to TrickHLA Object 'Wheelbot_hla_entity' attribute 'state.'
obj = TrickHLAObjectConfig(False,'Wheelbot_hla_entity','PhysicalEntity',None,None,None,None,False)

att0 = TrickHLAAttributeConfig('state','veh.vehicle.stcs',False,True,False,trick.CONFIG_CYCLIC,trick.ENCODING_LITTLE_ENDIAN)

obj.add_attribute(att0)

federate.add_fed_object(obj)



#==========================================
# Add the waypoints to the SIM. For the subscriber, no
# waypoints are necessary, just set veh.vehicle.subscriber to true.
# Set a home point by adding it as the last waypoint.
#==========================================
waypoints_path = "Modified_data/cross.snackpoints"
veh.vehicle.subscriber = True

#==========================================
# Start the display VarServer Client - this display will show both
# publishing and subscribing vehicles.
#==========================================
varServerPort = trick.var_server_get_port();
EVDisplay_path = "models/Graphics/dist/EVDisplay.jar"

if (os.path.isfile(EVDisplay_path)) :
    EVDisplay_cmd = "java -jar " \
                  + EVDisplay_path \
                  + " -v images/mouse_128x128.png" \
                  + " -w " + waypoints_path \
                  + " " + str(varServerPort) + " &" ;
    print(EVDisplay_cmd)
    os.system( EVDisplay_cmd);
else :
    print('==================================================================================')
    print('EVDisplay needs to be built. Please \"cd\" into models/Graphics and type \"make\".')
    print('==================================================================================')

#==========================================
# Start the display VarServer Client - this is the gui to command the vehicle to go home.
#==========================================
varServerPort = trick.var_server_get_port();
HomeDisplay_path = "models/GUIControl1/dist/HomeDisplay.jar"

if (os.path.isfile(HomeDisplay_path)) :
    HomeDisplay_cmd = "java -jar " \
                  + HomeDisplay_path \
                  + " " + str(varServerPort) + " &" ;
    print(HomeDisplay_cmd)
    os.system( HomeDisplay_cmd);
else :
    print('==================================================================================')
    print('HomeDisplay needs to be built. Please \"cd\" into models/GUIControl1 and type \"make\".')
    print('==================================================================================')

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )
federate.add_sim_object( root_ref_frame )
#federate.add_sim_object( ref_frame_A )


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