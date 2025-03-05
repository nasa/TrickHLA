#!/usr/bin/env python3
# @file check_code.py
# @brief This program applies a cppcheck program to TrickHLA source code.
#
# This is a Python program used to check the TrickHLA source code using the
# cppcheck utility.
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Initial creation based off of Dan Dexter's cppcheck_trickhla.csh script.}
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, March 2020, --, Fixed cppcheck arguments for checking includes, fixed args validation, using Trick version year and added --inconclusive option.}
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, June 2021, --, Using a build output directory unique to the check being done for better build artifact caching.}
# @revs_end
#
import argparse
import os
import subprocess
import shutil
import sys
import textwrap
import time

from trickhla_environment import *
from trickhla_message import *


# Main routine.
def main():

   # Set defaults for TrickHLA, TRICK_HOME and JEOD_HOME
   trickhla_home = '.'
   trick_home = None
   jeod_home = None

   # Initialize the lists that go into the cppcheck command argument list.
   system_includes = []
   trickhla_defines = []
   trickhla_include_dirs = []
   trickhla_source_dirs = []
   trickhla_ignore = []
   cppcheck_args = []

   # Set default values for the cppcheck files and directories.
   cppcheck_output_dir = 'cppcheck'
   cppcheck_build_dir = cppcheck_output_dir + '/build'
   cppcheck_output_text_file = 'cppcheck_output.txt'
   cppcheck_output_xml_file = 'cppcheck_output.xml'
   ccpcheck_suppresion_file = 'scripts/cppcheck_suppression.txt'
   ccpcheck_suppresion_autogen_file = 'suppression_autogen.txt'

   #
   # Setup command line argument parsing.
   #
   parser = argparse.ArgumentParser( prog = 'check_code', \
                                     formatter_class = argparse.RawDescriptionHelpFormatter, \
                                     description = 'Check the TrickHLA source code using cppcheck.', \
                                     epilog = textwrap.dedent( '''\n
Options -s, -e, -u, -x: Default error suppression file: ''' + ccpcheck_suppresion_file + '''\n
Option -a: Auto-generated error suppression file: ''' + cppcheck_output_dir + '/' + ccpcheck_suppresion_autogen_file + '''\n
Examples:\n  check_code -s -o -v --exhaustive\n  check_code -i -o -v --exhaustive\n  check_code -e -o -v --exhaustive\n  check_code -c -vv''' ) )

   parser.add_argument( '-a', '--autogen', \
                        help = 'Auto-generate a suppression file for all issues. Use this as a starting point for a project specific suppression file.', \
                        action = 'store_true', dest = 'autogen' )
   parser.add_argument( '-b', '--bin', \
                        help = 'Path to cppcheck binaries directory.', \
                        dest = 'bin_path' )
   parser.add_argument( '--clang', \
                        help = 'Use clang parser instead of the cppcheck built in parser (experimental).', \
                        action = 'store_true', dest = 'use_clang_parser' )
   parser.add_argument( '-c', '--clean', \
                        help = 'Clean all generated files.', \
                        action = 'store_true', dest = 'clean_gen_files' )
   parser.add_argument( '--cstyle_casts', \
                        help = 'Suppress C-style casts.', \
                        action = 'store_true', dest = 'suppress_cstylecasts' )
   parser.add_argument( '-e', '--errors', \
                        help = 'Check source code for errors only.', \
                        action = 'store_true', dest = 'check_errors_only' )
   parser.add_argument( '--exhaustive', \
                        help = 'Exhaustive checking, and it will take a while to complete.', \
                        action = 'store_true', dest = 'exhaustive' )
   parser.add_argument( '-i', '--includes', help = 'Check the #include\'s.', \
                        action = 'store_true', dest = 'check_includes' )
   parser.add_argument( '-n', '--inconclusive', help = 'Allow cppcheck to report even though the analysis is inconclusive. Caution, there are false positives with this option.', \
                        action = 'store_true', dest = 'check_inconclusive' )
   parser.add_argument( '-o', '--overwrite', help = 'Overwrite generated files.', \
                        action = 'store_true', dest = 'overwrite_files' )
   parser.add_argument( '-p', '--path', help = 'Path to TrickHLA directory.', \
                        dest = 'trickhla_home' )
   parser.add_argument( '-s', '--check_all', \
                        help = 'Check source code for all issues.', \
                        action = 'store_true', dest = 'check_all' )
   parser.add_argument( '-t', '--test', \
                        help = 'Do not run cppcheck just show what would be done.', \
                        action = 'store_true', dest = 'test_only' )
   parser.add_argument( '--rti', \
                        help = 'Provide a path to the RTI installation directory.', \
                        dest = 'rti_home' )
   parser.add_argument( '--trick', \
                        help = 'Provide a path to the Trick installation directory.', \
                        dest = 'trick_home' )
   parser.add_argument( '--jeod', \
                        help = 'Provide a path to the JEOD installation directory.', \
                        dest = 'jeod_home' )
   parser.add_argument( '-u', '--unused', \
                        help = 'Check source code for unused functions.',
                        action = 'store_true', dest = 'check_for_unused' )
   parser.add_argument( '-v', '--verbose', \
                        help = 'Check source code with verbose detailed error information.', \
                        action = 'store_true', dest = 'verbose' )
   parser.add_argument( '-vv', '--very_verbose', \
                        help = 'Run with both ccpcheck verbose output and script verbose output.', \
                        action = 'store_true', dest = 'very_verbose' )
   parser.add_argument( '-x', '--xml_output', \
                        help = 'Same as -s but the output is XML.', \
                        action = 'store_true', dest = 'generate_xml' )

   # Parse the command line arguments.
   args = parser.parse_args()

   #
   # Let's do some command line option sanity checks.
   #
   arg_error = False
   # Check for 'autogen' incompatible arguments.
   if args.autogen and args.clean_gen_files:
      arg_error = True
      TrickHLAMessage.warning( 'The \'-a\' and \'-c\' options are incompatible!' )

   if args.autogen and args.check_errors_only:
      arg_error = True
      TrickHLAMessage.warning( 'The \'-a\' and \'-e\' options are incompatible!' )

   if args.autogen and args.check_for_unused:
      arg_error = True
      TrickHLAMessage.warning( 'The \'-a\' and \'-u\' options are incompatible!' )

   if args.autogen and args.check_all:
      arg_error = True
      TrickHLAMessage.warning( 'The \'-a\' and \'-s\' options are incompatible!' )

   if args.autogen and args.generate_xml:
      arg_error = True
      TrickHLAMessage.warning( 'The \'-a\' and \'-x\' options are incompatible!' )

   # User must specify one of -a, -c, -e, -i, -s, -u, or -x.
   required_arg_cnt = 0
   if args.autogen:
      # -a
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_autogen'
   if args.clean_gen_files:
      # -c
      required_arg_cnt += 1
   if args.check_errors_only:
      # -e
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_check_errors_only'
   if args.check_includes:
      # -i
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_check_includes'
   if args.check_all:
      # -s
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_check_all'
   if args.check_for_unused:
      # -u
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_check_unused'
   if args.generate_xml:
      # -x
      required_arg_cnt += 1
      cppcheck_build_dir = cppcheck_output_dir + '/build_generate_xml'
   if required_arg_cnt == 0:
      arg_error = True
      TrickHLAMessage.warning( 'You must specify one of \'-a\', \'-c\', \'-e\', \'-i\', \'-s\', \'-u\' or \'-x\'!' )
   elif required_arg_cnt >= 2:
      arg_error = True
      TrickHLAMessage.warning( 'Only specify one of \'-a\', \'-c\', \'-e\', \'-i\', \'-s\', \'-u\' or \'-x\'!' )

   if arg_error:
      TrickHLAMessage.failure( 'Error detected in parsing command arguments!' )

   # If we selected very verbose, then we also need to set verbose in the cppcheck output.
   if args.very_verbose:
      args.verbose = True

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
            TrickHLAMessage.failure( 'TRICKHLA_HOME not set!' )

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

   # Find the cppcheck command and get the cppcheck version ID.
   cppcheck_cmd, cppcheck_ver = find_cppcheck( args.bin_path, args.very_verbose )
   if cppcheck_cmd is None:
      TrickHLAMessage.failure( 'No cppcheck command found!' )

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
   rti_include = rti_home + '/api/cpp/HLA_1516-2010'
   if os.path.isdir( rti_include ) is False:
      rti_include = rti_home + '/include'
      if os.path.isdir( rti_include ) is False:
         TrickHLAMessage.failure( 'Could not find the HLA RTI include directory: ' + rti_include )
   if args.very_verbose:
      TrickHLAMessage.status( 'Path to HLA RTI include directory: ' + rti_include )

   # Determine the path to JEOD home directory.
   jeod_home = find_jeod( args.jeod_home, args.very_verbose )
   if args.very_verbose and jeod_home:
      TrickHLAMessage.status( 'Path to JEOD: ' + jeod_home )

   # Define preprocessor symbols we use for TrickHLA and set the TRICK_VER based on the
   # version of the Trick simulation environment we found in our Path.
   trickhla_defines = ['-DTRICK_VER=' + trick_ver_year, '-DIEEE_1516_2010', '-DFPU_CW_PROTECTION' ]

   # Form relative paths to all the include directories used by TrickHLA.
   trickhla_include_dirs.extend( ['-I', './include'] )
   trickhla_include_dirs.extend( ['-I', trick_home + '/include'] )
   trickhla_include_dirs.extend( ['-I', trick_home + '/include/trick/compat'] )
   trickhla_include_dirs.extend( ['-I', trick_home + '/trick_source'] )
   trickhla_include_dirs.extend( ['-I', rti_include] )
   if jeod_home:
      trickhla_include_dirs.extend( ['-I', jeod_home + '/models'] )
   if os.path.isdir( './models/DistIf/include' ):
      trickhla_include_dirs.extend( ['-I', './models/DistIf/include'] )
   if os.path.isdir( './models/EntityDynamics/include' ):
      trickhla_include_dirs.extend( ['-I', './models/EntityDynamics/include'] )
   if os.path.isdir( './models/FrameDynamics/include' ):
      trickhla_include_dirs.extend( ['-I', './models/FrameDynamics/include'] )
   if os.path.isdir( './models/SAIntegrator/include' ):
      trickhla_include_dirs.extend( ['-I', './models/SAIntegrator/include'] )
   if os.path.isdir( './models/simconfig/include' ):
      trickhla_include_dirs.extend( ['-I', './models/simconfig/include'] )
   if os.path.isdir( './models/sine/include' ):
      trickhla_include_dirs.extend( ['-I', './models/sine/include'] )
   if os.path.isdir( './models/Wheelbot/Battery/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Battery/include'] )
   if os.path.isdir( './models/Wheelbot/Control/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Control/include'] )
   if os.path.isdir( './models/Wheelbot/Electrical/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Electrical/include'] )
   if os.path.isdir( './models/Wheelbot/Guidance/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Guidance/include'] )
   if os.path.isdir( './models/Wheelbot/Motor/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Motor/include'] )
   if os.path.isdir( './models/Wheelbot/Vehicle/include' ):
      trickhla_include_dirs.extend( ['-I', './models/Wheelbot/Vehicle/include'] )

   # Form relative paths to all the source directories used by TrickHLA.
   trickhla_source_dirs.extend ( ['./source'] )
   if os.path.isdir( './models/DistIf/src' ):
      trickhla_source_dirs.extend( ['./models/DistIf/src'] )
   if os.path.isdir( './models/EntityDynamics/src' ):
      trickhla_source_dirs.extend( ['./models/EntityDynamics/src'] )
   if os.path.isdir( './models/FrameDynamics/src' ):
      trickhla_source_dirs.extend( ['./models/FrameDynamics/src'] )
   if os.path.isdir( './models/SAIntegrator/src' ):
      trickhla_source_dirs.extend( ['./models/SAIntegrator/src'] )
   if os.path.isdir( './models/simconfig/src' ):
      trickhla_source_dirs.extend ( ['./models/simconfig/src'] )
   if os.path.isdir( './models/sine/src' ):
      trickhla_source_dirs.extend ( ['./models/sine/src'] )
   if os.path.isdir( './models/Wheelbot/Battery/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Battery/src'] )
   if os.path.isdir( './models/Wheelbot/Control/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Control/src'] )
   if os.path.isdir( './models/Wheelbot/Electrical/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Electrical/src'] )
   if os.path.isdir( './models/Wheelbot/Guidance/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Guidance/src'] )
   if os.path.isdir( './models/Wheelbot/Motor/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Motor/src'] )
   if os.path.isdir( './models/Wheelbot/Vehicle/src' ):
      trickhla_source_dirs.extend( ['./models/Wheelbot/Vehicle/src'] )

   # Add usr local include path if it exists.
   if os.path.isdir( '/usr/local/include' ):
      trickhla_include_dirs.extend( ['-I', '/usr/local/include'] )

   # Check to see if we are just cleaning up.
   if args.clean_gen_files:
      if cleanup_output_directory( cppcheck_output_dir, args.test_only, args.very_verbose ):
         TrickHLAMessage.success( 'Finished cleaning up TrickHLA cppcheck artifacts.' )
      else:
         TrickHLAMessage.failure( 'Error cleaning up TrickHLA cppcheck artifacts.' )
      return

   # Create the cppcheck output directory.
   if os.path.isdir( cppcheck_output_dir ):
      if args.overwrite_files:
         if args.very_verbose:
            TrickHLAMessage.status( 'Directory \'' + trickhla_home + '/'
                                    + cppcheck_output_dir + '\' exists!' )
      else:
         TrickHLAMessage.failure( 'Directory \'' + trickhla_home + '/'
                                  + cppcheck_output_dir + '\' exists!' )
   else:
      if args.test_only:
         TrickHLAMessage.status( 'Would create the \'' + trickhla_home + '/'
                                 + cppcheck_output_dir + '\' directory.' )
      else:
         if args.very_verbose:
            TrickHLAMessage.status( 'Creating \'' + trickhla_home + '/'
                                    + cppcheck_output_dir + '\' directory.' )
         # Create the cppcheck output directory.
         os.mkdir( cppcheck_output_dir )

   # Create the cppcheck/build directory.
   if os.path.isdir( cppcheck_build_dir + '' ):
      if args.overwrite_files:
         if args.very_verbose:
            TrickHLAMessage.status( 'Directory \'' + trickhla_home + '/'
                                    + cppcheck_build_dir + '\' exists!' )
      else:
         TrickHLAMessage.failure( 'Directory \'' + trickhla_home + '/'
                                  + cppcheck_build_dir + '\' exists!' )
   else:
      if args.test_only:
         TrickHLAMessage.status( 'Would create the \'' + trickhla_home + '/'
                                 + cppcheck_build_dir + '\' directory.' )
      else:
         if args.very_verbose:
            TrickHLAMessage.status( 'Creating \'' + trickhla_home + '/'
                                    + cppcheck_build_dir + '\' directory.' )
         # Create the cppcheck build directory.
         os.mkdir( cppcheck_build_dir )

   #
   # Let's start building the cppcheck command arguments.
   #
   # Suppress directives for non-TrickHLA include header files.
   if args.check_includes:
      trickhla_ignore.append( '--suppress=unmatchedSuppression' )
      # Ignore/suppress the Trick header file warnings.
      trickhla_ignore.append( '--suppress=missingInclude:' + trick_home + '/include/trick/rand_generator.h' )
      trickhla_ignore.append( '--suppress=preprocessorErrorDirective:' + trick_home + '/trick_source/er7_utils/interface/include/er7_class.hh' )
   else:
      # Ignore/suppress the HLA Evolved standard header file warnings.
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + rti_include + '/RTI/Exception.h' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + rti_include + '/RTI/Typedefs.h' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + rti_include + '/RTI/encoding/EncodingExceptions.h' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + rti_include + '/RTI/time/HLAinteger64Interval.h' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + rti_include + '/RTI/time/HLAinteger64Time.h' )
      trickhla_ignore.append( '--suppress=unhandledExceptionSpecification:' + rti_include + '/RTI/encoding/DataElement.h' )
      trickhla_ignore.append( '--suppress=unhandledExceptionSpecification:' + rti_include + '/RTI/encoding/HLAfixedRecord.h' )
      trickhla_ignore.append( '--suppress=unhandledExceptionSpecification:' + rti_include + '/RTI/time/HLAinteger64Time.h' )
      # Ignore/suppress the Trick header file warnings.
      trickhla_ignore.append( '--suppress=constParameterReference:' + trick_home + '/trick_source/er7_utils/integration/core/include/first_order_ode_integrator.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/DataRecordGroup.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/InstrumentBase.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/RealtimeSync.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/SysThread.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/ThreadBase.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/Threads.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/include/trick/ThreadTrigger.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/trick_source/er7_utils/integration/core/include/integrator_result.hh' )
      trickhla_ignore.append( '--suppress=noExplicitConstructor:' + trick_home + '/trick_source/er7_utils/integration/core/include/integrator_result_merger_container.hh' )
      trickhla_ignore.append( '--suppress=preprocessorErrorDirective:' + trick_home + '/trick_source/er7_utils/interface/include/er7_class.hh' )
      trickhla_ignore.append( '--suppress=uninitMemberVar:' + trick_home + '/include/trick/SimObject.hh' )
      trickhla_ignore.append( '--suppress=uselessOverride:' + trick_home + '/trick_source/er7_utils/integration/core/include/integrator_result_merger.hh' )
      # Ignore/suppress the JEOD header file warnings.
      if jeod_home:
         trickhla_ignore.append( '--suppress=cstyleCast:' + jeod_home + '/models/utils/math/include/numerical_inline.hh' )
         trickhla_ignore.append( '--suppress=constParameterPointer:' + jeod_home + '/models/utils/memory/include/jeod_alloc_construct_destruct.hh' )
         trickhla_ignore.append( '--suppress=constParameterPointer:' + jeod_home + '/models/utils/integration/include/restartable_state_integrator.hh' )
         trickhla_ignore.append( '--suppress=duplInheritedMember:' + jeod_home + '/models/utils/container/include/jeod_vector.hh' )
         trickhla_ignore.append( '--suppress=duplInheritedMember:' + jeod_home + '/models/utils/container/include/pointer_container.hh' )
         trickhla_ignore.append( '--suppress=duplInheritedMember:' + jeod_home + '/models/utils/memory/include/memory_table.hh' )
         trickhla_ignore.append( '--suppress=noExplicitConstructor:' + jeod_home + '/models/utils/container/include/container.hh' )
         trickhla_ignore.append( '--suppress=noExplicitConstructor:' + jeod_home + '/models/utils/memory/include/memory_type.hh' )
         trickhla_ignore.append( '--suppress=noExplicitConstructor:' + jeod_home + '/models/utils/named_item/include/named_item.hh' )
         trickhla_ignore.append( '--suppress=returnByReference:' + jeod_home + '/models/utils/named_item/include/named_item.hh' )
         trickhla_ignore.append( '--suppress=syntaxError:' + jeod_home + '/models/dynamics/mass/include/mass_point_init.hh' )
         trickhla_ignore.append( '--suppress=uselessOverride:' + jeod_home + '/models/utils/sim_interface/include/jeod_integrator_interface.hh' )

   if args.suppress_cstylecasts:
      # Suppress C-style casts.
      trickhla_ignore.append( '--suppress=cstyleCast' )

   # Set the cppcheck arguments based on the type of code checking the user wants to do.
   if args.check_errors_only:
      # We don't need to set any specific arguments for the default error checking.
      None
   elif args.check_for_unused:
      # Only look for unused functions.
      cppcheck_args.append( '--enable=unusedFunction' )
   elif args.check_includes:
      # Do we check all the #include's.
      cppcheck_args.append( '--check-config' )
      cppcheck_args.append( '--suppress=missingIncludeSystem' )
   elif args.check_all:
      # Check for everything except unused functions.
      cppcheck_args.append( '--enable=warning,style,performance,portability,information,missingInclude' )
      cppcheck_args.append( '--suppress=missingIncludeSystem' )
   elif args.autogen:
      # Check for everything including unused functions for the auto-generated
      # suppression list.
      cppcheck_args.append( '--enable=all' )
   else:
      # Otherwise check everything.
      cppcheck_args.append( '--enable=all' )

   if args.verbose:
      # Enable more verbose detailed error information.
      cppcheck_args.append( '-v' )

   if args.check_inconclusive:
      # From cppcheck usage: Allow cppcheck to report even though the analysis
      # is inconclusive. There are false positives with this option. Each result
      # must be carefully investigated before you know if it is good or bad.
      cppcheck_args.append( '--inconclusive' )

   # Use the experimental clang parser instead of the built in cppcheck parser.
   if args.use_clang_parser:
      cppcheck_args.append( '--clang' )

   # Use the c++03 standard.
   cppcheck_args.append( '--std=c++03' )

   # Configure cppcheck to use an output directory to cache build results.
   cppcheck_args.append( '--cppcheck-build-dir=' + cppcheck_build_dir )
   
   # Configure the checkers-report located in the build directory.
   cppcheck_args.append( '--checkers-report=' + cppcheck_build_dir + '/checkers-report.txt' )

   # --exhaustive Check level
   if args.exhaustive:
      cppcheck_args.append( '--check-level=exhaustive' )
   else:
      cppcheck_args.append( '--check-level=normal' )

   # If the -s, -e or -x options are specified use the parallel processing -j option.
   if args.check_all or args.check_errors_only or args.generate_xml:
      # Enable parallel cppcheck processing based on the total number of real CPU cores
      # the computer has.
      # Get the number of CPUs on the system.
      num_cpus = get_number_of_cpus()
      if args.very_verbose:
         TrickHLAMessage.status( 'Number of CPUs: ' + str( num_cpus ) )
      # Add the multi-processor parallelization arguments.
      cppc_loadmax = None
      if num_cpus > 1:
         cppc_loadmax = int( ( 3 * num_cpus ) / 2 )
         # Set the number of cppcheck job threads and limit the load.
         cppcheck_args.extend( ['-j', str( num_cpus ), '-l', str( cppc_loadmax )] )

   # All commands need these paths.
   cppcheck_args.append( '--quiet' )
   cppcheck_args.append( '--force' )
   cppcheck_args.extend( system_includes )
   cppcheck_args.extend( trickhla_ignore )
   cppcheck_args.extend( trickhla_defines )
   cppcheck_args.extend( trickhla_include_dirs )
   cppcheck_args.extend( trickhla_source_dirs )

   # If we are running cppcheck on the source then use a suppression file if it exists.
   if args.check_all or args.check_errors_only or args.check_for_unused or args.generate_xml:
      if os.path.isfile( ccpcheck_suppresion_file ):
         cppcheck_args.insert( 0, '--suppressions-list=' + ccpcheck_suppresion_file )

   # Enable XML output.
   if args.generate_xml:
      cppcheck_args.insert( 0, '--xml-version=2 ' )

   # Check if auto-generating a suppression file.
   if args.autogen:
      # NOTE: Don't enable inline-suppressions so we get all issues in the file.

      # Set the output file.
      output_file = cppcheck_output_dir + '/' + ccpcheck_suppresion_autogen_file
      cppcheck_args.append( '--output-file=' + output_file )

      # Define the output format template.
      cppcheck_args.insert( 0, '--template={id}:{file}:{line}' )

   else:

      # Enable inline suppressions.
      cppcheck_args.append( '--inline-suppr' )

      # Set the output file.
      if args.generate_xml:
         output_file = cppcheck_output_dir + '/' + cppcheck_output_xml_file
      else:
         output_file = cppcheck_output_dir + '/' + cppcheck_output_text_file

      cppcheck_args.append( '--output-file=' + output_file )

   # Echo check the data if verbose is selected.
   if args.verbose:
      TrickHLAMessage.status( '---------------------------------------------------------------------' )
      TrickHLAMessage.status( 'trickhla_home         = ' + trickhla_home )
      TrickHLAMessage.status( 'trickhla_defines      = ' + ' '.join( trickhla_defines ) )
      TrickHLAMessage.status( 'trickhla_source_dirs  = ' + ' '.join( trickhla_source_dirs ) )
      TrickHLAMessage.status( 'trickhla_include_dirs = ' + ' '.join( trickhla_include_dirs ) )
      TrickHLAMessage.status( 'trick_home            = ' + trick_home )
      TrickHLAMessage.status( 'cppcheck_cmd          = ' + cppcheck_cmd )
      TrickHLAMessage.status( 'cppcheck_ver          = ' + cppcheck_ver )
      TrickHLAMessage.status( 'cppcheck_output_dir   = ' + cppcheck_output_dir )
      TrickHLAMessage.status( 'cppcheck_build_dir    = ' + cppcheck_build_dir )
      TrickHLAMessage.status( 'cppcheck_args         = ' + ' '.join( cppcheck_args ) )
      TrickHLAMessage.status( '---------------------------------------------------------------------' )

   # Form the cppcheck command with command line options.
   shell_command = [ cppcheck_cmd ]
   shell_command.extend( cppcheck_args )

   #
   # Execute the cppcheck command
   #
   if args.test_only:
      TrickHLAMessage.status( 'Would execute: ' + ' '.join( shell_command ) )
   else:
      if args.very_verbose:
         TrickHLAMessage.status( 'Executing: ' + ' '.join( shell_command ) )

      # Execute the cppcheck command.
      try:

         # Spawn off the cppcheck process using Popen.
         cppcheck_proc = subprocess.Popen( shell_command )

         # Use spinner to indicate activity.
         count = 0
         while cppcheck_proc.poll() is None:

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

      except subprocess.CalledProcessError:
         TrickHLAMessage.failure( '\'cppcheck\' command failed! '
                                  + subprocess.CalledProcessError.message )

   # Let the user know that we are done.
   TrickHLAMessage.success( 'Finished checking TrickHLA source code.' )
   TrickHLAMessage.success( 'Check results in file: ' + output_file )

   return


# Function to find the cppcheck command.
#
# This function searches common locations for the cppcheck command.
#
# @return cppcheck_command The path to the cppcheck command.
# @return cppcheck_version The version of cppcheck.
# @param  cppcheck_bin     The path to the cppcheck programs directory.
# @param  verbose          Flag to set if verbose outputs are on.
#
def find_cppcheck( cppcheck_bin, verbose = True ):

   # Initialize the cppcheck command path.
   cppcheck_command = None
   cppcheck_version = None

   # Check to see if the path to the cppcheck binaries are set.
   if cppcheck_bin:

      # Use the command line path to set the command.
      cppcheck_command = cppcheck_bin + '/cppcheck'

   # cppcheck_bin is not set so lets check other options.
   else:

      # Check to see if the $CPPCHECK_HOME environment variable is defined and set.
      cppcheck_home = os.environ.get( 'CPPCHECK_HOME' )
      if cppcheck_home:

         # Check to see if CPPCHECK_HOME exists.
         if os.path.isdir( cppcheck_home ):

            # Form the cppcheck command based on CPPCHECK_HOME.
            cppcheck_command = cppcheck_home + '/cppcheck'

            if verbose:
               # Let the user know that we have CPPCHECK_HOME and where it is.
               TrickHLAMessage.status( 'CPPCHECK_HOME: ' + cppcheck_home )

         else:
            TrickHLAMessage.failure( 'CPPCHECK_HOME not found: ' + cppcheck_home )

      else:

         # CPPCHECK_HOME is not set so look in the standard locations for
         # cppcheck starting with the system path first.
         cppcheck_command = shutil.which( 'cppcheck' )
         if cppcheck_command is None:
            if os.path.isfile( '/usr/bin/cppcheck' ):
               cppcheck_command = '/usr/bin/cppcheck'
            elif os.path.isfile( '/usr/local/bin/cppcheck' ):
               cppcheck_command = '/usr/local/bin/cppcheck'
            elif os.path.isfile( '/opt/homebrew/bin/cppcheck' ):
               cppcheck_command = '/opt/homebrew/bin/cppcheck'

   # We're finished hunting. Now let's check for the cppcheck command.
   if cppcheck_command is None:
      TrickHLAMessage.failure( 'Could not find the cppcheck command!' )
   else:
      if not os.path.isfile( cppcheck_command ):
         TrickHLAMessage.failure( 'Could not find the cppcheck command!: '
                                  + cppcheck_command )
      else:
         if verbose:
            TrickHLAMessage.status( 'Using cppcheck command: ' + cppcheck_command )

   #
   # Now get the cppcheck version ID tag.
   #
   try:
      cppcheck_version = subprocess.check_output( [cppcheck_command, '--version'] ).decode('utf8', errors='strict').strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'cppcheck --version\' command failed!: '
                               + cppcheck_command + ', '
                               + subprocess.CalledProcessError.message )

   return cppcheck_command, cppcheck_version


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
            trick_home = subprocess.check_output( trick_gte_cmd ).decode('utf8', errors='strict').strip()

         except subprocess.CalledProcessError:
            TrickHLAMessage.failure( '\'trick-gte\' command failed!: '
                                     + trick_gte_cmd + ', '
                                     + subprocess.CalledProcessError.message )

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
                                  + trick_home )
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
                               + trick_version_cmd )

   # Capture the output from the 'trick-version -version' command.
   trick_ver_number_cmd = [trick_version_cmd, '-version']
   try:
      trick_version = subprocess.check_output( trick_ver_number_cmd ).decode('utf8', errors='strict').strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'trick-version -version\' command failed!: '
                               + trick_ver_number_cmd + ', '
                               + subprocess.CalledProcessError.message )

   # Capture the output from the 'trick-version -year' command.
   trick_ver_yr_cmd = [trick_version_cmd, '-year']
   try:
      trick_version_year = subprocess.check_output( trick_ver_yr_cmd ).decode('utf8', errors='strict').strip()
   except subprocess.CalledProcessError:
      TrickHLAMessage.failure( '\'trick-version -year\' command failed!: '
                               + trick_ver_yr_cmd + ', '
                               + subprocess.CalledProcessError.message )

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


# Function to get the number of CPUs on the system
#
# This routine will query the system to determine the number of CPUs
# available for processing.
# @return num_cpus Number of CPUs on the system.
#
def get_number_of_cpus():

   # There has to be at least 1 CPU.
   num_cpus = 1

   if os.path.isfile( '/usr/bin/nproc' ):

      # Linux OS
      num_cpus = int( subprocess.check_output( ['/usr/bin/nproc', '--all'] ) )

   elif os.path.isfile( '/proc/cpuinfo' ):

      # Linux OS
      num_cpus = int( subprocess.check_output( ['grep', '-c', '^processor', '/proc/cpuinfo'] ) )

   elif os.path.isfile( '/usr/sbin/sysctl' ):

      # Check for Mac OS
      try:
         system_type = subprocess.check_output( ['/usr/bin/uname', '-s'] ).decode('utf8', errors='strict').strip()
      except subprocess.CalledProcessError:
         TrickHLAMessage.failure( subprocess.CalledProcessError.message )

      if 'Darwin' in system_type:
         num_cpus = int( subprocess.check_output( ['/usr/sbin/sysctl', '-n', 'hw.ncpu'] ) )

   return num_cpus


# Function to cleans artifacts in the cppcheck output directory.
#
# This routine will clean up all the cppcheck output artifacts in the
# 'cppcheck' directory if the directory exists.
#
# @return status       Returns True if error detected.
# @param  cppcheck_dir The cppcheck output directory to clean up.
# @param  test_only    Flag indicating to only show what would be done.
# @param  verbose      Flag to set if verbose outputs are on.
#
def cleanup_output_directory( 
   cppcheck_dir = 'cppcheck',
   test_only = False,
   verbose = True ):

   if verbose:
      TrickHLAMessage.status( 'Cleaning up the cppcheck output directory!: ' + cppcheck_dir )

   # Check for the cppcheck output directory.
   if os.path.isdir( cppcheck_dir ):
      if test_only:
         TrickHLAMessage.status( 'Would remove \'' + cppcheck_dir + '\' directory and contents.' )
      else:
         if verbose:
            TrickHLAMessage.status( 'Removing \'' + cppcheck_dir + '\' directory and contents.' )
         shutil.rmtree( cppcheck_dir )

      # End: if test_only :
      return True

   else:
      TrickHLAMessage.warning( 'No \'' + cppcheck_dir + '\' directory found!' )

   # End: if os.path.isdir( cppcheck_dir ) :

   return False


#
# Call the main function.
#
main()
