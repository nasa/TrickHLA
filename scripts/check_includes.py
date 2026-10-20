#!/usr/bin/env python3
# @file check_includes.py
# @brief Applies include-what-you-use to TrickHLA source code.
#
# This is a Python program used to scan the TrickHLA source code using the
# include-what-you-use utility.
#
# MacOS: brew info include-what-you-use
# See: https://github.com/include-what-you-use/include-what-you-use
#
# @revs_title
# @revs_begin
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, October 2026, --, Initial implementation.}
# @revs_end
#
import argparse
import asyncio
import os
import subprocess
import shutil
import sys
import textwrap
import time

from trickhla_environment import *


# Main routine.
def main():

   # Set defaults for TrickHLA, TRICK_HOME and JEOD_HOME
   trickhla_home = '.'
   trick_home = None
   jeod_home = None
   tsync_home = None

   # Initialize the lists that go into the include-what-you-use command argument list.
   system_includes = []
   trickhla_defines = []
   include_dirs = []
   source_dirs = []
   trickhla_ignore = []
   iwyu_args = []

   #
   # Setup command line argument parsing.
   #
   parser = argparse.ArgumentParser( prog = 'check_includes', \
                                     formatter_class = argparse.RawDescriptionHelpFormatter, \
                                     description = 'Scan the TrickHLA source code using include-what-you-use.', \
                                     epilog = textwrap.dedent( '''\n
Examples:\n  check_includes --TrickHLA --SpaceFOM -v --hla4''' ) )

   parser.add_argument( '-a', '--all', \
                        help = 'Process all the source code.', \
                        action = 'store_true', dest = 'process_all' )
   parser.add_argument( '--TrickHLA', \
                        help = 'Process the core TrickHLA source code.', \
                        action = 'store_true', dest = 'process_TrickHLA' )
   parser.add_argument( '--Encoding', \
                        help = 'Process the TrickHLA/encoding source code.', \
                        action = 'store_true', dest = 'process_TrickHLA_encoding' )
   parser.add_argument( '--Time', \
                        help = 'Process the TrickHLA/time source code.', \
                        action = 'store_true', dest = 'process_TrickHLA_time' )
   parser.add_argument( '--Utils', \
                        help = 'Process the TrickHLA/utils source code.', \
                        action = 'store_true', dest = 'process_TrickHLA_utils' )
   parser.add_argument( '--SpaceFOM', \
                        help = 'Process the SpaceFOM source code.', \
                        action = 'store_true', dest = 'process_SpaceFOM' )
   parser.add_argument( '--IMSim', \
                        help = 'Process the IMSim source code.', \
                        action = 'store_true', dest = 'process_IMSim' )
   parser.add_argument( '--JEOD', \
                        help = 'Process the JEOD source code.', \
                        action = 'store_true', dest = 'process_JEOD' )
   parser.add_argument( '--models', \
                        help = 'Process the models source code.', \
                        action = 'store_true', dest = 'process_models' )
   parser.add_argument( '-b', '--bin', \
                        help = 'Path to include-what-you-use binaries directory.', \
                        dest = 'bin_path' )
   parser.add_argument( '--hla3', \
                        help = 'Check against HLA 3 (default), IEEE 1516-2010', \
                        action = 'store_true', dest = 'hla3' )
   parser.add_argument( '--hla4', \
                        help = 'Check against HLA 4, IEEE 1516-2025', \
                        action = 'store_true', dest = 'hla4' )
   parser.add_argument( '--file-start', \
                        help = 'File count to start scanning from (default: 1).', \
                        dest = 'file_start_cnt' )
   parser.add_argument( '--file-limit', \
                        help = 'Maximum number of files to scan (default: Unlimited).', \
                        dest = 'file_cnt_limit' )
   parser.add_argument( '--jeod-home', \
                        help = 'Provide a path to the JEOD installation directory.', \
                        dest = 'jeod_home' )
   parser.add_argument( '-p', '--path', help = 'Path to TrickHLA directory.', \
                        dest = 'trickhla_home' )
   parser.add_argument( '--rti-home', \
                        help = 'Provide a path to the RTI installation directory.', \
                        dest = 'rti_home' )
   parser.add_argument( '-t', '--test', \
                        help = 'Do not run include-what-you-use just show what would be done.', \
                        action = 'store_true', dest = 'test_only' )
   parser.add_argument( '--trick-home', \
                        help = 'Provide a path to the Trick installation directory.', \
                        dest = 'trick_home' )
   parser.add_argument( '--tsync-home', \
                        help = 'Provide a path to the TSync driver installation directory.', \
                        dest = 'tsync_home' )
   parser.add_argument( '-v', '--verbose', \
                        help = 'Check source code with verbose detailed error information.', \
                        action = 'store_true', dest = 'verbose' )
   parser.add_argument( '-vv', '--very_verbose', \
                        help = 'Run with both include-what-you-use verbose output and script verbose output.', \
                        action = 'store_true', dest = 'very_verbose' )

   # Parse the command line arguments.
   args = parser.parse_args()
   arg_error = False

   # User must specify one of --all, --TrickHLA, --SpaceFOM, --IMSim, --JEOD, or --models
   required_arg_cnt = 0
   if args.process_all:
      # -a, --all
      required_arg_cnt += 1
   if args.process_TrickHLA:
      # --TrickHLA
      required_arg_cnt += 1
   if args.process_TrickHLA_encoding:
      # --Encoding
      required_arg_cnt += 1
   if args.process_TrickHLA_time:
      # --Time
      required_arg_cnt += 1
   if args.process_TrickHLA_utils:
      # --Utils
      required_arg_cnt += 1
   if args.process_SpaceFOM:
      # --SpaceFOM
      required_arg_cnt += 1
   if args.process_IMSim:
      # --IMSim
      required_arg_cnt += 1
   if args.process_JEOD:
      # --JEOD
      required_arg_cnt += 1
   if args.process_models:
      # --models
      required_arg_cnt += 1

   if required_arg_cnt == 0:
      arg_error = True
      TrickHLAMessage.warning( 'You must specify at least one of \'-a\', \'--TrickHLA\', \'--SpaceFOM\', \'--IMSim\', \'--JEOD\', \'--models\'!' )

   # Can only specify one of --hla3 or --hla4.
   if args.hla3 and args.hla4:
      arg_error = True
      TrickHLAMessage.warning( 'Only specify one of \'--hla3\' or \'--hla4\'!' )

   if arg_error:
      TrickHLAMessage.failure( 'Error detected in parsing command arguments!' )

   # If we selected very verbose, then we also need to set verbose in the include-what-you-use output.
   if args.very_verbose:
      args.verbose = True

   # The file count to start scanning from, default is 1.
   file_start_cnt = 1
   if args.file_start_cnt:
      file_start_cnt = int( args.file_start_cnt )

   # Default is unlimited files to scan (<= 0), otherwise a fixed number of files.
   file_cnt_limit = -1
   if args.file_cnt_limit:
      file_cnt_limit = int( args.file_cnt_limit )

   #
   # Now let's check for paths to commands and directories.
   #
   # Check for TrickHLA.
   if args.trickhla_home:
      trickhla_home = args.trickhla_home
   else:
      trickhla_home = os.environ.get( 'TRICKHLA_HOME' )
      if trickhla_home is None:
         trickhla_home = os.environ.get( 'TRICK_HLA_HOME' )
         if trickhla_home is None:
            TrickHLAMessage.failure( 'TRICKHLA_HOME environment variable not set!' )

   # Move into the TrickHLA home directory.
   # But first make sure that the directory actually exists before moving there.
   if os.path.isdir( trickhla_home ):
      if args.very_verbose:
         TrickHLAMessage.status( 'Moving to TRICKHLA_HOME: ' + trickhla_home )
      os.chdir( trickhla_home )
   else:
      TrickHLAMessage.failure( 'TrickHLA Home directory not found!' )

   # Now let's check to make sure that the include, source and script directories exist.
   if not os.path.isdir( 'include' ):
      TrickHLAMessage.failure( 'Could not find the \'include\' directory!' )
   if not os.path.isdir( 'source' ):
      TrickHLAMessage.failure( 'Could not find the \'source\' directory!' )
   if not os.path.isdir( 'scripts' ):
      TrickHLAMessage.failure( 'Could not find the \'scripts\' directory!' )

   # Find the include-what-you-use command and get the cland-tidy version ID.
   iwyu_cmd, iwyu_ver = find_include_what_you_use( args.bin_path, args.very_verbose )
   if iwyu_cmd is None:
      TrickHLAMessage.failure( 'No include-what-you-use command found!' )

   # Determine the path to Trick HOME and the Trick version ID.
   trick_home, trick_ver, trick_ver_year = find_trick( args.trick_home, args.very_verbose )
   if args.very_verbose:
      TrickHLAMessage.status( 'Path to Trick: ' + trick_home )
      TrickHLAMessage.status( 'Trick Version: ' + trick_ver )
      TrickHLAMessage.status( 'Trick Version Year: ' + trick_ver_year )

   # Determine the path to HLA RTI home directory.
   rti_home = find_hla_rti( args.rti_home, args.very_verbose )
   if args.very_verbose:
      TrickHLAMessage.status( 'Path to HLA RTI: ' + rti_home )

   # Determine the path to the HLA RTI include directory.
   if args.hla4:
      rti_include = rti_home + '/api/cpp/HLA_1516-2025'
      if os.path.isdir( rti_include ) is False:
         TrickHLAMessage.failure( 'Could not find the HLA 4 RTI include directory: ' + rti_include )
   else:
      rti_include = rti_home + '/api/cpp/HLA_1516-2010'
      if os.path.isdir( rti_include ) is False:
         # Try the include directory for older Pitch RTI versions.
         rti_include = rti_home + '/include'
         if os.path.isdir( rti_include ) is False:
            TrickHLAMessage.failure( 'Could not find the HLA Evolved RTI include directory: ' + rti_include )
   if args.very_verbose:
      TrickHLAMessage.status( 'Path to HLA RTI include directory: ' + rti_include )

   # Determine the path to JEOD home directory.
   jeod_home = find_jeod( args.jeod_home, args.very_verbose )
   if args.very_verbose and jeod_home:
      TrickHLAMessage.status( 'Path to JEOD: ' + jeod_home )

   # Check for TSync Central Timing Equipment (CTE) driver home directory.
   if args.tsync_home:
      tsync_home = args.tsync_home
   else:
      tsync_home = os.environ.get( 'TSYNC_HOME' )
   if tsync_home and not os.path.isdir( tsync_home ):
      TrickHLAMessage.failure( 'Could not find the TSync Home directory: ' + tsync_home )

   # Define preprocessor symbols we use for TrickHLA and set the TRICK_VER based on the
   # version of the Trick simulation environment we found in our Path.
   if args.hla4:
      trickhla_defines = ['-DTRICK_VER=' + trick_ver_year, '-DIEEE_1516_2025', '-DFPU_CW_PROTECTION' ]
   else:
      trickhla_defines = ['-DTRICK_VER=' + trick_ver_year, '-DIEEE_1516_2010', '-DFPU_CW_PROTECTION' ]

   # Form relative paths to all the include directories used by TrickHLA.
   include_dirs.extend( ['-I./include'] )
   include_dirs.extend( ['-I' + trick_home + '/include'] )
   include_dirs.extend( ['-I' + trick_home + '/include/trick/compat'] )
   include_dirs.extend( ['-I' + trick_home + '/trick_source'] )
   include_dirs.extend( ['-I' + rti_include] )
   if tsync_home:
      include_dirs.extend( ['-I' + tsync_home + '/libtsync/include'] )
      include_dirs.extend( ['-I' + tsync_home + '/tsync-driver/include'] )

   # Add models source code and include paths to process.
   if args.process_all or args.process_models:
      include_dirs.extend( ['-I./models'] )

      if os.path.isdir( './models/Ball/src' ):
         source_dirs.extend( ['./models/Ball/src/'] )
      if os.path.isdir( './models/DistIf/src' ):
         source_dirs.extend( ['./models/DistIf/src/'] )
      if os.path.isdir( './models/encoding/src' ):
         source_dirs.extend( ['./models/encoding/src/'] )
      if os.path.isdir( './models/EntityDynamics/src' ):
         source_dirs.extend( ['./models/EntityDynamics/src/'] )
      if os.path.isdir( './models/FrameDynamics/src' ):
         source_dirs.extend( ['./models/FrameDynamics/src/'] )
      if os.path.isdir( './models/SAIntegrator/src' ):
         source_dirs.extend( ['./models/SAIntegrator/src/'] )
      if os.path.isdir( './models/simconfig/src' ):
         source_dirs.extend( ['./models/simconfig/src/'] )
      if os.path.isdir( './models/sine/src' ):
         source_dirs.extend( ['./models/sine/src/'] )
      if os.path.isdir( './sims/TrickHLA/SIM_Encoders/models' ):
         source_dirs.extend( ['./sims/TrickHLA/SIM_Encoders/models/src/'] )
         include_dirs.extend( ['-I./sims/TrickHLA/SIM_Encoders'] )

   # JEOD
   if args.process_all or args.process_JEOD:
      source_dirs.extend ( ['./source/JEOD/'] )
      if jeod_home:
         include_dirs.extend( ['-I' + jeod_home + '/models'] )

   # IMSim
   if args.process_all or args.process_IMSim:
      source_dirs.extend ( ['./source/IMSim/'] )

   # SpaceFOM
   if args.process_all or args.process_SpaceFOM:
      source_dirs.extend ( ['./source/SpaceFOM/'] )

   # TrickHLA
   if args.process_all or args.process_TrickHLA:
      source_dirs.extend ( ['./source/TrickHLA/'] )

   # TrickHLA Encoding
   if args.process_all or args.process_TrickHLA_encoding:
      source_dirs.extend ( ['./source/TrickHLA/encoding/'] )

   # TrickHLA Time
   if args.process_all or args.process_TrickHLA_time:
      source_dirs.extend ( ['./source/TrickHLA/time/'] )

   # TrickHLA Utils
   if args.process_all or args.process_TrickHLA_utils:
      source_dirs.extend ( ['./source/TrickHLA/utils/'] )

   # Add usr local include path if it exists.
   if os.path.isdir( '/usr/local/include' ):
      include_dirs.extend( ['-I/usr/local/include'] )

   # IWYU args
   iwyu_args.append( '-std=c++14' )


   # Echo check the data if verbose is selected.
   if args.verbose:
      TrickHLAMessage.status( '---------------------------------------------------------------------' )
      TrickHLAMessage.status( 'trickhla_home    = ' + trickhla_home )
      TrickHLAMessage.status( 'trickhla_defines = ' + ' '.join( trickhla_defines ) )
      TrickHLAMessage.status( 'source_dirs      = ' + ' '.join( source_dirs ) )
      TrickHLAMessage.status( 'include_dirs     = ' + ' '.join( include_dirs ) )
      TrickHLAMessage.status( 'trick_home       = ' + trick_home )
      if tsync_home:
         TrickHLAMessage.status( 'tsync_home       = ' + tsync_home )
      TrickHLAMessage.status( 'iwyu_cmd         = ' + iwyu_cmd )
      TrickHLAMessage.status( 'iwyu_ver         = ' + iwyu_ver )
      TrickHLAMessage.status( 'iwyu_args        = ' + ' '.join( iwyu_args ) )
      TrickHLAMessage.status( '---------------------------------------------------------------------' )

   # Build the list if source filenames given the source directories.
   source_files = []
   file_cnt = 0
   file_added_cnt = 0
   for src_dir in source_dirs:
      for dir_path, dir_names, filenames in os.walk( src_dir ):
         for filename in filenames:
            if filename.endswith( ".cpp" ) or filename.endswith( ".cxx" ) or filename.endswith( ".cc" ) or filename.endswith( ".c" ):
               file_cnt += 1
               if ( file_cnt >= file_start_cnt ) and ( ( file_cnt_limit <= 0 ) or ( file_added_cnt < file_cnt_limit ) ):
                  source_files.append( os.path.join( dir_path, filename ) )
                  file_added_cnt += 1

   #
   # Execute the include-what-you-use command
   #
   for src_file in source_files:
      # Form the include-what-you-use command with command line options.
      iwyu_command = [ iwyu_cmd ]
      iwyu_command.extend( iwyu_args )
      iwyu_command.extend( trickhla_defines )
      iwyu_command.extend( include_dirs )
      iwyu_command.append( src_file )

      if args.test_only:
         TrickHLAMessage.status( 'Would execute: ' + ' '.join( iwyu_command ) )
      else:
         if args.very_verbose:
            TrickHLAMessage.status( 'Executing: ' + ' '.join( iwyu_command ) )

         # Execute the include-what-you-use command.
         try:
   
            iwyu_proc = subprocess.Popen( iwyu_command, stdout = subprocess.PIPE, stderr = subprocess.PIPE, text = True )
   
            if ( True ):
               # Print files summary as they are processed.
               while True:
                  line = iwyu_proc.stderr.readline()
                  if line == '' and iwyu_proc.poll() is not None:
                     break
                  if line:
                     print( line.strip() )
   
            else:
               # Use spinner to indicate activity.
               count = 0
               while iwyu_proc.poll() is None:
   
                  # Spinner.
                  if count % 4 == 0:
                     sys.stdout.write( '\b' )
                     sys.stdout.write( '|' )
                     sys.stdout.flush()
                  if count % 4 == 1:
                     sys.stdout.write( '\b' )
                     sys.stdout.write( '/' )
                     sys.stdout.flush()
                  if count % 4 == 2:
                     sys.stdout.write( '\b' )
                     sys.stdout.write( '-' )
                     sys.stdout.flush()
                  if count % 4 == 3:
                     sys.stdout.write( '\b' )
                     sys.stdout.write( '\\' )
                     sys.stdout.flush()
                  count += 1
   
                  # Check process activity 10 times a second.
                  time.sleep( 0.1 )
   
               # Clear the spinner line.
               sys.stdout.write( '\b' )
               sys.stdout.write( ' ' )
               sys.stdout.write( '\b' )
               sys.stdout.flush()
   
            output, errors = iwyu_proc.communicate()
            print( errors )
            print( output )
   
         except subprocess.CalledProcessError:
            TrickHLAMessage.failure( '\'include-what-you-use\' command failed! '
                                     +subprocess.CalledProcessError.message )

   # Let the user know that we are done.
   TrickHLAMessage.success( 'Finished checking TrickHLA source code.' )

   return


# Function to find the include-what-you-use command.
#
# This function searches common locations for the include-what-you-use command.
#
# @return iwyu_command The path to the include-what-you-use command.
# @return iwyu_version The version of include-what-you-use.
# @param  iwyu_tidy_bin     The path to the include-what-you-use programs directory.
# @param  verbose            Flag to set if verbose outputs are on.
#
def find_include_what_you_use( iwyu_tidy_bin, verbose = True ):

   # Initialize the include-what-you-use command path.
   iwyu_command = None
   iwyu_version = None

   # Check to see if the path to the include-what-you-use binaries are set.
   if iwyu_tidy_bin:

      # Use the command line path to set the command.
      iwyu_command = iwyu_tidy_bin + '/include-what-you-use'

   # iwyu_tidy_bin is not set so lets check other options.
   else:

      # Check to see if the $LLVM_HOME environment variable is defined and set.
      LLVM_HOME = os.environ.get( 'LLVM_HOME' )
      if LLVM_HOME:

         # Check to see if LLVM_HOME exists.
         if os.path.isdir( LLVM_HOME ):

            # Form the include-what-you-use command based on LLVM_HOME.
            iwyu_command = LLVM_HOME + '/bin/include-what-you-use'

            if verbose:
               # Let the user know that we have LLVM_HOME and where it is.
               TrickHLAMessage.status( 'LLVM_HOME: ' + LLVM_HOME )

         else:
            TrickHLAMessage.failure( 'LLVM_HOME not found: ' + LLVM_HOME )

      else:

         # LLVM_HOME is not set so look in standard locations for
         # include-what-you-use starting with the system path first.
         iwyu_command = shutil.which( 'include-what-you-use' )
         if iwyu_command is None:
            if os.path.isfile( '/usr/bin/include-what-you-use' ):
               iwyu_command = '/usr/bin/include-what-you-use'
            elif os.path.isfile( '/usr/local/bin/include-what-you-use' ):
               iwyu_command = '/usr/local/bin/include-what-you-use'
            elif os.path.isfile( '/usr/local/opt/llvm/bin/include-what-you-use' ):
               iwyu_command = '/usr/local/opt/llvm/bin/include-what-you-use'

   # We're finished hunting. Now let's check for the include-what-you-use command.
   if iwyu_command is None:
      TrickHLAMessage.failure( 'Could not find the include-what-you-use command!' )
   else:
      if not os.path.isfile( iwyu_command ):
         TrickHLAMessage.failure( 'Could not find the include-what-you-use command!: '
                                  +iwyu_command )
      else:
         if verbose:
            TrickHLAMessage.status( 'Using include-what-you-use command: ' + iwyu_command )

   #
   # Now get the include-what-you-use version ID tag.
   #
   try:
      tidy_ver_cmd = [iwyu_command, '--version']
      grep_cmd = ['grep', '-i', 'include-what-you-use']

      tidy_ver_process = subprocess.Popen( tidy_ver_cmd, stdout = subprocess.PIPE )
      grep_process = subprocess.Popen( grep_cmd, stdin = tidy_ver_process.stdout, stdout = subprocess.PIPE )
      tidy_ver_process.stdout.close()

      output, _ = grep_process.communicate()
      iwyu_version = output.decode().rstrip()

   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'include-what-you-use --version\' command failed!: '
                               +iwyu_command + ', '
                               +subprocess.CalledProcessError.message )

   return iwyu_command, iwyu_version


# Function to find the Trick home directory and return the Trick version.
#
# This function searches common locations for Trick and returns the version.
#
# @return trick_home          The location of to the Trick home directory.
# @return trick_version       The Trick version tag.
# @return trick_version_year  The Trick year version.
# @param trick_path           The path to the Trick home directory.
# @param verbose              Flag to set if verbose outputs are on.
#
def find_trick( trick_path = None, verbose = True ):

   # Check to see if the path to the Trick home directory is already set.
   if trick_path:

      # Use the value passed in.
      trick_home = trick_path

   # The path to the Trick home directory was not passed in so lets check other options.
   else:

      # Check to see if the $TRICK_HOME environment variable is defined and set.
      trick_home = os.environ.get( 'TRICK_HOME' )
      if trick_home:

         # Check to see if TRICK_HOME exists.
         if os.path.isdir( trick_home ):
            if verbose:
               # Let the user know that we have TRICK_HOME and where it is.
               TrickHLAMessage.status( 'TRICK_HOME: ' + trick_home )

         else:
            TrickHLAMessage.failure( 'TRICK_HOME not found: ' + trick_home )

      else:

         # Capture the output from the 'trick-gte TRICK_HOME' command.
         trick_gte_cmd = ['trick-gte', 'TRICK_HOME']
         try:
            trick_home = subprocess.check_output( trick_gte_cmd ).decode( 'utf8', errors = 'strict' ).strip()

         except subprocess.CalledProcessError:
            TrickHLAMessage.failure( '\'trick-gte\' command failed!: '
                                     +trick_gte_cmd + ', '
                                     +subprocess.CalledProcessError.message )

         # Look in other possible locations.
         if trick_home == '':
            # TRICK_HOME is not set so look in the standard locations.
            if os.path.isdir( '/usr/local/Trick' ):
               trick_home = '/usr/local/Trick'
            elif os.path.isdir( '/Applications/Trick' ):
               trick_home = '/Applications/Trick'
            elif os.path.isdir( os.environ.get( 'HOME' ) + '/Trick' ):
               trick_home = os.environ.get( 'HOME' ) + '/Trick'

   # We're finished hunting. Now let's check for the Trick home directory.
   if trick_home is None:
      TrickHLAMessage.failure( 'Could not find the Trick home directory!' )
   else:
      if os.path.isdir( trick_home ) is False:
         TrickHLAMessage.failure( 'Could not find the Trick home directory: '
                                  +trick_home )
         trick_home = None
      else:
         if verbose:
            TrickHLAMessage.status( 'Using TRICK_HOME: ' + trick_home )

   #
   # Now get the Trick version.
   #
   # Form the trick-version command based on TRICK_HOME.
   trick_version_cmd = trick_home + '/bin/trick-version'
   if os.path.isfile( trick_version_cmd ) is False:
      TrickHLAMessage.failure( 'The \'trick-version\' command not found!: '
                               +trick_version_cmd )

   # Capture the output from the 'trick-version -version' command.
   trick_ver_number_cmd = [trick_version_cmd, '-version']
   try:
      trick_version = subprocess.check_output( trick_ver_number_cmd ).decode( 'utf8', errors = 'strict' ).strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'trick-version -version\' command failed!: '
                               +trick_ver_number_cmd + ', '
                               +subprocess.CalledProcessError.message )

   # Capture the output from the 'trick-version -year' command.
   trick_ver_yr_cmd = [trick_version_cmd, '-year']
   try:
      trick_version_year = subprocess.check_output( trick_ver_yr_cmd ).decode( 'utf8', errors = 'strict' ).strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'trick-version -year\' command failed!: '
                               +trick_ver_yr_cmd + ', '
                               +subprocess.CalledProcessError.message )

   # Return what we found.
   return trick_home, trick_version, trick_version_year


# Function to find the HLA RTI home directory.
#
# This function searches common locations for the installed HLA RTI.
#
# @return rti_home  The location of to the RTI home directory.
# @param  rti_path  The path to the Trick home directory.
# @param  verbose   Flag to set if verbose outputs are on.
#
def find_hla_rti( rti_path = None, verbose = True ):

   # Check to see if the path to the HLA RTI home directory is already set.
   if rti_path:

      # Use the value passed in.
      rti_home = rti_path

   # The path to the HLA RTI home directory was not passed in so lets check other options.
   else:

      # Check to see if the $RTI_HOME environment variable is defined and set.
      rti_home = os.environ.get( 'RTI_HOME' )

   # We're finished hunting. Now let's check for the HLA RTI home directory.
   if rti_home is None:
      TrickHLAMessage.failure( 'Could not find the HLA RTI home directory!' )
   else:
      if os.path.isdir( rti_home ) is False:
         TrickHLAMessage.failure( 'Could not find the HLA RTI home directory: ' + rti_home )
         rti_home = None
      else:
         if verbose:
            TrickHLAMessage.status( 'Using RTI_HOME: ' + rti_home )

   # Return what we found.
   return rti_home


# Function to find the JEOD home directory.
#
# This function searches common locations for the installed JEOD.
#
# @return jeod_home  The location of to the JEOD home directory.
# @param  jeod_path  The path to the JEOD home directory.
# @param  verbose    Flag to set if verbose outputs are on.
#
def find_jeod( jeod_path = None, verbose = True ):

   # Check to see if the path to the JEOD home directory is already set.
   if jeod_path:

      # Use the value passed in.
      jeod_home = jeod_path

   # The path to the JEOD home directory was not passed in so lets check other options.
   else:

      # Check to see if the $JEOD_HOME environment variable is defined and set.
      jeod_home = os.environ.get( 'JEOD_HOME' )

   # We're finished hunting. Now let's check for the JEOD home directory.
   if jeod_home is None:
      TrickHLAMessage.status( 'Could not find the JEOD home directory!' )
   else:
      if os.path.isdir( jeod_home ) is False:
         TrickHLAMessage.status( 'Could not find the JEOD home directory: ' + jeod_home )
         jeod_home = None
      else:
         if verbose:
            TrickHLAMessage.status( 'Using JEOD_HOME: ' + jeod_home )

   # Return what we found.
   return jeod_home


#
# Call the main function.
#
main()
