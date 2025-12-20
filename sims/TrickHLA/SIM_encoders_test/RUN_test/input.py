##############################################################################
# PURPOSE:
#    (Python input file for configuring the Encoders test simulation.)
#
# REFERENCE:
#    (Trick 19 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((None))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (Dec 2025) (--) (Initial implementation.)))
##############################################################################
import sys
sys.path.append( '../../../' )


def print_usage_message():

   print( ' ' )
   print( 'TrickHLA Encoders Test Simulation Command Line Configuration Options:' )
   print( '  -h --help              : Print this help message.' )
   print( '  --verbose [on|off]     : on: Show verbose messages (Default), off: disable messages.' )
   print( ' ' )

   trick.exec_terminate_with_return( -1,
                                     sys._getframe( 0 ).f_code.co_filename,
                                     sys._getframe( 0 ).f_lineno,
                                     'Print usage message.' )
   return


def parse_command_line():

   global print_usage
   global verbose

   # Get the Trick command line arguments.
   argc = trick.command_line_args_get_argc()
   argv = trick.command_line_args_get_argv()

   # Process the command line arguments.
   # argv[0]=S_main*.exe, argv[1]=RUN/input.py file
   index = 2
   while ( index < argc ):

      if ( ( str( argv[index] ) == '-h' ) | ( str( argv[index] ) == '--help' ) ):
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

# Default is to NOT show verbose messages.
verbose = False

parse_command_line()

if ( print_usage == True ):
   print_usage_message()


# Set the debug output level.
if ( verbose == True ):
   T.encoder_test.set_debug_level( trick.DEBUG_LEVEL_5_TRACE )
else:
   T.encoder_test.set_debug_level( trick.DEBUG_LEVEL_1_TRACE )
