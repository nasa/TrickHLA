#!/usr/bin/env python3
# @file cp_chkpnt.py
# @brief This program will copy a checkpoint file between run directories.
#
# This is a Python program used to copy a Trick checkpoint file from one run
# directory into another.  It make no assumption on the checkpoint file other
# that the run directory names need to be replaced.  It does check for
# overwrite which can be overridden.
# 
# @copyright Copyright 2026 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Other Rights Reserved.
# 
# \par<b>Responsible Organization</b>
# Simulation and Graphics Branch, Mail Code ER7\n
# Software, Robotics & Simulation Division\n
# NASA, Johnson Space Center\n
# 2101 NASA Parkway, Houston, TX  77058
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, February 2026, --, Initial version.}
# @revs_end

import sys
import os
import shutil
import argparse

# Import the TrickHLA Python environment.
from trickhla_environment import *


# Main routine.
def main():

   # General command line parameters.
   run_dir_in     = ''
   input_path     = ''
   run_dir_out    = ''
   output_path    = ''
   overwrite      = False
   test           = False
   verbose_output = False

   # Setup command line argument parsing.
   parser = argparse.ArgumentParser( prog = 'cp_chkpnt', \
                                     description = 'Copy and transform a Trick checkpoint file.' )
   parser.add_argument( 'input_path' )
   parser.add_argument( 'output_path' )
   parser.add_argument( '-f', '--overwrite', \
                        help = 'Overwrite an existing checkpoint file.', \
                        action = "store_true" )
   parser.add_argument( '-t', '--test', help = 'Do not generate documentation, just show what would be done.', \
                         action = "store_true" )
   parser.add_argument( '-v', '--verbose', \
                        help = 'Generate verbose output.', \
                        action = "store_true" )

   # Parse the command line arguments.
   args = parser.parse_args()

   # Assign command line parameters.
   overwrite = args.overwrite
   test      = args.test

   # Determine the verbose status. Verbose is on for test.
   if test:
      verbose_output = True
   else:
      verbose_output = args.verbose

   #
   # Parse the input checkpoint file to split the input run directory from the
   #
   input_path = args.input_path
   # input checkpoint file.
   input_checkpoint = input_path.split( os.sep )
   if len(input_checkpoint) == 2:
      run_dir_in = input_checkpoint[0]
      chkpnt_in  = input_checkpoint[1]
   else:
      parser.print_help()
      TrickHLAMessage.failure( 'Parse error with input checkpoint path: ' + input_path )

   if verbose_output:
      TrickHLAMessage.status( 'Input RUN directory:   ' + run_dir_in )
      TrickHLAMessage.status( 'Input checkpoint file: ' + chkpnt_in )

   # Check for the existence of the input checkpoint file.
   if os.path.isfile( input_path ):
      if verbose_output:
         TrickHLAMessage.success( 'Input checkpoint file found: ' + input_path )
   else:
      TrickHLAMessage.failure( 'Input checkpoint file NOT found: ' + input_path )

   #
   # Parse the output checkpoint file to split the output run directory from the
   # output checkpoint file.  If only a directory is provided, use the input
   # checkpoint file name.
   #
   output_path = args.output_path
   output_checkpoint = output_path.split( os.sep )
   if len( output_checkpoint ) == 2:
      run_dir_out = output_checkpoint[0]
      chkpnt_out  = output_checkpoint[1]
   elif len( output_checkpoint ) == 1:
      run_dir_out = output_checkpoint[0]
      chkpnt_out  = chkpnt_in
   else:
      parser.print_help()
      TrickHLAMessage.failure( 'Parse error with output checkpoint path: ' + output_path )

   if verbose_output:
      TrickHLAMessage.status( 'Output RUN directory:   ' + run_dir_out )
      TrickHLAMessage.status( 'Output checkpoint file: ' + chkpnt_out )

   # Check for the existence of the output checkpoint directory.
   if os.path.isdir( run_dir_out ):
      if verbose_output:
         TrickHLAMessage.success( 'Output run directory found: ' + run_dir_out )
   else:
      TrickHLAMessage.failure( 'Output run directory NOT found: ' + run_dir_out )

   # Build the checkpoint output file path.
   chkpnt_out_path = run_dir_out + os.sep + chkpnt_out

   # Check for the existence of the output checkpoint file.
   if os.path.isfile( chkpnt_out_path ):
      if overwrite:
         if verbose_output:
            TrickHLAMessage.warning( 'Overwriting output checkpoint file: ' + chkpnt_out_path )
      else:
         TrickHLAMessage.failure( 'Output checkpoint file exists: ' + chkpnt_out_path )
   else:
      if verbose_output:
         TrickHLAMessage.success( 'Generating output checkpoint file: ' + chkpnt_out_path )

   # If test is selected, then just return.
   if test:
      TrickHLAMessage.success( 'Done with test!' )
      return


   #
   # Everything should be cleared to transform the checkpoint file.
   #
   # First open the output file.
   try:
      output_file = open( chkpnt_out_path, 'w' )
   except IOError as e:
      TrickHLAMessage.failure( f"Could not open file: {e}" )
   finally:
      if verbose_output:
         TrickHLAMessage.warning( 'Opening output checkpoint file: ' + chkpnt_out_path )

   # Now open and loop through the input file.
   try:

      # Open the input checkpoint file.
      with open( input_path, 'r' ) as file:

         # Loop through the input file line by line.
         for line in file:

            # Replace the input directory name with the output directory name.
            new_line = line.replace( run_dir_in, run_dir_out )

            # Write the transformed line to the output file.
            output_file.write( new_line )

      # Close the output file once the input file is processed.
      output_file.close()

   except IOError as e:

      # Close the output file once the input file is processed.
      output_file.close()

      TrickHLAMessage.failure( f"Could not open file: {e}" )

   finally:

      # Close the output file once the input file is processed.
      output_file.close()

      if verbose_output:
         TrickHLAMessage.warning( 'Processed input checkpoint file: ' + input_path )

   # End of main routine.
   return

#
# Call the main function.
#
main()
