#!/usr/bin/python
## @file check_code.py
# @brief This program applies a flawfinder program to TrickHLA source code.
# 
# This is a Python program used to check the TrickHLA source code using the
# flawfinder utility. https://dwheeler.com/flawfinder/
#
# @revs_title
# @revs_begin
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, June 2020, --, Initial implementation, based on check_code.py.}
# @revs_end
#
import sys
import time
import os
import shutil
import subprocess
import argparse
import textwrap

from trickhla_message import *
from trickhla_environment import *
from __builtin__ import True

## Main routine.
def main() :

   # Set defaults for TrickHLA and TRICK_HOME.
   trickhla_home = '.'

   # Initialize the lists that go into the flawfinder command argument list.
   trickhla_source_dirs = []
   flawfinder_args      = []

   #
   # Setup command line argument parsing.
   #
   parser = argparse.ArgumentParser( prog='find_code_flaws',\
                                     formatter_class=argparse.RawDescriptionHelpFormatter,\
                                     description='Check the TrickHLA source code using flawfinder.' )

   parser.add_argument( '-m0',\
                        help='Maximum flaw risk checking. Check source with minimum risk level 0 for inclusion in hitlist.',\
                        action='store_true', dest='m0' )
   parser.add_argument( '-m1',\
                        help='Check source with minimum risk level 1 for inclusion in hitlist.',\
                        action='store_true', dest='m1' )
   parser.add_argument( '-m2',\
                        help='Check source with minimum risk level 2 for inclusion in hitlist.',\
                        action='store_true', dest='m2' )
   parser.add_argument( '-m3',\
                        help='Check source with minimum risk level 3 for inclusion in hitlist.',\
                        action='store_true', dest='m3' )
   parser.add_argument( '-m4',\
                        help='Check source with minimum risk level 4 for inclusion in hitlist.',\
                        action='store_true', dest='m4' )
   parser.add_argument( '-m5',\
                        help='Minimal flaw risk checking. Check source with minimum risk level 5 for inclusion in hitlist.',\
                        action='store_true', dest='m5' )

   # Parse the command line arguments.
   args = parser.parse_args()

   #
   # Let's do some command line option sanity checks.
   #
   arg_error = False

   # User must specify one of -m0, -m1, -m2, -m3, -m4, -m5.
   required_arg_cnt = 0
   if args.m0 :
      # -m0
      required_arg_cnt += 1
   if args.m1 :
      # -m1
      required_arg_cnt += 1
   if args.m2 :
      # -m2
      required_arg_cnt += 1
   if args.m3 :
      # -m3
      required_arg_cnt += 1
   if args.m4 :
      # -m4
      required_arg_cnt += 1
   if args.m5 :
      # -m5
      required_arg_cnt += 1
   if ( required_arg_cnt == 0 ) :
      arg_error = True
      TrickHLAMessage.warning( 'You must specify one of \'-m0\', \'-m1\', \'-m2\', \'-m4\', \'-m4\' or \'-m5\'!' )
   elif ( required_arg_cnt >= 2 ) :
      arg_error = True
      TrickHLAMessage.warning( 'Only specify one of \'-m0\', \'-m1\', \'-m2\', \'-m4\', \'-m4\' or \'-m5\'!' )

   if arg_error :
      TrickHLAMessage.failure( 'Error detected in parsing command arguments!' )


   #
   # Now let's check for paths to commands and directories.
   #
   # Check for TrickHLA.
   trickhla_home = os.environ.get( 'TRICKHLA_HOME' )
   if trickhla_home == None :
      trickhla_home = os.environ.get( 'TRICK_HLA_HOME' )
      if trickhla_home == None :
         TrickHLAMessage.failure( 'TRICKHLA_HOME not set!' )

   # Move into the TrickHLA home directory.
   # But first make sure that the directory actually exists before moving there.
   if os.path.isdir( trickhla_home ) :
      os.chdir( trickhla_home )
   else :
      TrickHLAMessage.failure( 'TrickHLA Home directory not found!' )

   # Now let's check to make sure that the source and models directories exist.
   if not os.path.isdir( 'source' ) :
      TrickHLAMessage.failure( 'Could not find the \'source\' directory!' )
   if not os.path.isdir( 'models' ) :
      TrickHLAMessage.failure( 'Could not find the \'models\' directory!' )

   # Find the flawfinder command and get the flawfinder version number.
   flawfinder_cmd, flawfinder_ver = find_flawfinder( None, False )
   if flawfinder_cmd == None :
      TrickHLAMessage.failure( 'No flawfinder command found!' )

   # Form relative paths to all the source directories used by TrickHLA.
   trickhla_source_dirs.extend ( ['./source'] )
   trickhla_source_dirs.extend ( ['./models'] )

   # Set the flawfinder arguments based on the type of code checking the user wants to do.
   if args.m0 :
      flawfinder_args.append( '--minlevel=0' )
   elif args.m1 :
      flawfinder_args.append( '--minlevel=1' )
   elif args.m2 :
      flawfinder_args.append( '--minlevel=2' )
   elif args.m3 :
      flawfinder_args.append( '--minlevel=3' )
   elif args.m4 :
      flawfinder_args.append( '--minlevel=4' )
   elif args.m5 :
      flawfinder_args.append( '--minlevel=5' )
   else :
      flawfinder_args.append( '--minlevel=1' )

   # Add remaining args.
   flawfinder_args.append( '--allowlink' )
   flawfinder_args.append( '--context' )

   # Add the source code paths.
   flawfinder_args.extend( trickhla_source_dirs )

   # Form the flawfinder command with command-line arguments.
   shell_command = [ flawfinder_cmd ]
   shell_command.extend( flawfinder_args )

   # Show a summary of the settings.
   TrickHLAMessage.status( '---------------------------------------------------------------------' )
   TrickHLAMessage.status( 'trickhla_home        = ' + trickhla_home )
   TrickHLAMessage.status( 'trickhla_source_dirs = ' + ' '.join(trickhla_source_dirs) )
   TrickHLAMessage.status( 'flawfinder_cmd       = ' + flawfinder_cmd )
   TrickHLAMessage.status( 'flawfinder_ver       = ' + flawfinder_ver )
   TrickHLAMessage.status( 'flawfinder_args      = ' + ' '.join(flawfinder_args) )
   TrickHLAMessage.status( '---------------------------------------------------------------------' )

   # Execute the flawfinder command.
   try :

      # Spawn off the flawfinder process using Popen.
      flawfinder_proc = subprocess.Popen( shell_command )

      while flawfinder_proc.poll() == None :
         # Check process activity 10 times a second.
         time.sleep(0.1)

   except subprocess.CalledProcessError :
      TrickHLAMessage.error( subprocess.error )
      TrickHLAMessage.failure( '\'flawfinder\' command failed!' )

   # Let the user know that we are done.
   TrickHLAMessage.success( 'Finished checking TrickHLA source code.' )

   return


## Function to find the flawfinder command.
#
# This function searches common locations for the flawfinder command.
#
# @return flawfinder_command The path to the flawfinder command.
# @return flawfinder_version The version of flawfinder.
# @param  flawfinder_bin     The path to the flawfinder programs directory.
# @param  verbose            Flag to set if verbose outputs are on.
#
def find_flawfinder( flawfinder_bin, verbose = True ) :

   # Initialize the flawfinder command path.
   flawfinder_command = None
   flawfinder_version = None

   # Check to see if the path to the flawfinder binaries are set.
   if flawfinder_bin :

      # Use the command line path to set the command.
      flawfinder_command = flawfinder_bin + '/flawfinder'

   # flawfinder_bin is not set so lets check other options.
   else :

      # Check to see if the $FLAWFINDER_HOME environment variable is defined and set.
      flawfinder_home = os.environ.get( 'FLAWFINDER_HOME' )
      if flawfinder_home :

         # Check to see if FLAWFINDER_HOME exists.
         if os.path.isdir( flawfinder_home ) :

            if verbose :
               # Let the use know that we have FLAWFINDER_HOME and where it is.
               TrickHLAMessage.status( 'FLAWFINDER_HOME: ' + flawfinder_home )

               # Form the flawfinder command based on FLAWFINDER_HOME.
               flawfinder_command = flawfinder_home + '/flawfinder'

         else :
            TrickHLAMessage.failure( 'FLAWFINDER_HOME not found: ' + flawfinder_home )

      else :

         # FLAWFINDER_HOME is not set so look in the standard locations for flawfinder.
         if os.path.isfile('/usr/bin/flawfinder') :
            flawfinder_command = '/usr/bin/flawfinder'
         elif os.path.isfile('/usr/local/bin/flawfinder') :
            flawfinder_command = '/usr/local/bin/flawfinder'

   # We're finished hunting. Now let's check for the flawfinder command.
   if flawfinder_command == None :
      TrickHLAMessage.failure( 'Could not find the flawfinder command!' )
   else :
      if not os.path.isfile( flawfinder_command ) :
         TrickHLAMessage.failure( 'Could not find the flawfinder command!: '\
                                  + flawfinder_command )
      else :
         if verbose :
            TrickHLAMessage.status( 'Using flawfinder command: ' + flawfinder_command )

   #
   # Now get the flawfinder version ID tag.
   #
   try :
      flawfinder_version = str(subprocess.check_output( [flawfinder_command, '--version'] )).strip()
   except subprocess.CalledProcessError :
      TrickHLAMessage.error( subprocess.error )
      TrickHLAMessage.failure( '\'flawfinder --version\' command failed!: ' 
                               + flawfinder_command )

   return flawfinder_command, flawfinder_version


#
# Call the main function.
#
main()
