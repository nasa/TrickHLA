#!/usr/bin/python
# @file cyclomatic_complexity.py
# @brief This program uses the lizard python utility to determine cyclomatic complexity.
#
# This is a Python program used to check the TrickHLA source code
# cyclomatic complexity using the lizard utility.
# 1) git clone https://github.com/terryyin/lizard.git
# 2) cd lizard
# 3) ./build.sh
# 4) sudo python setup.py install
#
# @revs_title
# @revs_begin
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, Sept 2020, --, Initial implementation.}
# @revs_end
#
import time
import os
import subprocess
import argparse

from trickhla_message import *
from trickhla_environment import *


# Main routine.
def main():

   # Set defaults for TrickHLA and TRICK_HOME.
   trickhla_home = '.'

   # Initialize the lists that go into the lizard command argument list.
   trickhla_source_dirs = []
   lizard_args = []

   #
   # Setup command line argument parsing.
   #
   parser = argparse.ArgumentParser( prog = '', \
                                     formatter_class = argparse.RawDescriptionHelpFormatter, \
                                     description = 'Determine the cyclomatic complexity of the TrickHLA source code using lizard.\n\
Available here https://github.com/terryyin/lizard' )

   parser.add_argument( '-C', '--CCN', type = int, default = 15, dest = 'CCN',
                        help = 'Threshold for the cyclomatic complexity number warning messages to be shown (default 15).' )
   parser.add_argument( '-L', '--length', type = int, default = 1000, dest = 'LENGTH',
                        help = 'Threshold for the maximum function length warning messages to be shown (default 1000).' )
   parser.add_argument( '-w', '--warnings_only', action = 'store_true', dest = 'warn_only',
                        help = 'Show warnings only.' )

   # Parse the command line arguments.
   args = parser.parse_args()

   if args.warn_only:
      lizard_args.append( '-w' )

   lizard_args.append( '-C' + str( args.CCN ) )
   lizard_args.append( '-L' + str( args.LENGTH ) )

   #
   # Now let's check for paths to commands and directories.
   #
   # Check for TrickHLA.
   trickhla_home = os.environ.get( 'TRICKHLA_HOME' )
   if trickhla_home is None:
      trickhla_home = os.environ.get( 'TRICK_HLA_HOME' )
      if trickhla_home is None:
         TrickHLAMessage.failure( 'TRICKHLA_HOME not set!' )

   # Move into the TrickHLA home directory.
   # But first make sure that the directory actually exists before moving there.
   if os.path.isdir( trickhla_home ):
      os.chdir( trickhla_home )
   else:
      TrickHLAMessage.failure( 'TrickHLA Home directory not found!' )

   # Now let's check to make sure that the source and models directories exist.
   if not os.path.isdir( 'include' ):
      TrickHLAMessage.failure( 'Could not find the \'include\' directory!' )
   if not os.path.isdir( 'source' ):
      TrickHLAMessage.failure( 'Could not find the \'source\' directory!' )
   if not os.path.isdir( 'models' ):
      TrickHLAMessage.failure( 'Could not find the \'models\' directory!' )

   # Find the lizard command and get the lizard version number.
   lizard_cmd, lizard_ver = find_lizard( None, False )
   if lizard_cmd is None:
      TrickHLAMessage.failure( 'No lizard command found! Try running: git clone https://github.com/terryyin/lizard.git && cd lizard && ./build.sh && sudo python setup.py install' )

   # Form relative paths to all the source directories used by TrickHLA.
   trickhla_source_dirs.extend ( ['./include/'] )
   trickhla_source_dirs.extend ( ['./models/'] )
   trickhla_source_dirs.extend ( ['./source/'] )

   # Add the source code paths.
   lizard_args.extend( trickhla_source_dirs )

   # Form the lizard command with command-line arguments.
   shell_command = [ lizard_cmd ]
   shell_command.extend( lizard_args )

   # Show a summary of the settings.
   TrickHLAMessage.status( '---------------------------------------------------------------------' )
   TrickHLAMessage.status( 'trickhla_home        = ' + trickhla_home )
   TrickHLAMessage.status( 'trickhla_source_dirs = ' + ' '.join( trickhla_source_dirs ) )
   TrickHLAMessage.status( 'lizard_cmd           = ' + lizard_cmd )
   TrickHLAMessage.status( 'lizard_ver           = ' + lizard_ver )
   TrickHLAMessage.status( 'lizard_args          = ' + ' '.join( lizard_args ) )
   TrickHLAMessage.status( '---------------------------------------------------------------------' )

   # Execute the lizard command.
   try:

      # Spawn off the lizard process using Popen.
      lizard_proc = subprocess.Popen( shell_command )

      while lizard_proc.poll() is None:
         # Check process activity 10 times a second.
         time.sleep( 0.1 )

   except subprocess.CalledProcessError:
      TrickHLAMessage.error( subprocess.CalledProcessError.message )
      TrickHLAMessage.failure( '\'lizard\' command failed!' )

   # Let the user know that we are done.
   TrickHLAMessage.success( 'Finished processing TrickHLA source code.' )

   return


# Function to find the lizard command.
#
# This function searches common locations for the lizard command.
#
# @return lizard_command The path to the lizard command.
# @return lizard_version The version of lizard.
# @param  lizard_bin     The path to the lizard programs directory.
# @param  verbose        Flag to set if verbose outputs are on.
#
def find_lizard( lizard_bin, verbose = True ):

   # Initialize the lizard command path.
   lizard_command = None
   lizard_version = None

   # Check to see if the path to the lizard binaries are set.
   if lizard_bin:

      # Use the command line path to set the command.
      lizard_command = lizard_bin + '/lizard'

   # lizard_bin is not set so lets check other options.
   else:

      # Check to see if the $LIZARD_HOME environment variable is defined and set.
      lizard_home = os.environ.get( 'LIZARD_HOME' )
      if lizard_home:

         # Check to see if LIZARD_HOME exists.
         if os.path.isdir( lizard_home ):

            # Form the lizard command based on LIZARD_HOME.
            lizard_command = lizard_home + '/lizard'

            if verbose:
               # Let the use know that we have LIZARD_HOME and where it is.
               TrickHLAMessage.status( 'LIZARD_HOME: ' + lizard_home )

         else:
            TrickHLAMessage.failure( 'LIZARD_HOME not found: ' + lizard_home )

      else:

         # LIZARD_HOME is not set so look in the standard locations for lizard.
         if os.path.isfile( '/usr/bin/lizard' ):
            lizard_command = '/usr/bin/lizard'
         elif os.path.isfile( '/usr/local/bin/lizard' ):
            lizard_command = '/usr/local/bin/lizard'

   # We're finished hunting. Now let's check for the lizard command.
   if lizard_command is None:
      TrickHLAMessage.failure( 'Could not find the lizard command!' )
   else:
      if not os.path.isfile( lizard_command ):
         TrickHLAMessage.failure( 'Could not find the lizard command!: '\
                                  +lizard_command )
      else:
         if verbose:
            TrickHLAMessage.status( 'Using lizard command: ' + lizard_command )

   #
   # Now get the lizard version ID tag.
   #
   try:
      lizard_version = str( subprocess.check_output( [lizard_command, '--version'],
                                                     stderr = subprocess.STDOUT ) ).strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.error( subprocess.CalledProcessError.message )
      TrickHLAMessage.failure( '\'lizard --version\' command failed!: '
                               +lizard_command )

   return lizard_command, lizard_version


#
# Call the main function.
#
main()
