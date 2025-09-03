##############################################################################
# PURPOSE:
#    (This is a Python input file for configuring the simple HLA time managed
#     federate.)
#
# REFERENCE:
#    (Trick 19 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Uses the SpaceFOMFederateConfig Python class.)
#     (Uses the SpaceFOMRefFrameObject Python class.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (Sept 2025) (--) (Initial implementation.)))
##############################################################################
import socket
import subprocess
import sys
sys.path.append( '../../../' )

# Load the SpaceFOM specific federate configuration object.
from TrickHLA_data.SpaceFOM.SpaceFOMFederateConfig import *

# Load the SpaceFOM specific reference frame configuration object.
from TrickHLA_data.SpaceFOM.SpaceFOMRefFrameObject import *

# Load the sine specific Sine object.
from TrickHLA_data.sine.SineObject import *


def print_usage_message():

   print( ' ' )
   print( 'TrickHLA SpaceFOM HLA Time Managed Simulation Command Line Configuration Options:' )
   print( '  -h --help              : Print this help message.' )
   print( '  -f --fed_name [name]   : Name of the Federate, default is Simple-Time-Fed.' )
   print( '  -fe --fex_name [name]  : Name of the Federation Execution, default is SpaceFOM_sine.' )
   print( '  --nostop               : Set no stop time on simulation.' )
   print( '  -s --stop [time]       : Time to stop simulation, default is 10.0 seconds.' )
   print( '  --verbose [on|off]     : on: Show verbose messages (Default), off: disable messages.' )
   print( ' ' )

   trick.exec_terminate_with_return( -1,
                                     sys._getframe( 0 ).f_code.co_filename,
                                     sys._getframe( 0 ).f_lineno,
                                     'Print usage message.' )
   return


def parse_command_line():

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
   while ( index < argc ):

      if ( ( str( argv[index] ) == '-h' ) | ( str( argv[index] ) == '--help' ) ):
         print_usage = True

      elif ( ( str( argv[index] ) == '-f' ) | ( str( argv[index] ) == '--fed_name' ) ):
         index = index + 1
         if ( index < argc ):
            federate_name = str( argv[index] )
         else:
            print( 'ERROR: Missing --fed_name [name] argument.' )
            print_usage = True

      elif ( ( str( argv[index] ) == '-fe' ) | ( str( argv[index] ) == '--fex_name' ) ):
         index = index + 1
         if ( index < argc ):
            federation_name = str( argv[index] )
         else:
            print( 'ERROR: Missing --fex_name [name] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--nostop' ):
         run_duration = None

      elif ( ( str( argv[index] ) == '-s' ) | ( str( argv[index] ) == '--stop' ) ):
         index = index + 1
         if ( index < argc ):
            run_duration = float( str( argv[index] ) )
         else:
            print( 'ERROR: Missing -stop [time] argument.' )
            print_usage = True

      elif ( str( argv[index] ) == '--verbose' ):
         index = index + 1
         if ( index < argc ):
            if ( str( argv[index] ) == 'on' ):
               verbose = True
            elif ( str( argv[index] ) == 'off' ):
               verbose = False
            else:
               print( 'ERROR: Unknown --verbose argument: ' + str( argv[index] ) )
               print_usage = True
         else:
            print( 'ERROR: Missing --verbose [on|off] argument.' )
            print_usage = True

      elif ( ( str( argv[index] ) == '-d' ) ):
         # Pass this on to Trick.
         break

      else:
         print( 'ERROR: Unknown command line argument ' + str( argv[index] ) )
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
federate_name = 'Simple-Time-Fed'

# Set the default Federation Execution name.
federation_name = 'SpaceFOM_sine'

parse_command_line()

if ( print_usage == True ):
   print_usage_message()


#---------------------------------------------
# Set up Trick executive parameters.
#---------------------------------------------
# instruments.echo_jobs.echo_jobs_on()
trick.exec_set_trap_sigfpe( True )
# trick.checkpoint_pre_init( 1 )
# trick.checkpoint_post_init( 1 )
# trick.add_read( 0.0 , '''trick.checkpoint('chkpnt_point')''' )
# trick.checkpoint_end( 1 )

trick.exec_set_enable_freeze( False )
trick.exec_set_freeze_command( False )
trick.exec_set_stack_trace( False )


# =========================================================================
# Set up the HLA interfaces.
# =========================================================================
# Instantiate the Python SpaceFOM configuration object.
federate = SpaceFOMFederateConfig(
   thla_federate        = THLA.federate,
   thla_manager         = THLA.manager,
   thla_control         = THLA.execution_control,
   thla_config          = THLA.ExCO,
   thla_federation_name = federation_name,
   thla_federate_name   = federate_name,
   thla_enabled         = True )

federate.fix_var_server_source_address()

# Set the name of the ExCO S_define instance.
# We do not need to do this since we're using the ExCO default_data job
# to configure the ExCO. This is only needed for input file configuration.
# federate.set_config_S_define_name( 'THLA_INIT.ExCO' )

# Set the debug output level.
if ( verbose == True ):
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_4_TRACE )
else:
   federate.set_debug_level( trick.TrickHLA.DEBUG_LEVEL_0_TRACE )

#--------------------------------------------------------------------------
# Configure this federate SpaceFOM roles for this federate.
#--------------------------------------------------------------------------
federate.set_master_role( False )  # This is Not the Master federate.
federate.set_pacing_role( False )  # This is Not the Pacing federate.
federate.set_RRFP_role( False )    # This is Not the Root Reference Frame Publisher.

#--------------------------------------------------------------------------
# Add in known required federates.
#--------------------------------------------------------------------------
federate.add_known_federate( True, str( federate.federate.name ) )

#--------------------------------------------------------------------------
# Configure the CRC.
#--------------------------------------------------------------------------
# Pitch specific local settings designator:
THLA.federate.local_settings = 'crcHost = localhost\n crcPort = 8989'

#--------------------------------------------------------------------------
# Set up federate time related parameters.
#--------------------------------------------------------------------------
# Specify the HLA base time units (default: trick.HLA_BASE_TIME_MICROSECONDS).
federate.set_HLA_base_time_units( trick.HLA_BASE_TIME_MICROSECONDS )

# Scale the Trick Time Tic value based on the HLA base time units.
federate.scale_trick_tics_to_base_time_units()

# Must specify a federate HLA lookahead value in seconds.
federate.set_lookahead_time( 0.250 )

# Must specify the Least Common Time Step for all federates in the
# federation execution.
federate.set_least_common_time_step( 0.250 )

# Must specify a Trick software frame that meets the time constraints
# for the Least Common Time Step (LCTS) value set in the ExCO by the
# Master federate. (LCTS >= RT) && (LCTS % RT = 0)
trick.exec_set_software_frame( 0.250 )

# Setup Time Management parameters.
federate.set_time_regulating( True )
federate.set_time_constrained( True )

#---------------------------------------------------------------------------
# Add the HLA SimObjects associated with this federate.
# This is really only useful for turning on and off HLA objects.
# This doesn't really apply to these example simulations which are only HLA.
#---------------------------------------------------------------------------
federate.add_sim_object( THLA )
federate.add_sim_object( THLA_INIT )

#---------------------------------------------------------------------------
# Make sure that the Python federate configuration object is initialized.
#---------------------------------------------------------------------------
# federate.disable()
federate.initialize()

#---------------------------------------------------------------------------
# Set up simulation termination time.
#---------------------------------------------------------------------------
if run_duration:
   trick.sim_services.exec_set_terminate_time( run_duration )
