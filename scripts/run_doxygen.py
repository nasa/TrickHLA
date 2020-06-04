#!/usr/bin/python
## @file run_doxygen.py
# @brief This program uses the Doxygen tool to generate TrickHLA documentation.
# 
# This is a Python program used to generate documentation for the TrickHLA
# source code.  This tool uses the Doxygen tool and a series of configuration
# files.  The default is to generate documentation for just the models.
# There are command line options to generate separate documentation for
# the Python scripts, the simulations, or everything.
#
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, April 2019, --, Initial creation based off of Valkyrie scripts.}
# @revs_end

import sys
import os
import subprocess
import shutil
import argparse
import distutils.spawn

from trickhla_message import *
from trickhla_environment import *



## Main routine.
def main() :
   
   # General command line parameters.
   clean          = False
   generate_latex = False
   test           = False
   trickhla_home  = ''
   verbose_output = False
   
   # Command line document generation options.
   combined_docs = False
   model_docs    = True
   scripts_docs  = False
   sims_docs     = False

   version_id  = '3.0'
   version_tag = None

   # Setup command line argument parsing.
   parser = argparse.ArgumentParser( prog='run_doxygen', \
                                     description='Create Doxygen documentation.' )
   parser.add_argument( '-a', '--all', \
                        help='Generate documentation for everything.', \
                        action="store_true" )
   parser.add_argument( '-c', '--clean', \
                        help='Clean up TrickHLA documentation artifacts.',\
                        action="store_true" )
   parser.add_argument( '-e', '--everything', \
                        help='Generate documentation for everything.', \
                        action="store_true" )
   parser.add_argument( '-l', '--latex', \
                        help='Generate latex output.', \
                        action="store_true" )
   parser.add_argument( '-m', '--model', \
                        help='Generate documentation for TrickHLA models.', \
                        action="store_true" )
   parser.add_argument( '-p', '--python', \
                        help='Generate documentation for Python scripts.', \
                        action="store_true" )
   parser.add_argument( '-s', '--sims', \
                        help='Generate documentation for TrickHLA example sims.', \
                        action="store_true" )
   parser.add_argument( '--thla_home', \
                        help='Path to TrickHLA directory.' )
   parser.add_argument( '-t', '--test', help='Do not generate documentation, just show what would be done.',\
                         action="store_true" )
   parser.add_argument( '-v', '--verbose', \
                        help='Generate verbose output.', \
                        action="store_true" )
   parser.add_argument( '--version_id', help='Model version ID.' )
   parser.add_argument( '--version_tag', help='Model version tag.' )

   # Parse the command line arguments.
   args = parser.parse_args()

   # Assign command line parameters.
   clean          = args.clean
   generate_latex = args.latex
   test           = args.test
   verbose_output = args.verbose
   
   # Set the TrickHLA version string components.
   if args.version_id :
      version_id = args.version_id
   if args.version_tag :
      version_tag = args.version_tag
   
   # Determine which documentation to generate.
   if args.everything or args.all :
      combined_docs = True
   model_docs    = args.model
   scripts_docs  = args.python
   sims_docs     = args.sims
      
   #
   # Let's do some command line option sanity checks.
   #
   # Check for 'everything' redundancy.
   if combined_docs and model_docs :
      TrickHLAMessage.warning( 'The \'everything\' and \'model\' options are redundant!' )
      
   if combined_docs and scripts_docs :
      TrickHLAMessage.warning( 'The \'everything\' and \'python\' options are redundant!' )
      
   if combined_docs and sims_docs :
      TrickHLAMessage.warning( 'The \'everything\' and \'sims\' options are redundant!' )

   # If nothing else is set, then at least generate the model documentation.
   if not (args.all or args.everything or args.python or args.sims) :
      model_docs = True

   # If 'clean' specified as a command line option, make sure that the
   # user really wants to do that.  It WILL remove specified documentation.
   if args.clean and not args.test:
      TrickHLAMessage.warning( 'Cleaning up TrickHLA documentation!' )
      check_clean = raw_input( 'Are you sure you want to do this? [y]: ' )
      if check_clean == 'y' :
         TrickHLAMessage.status( 'Proceeding with cleaning . . .' )
      else:
         TrickHLAMessage.status( 'You chose not to proceed. Exiting!' )
         sys.exit()

   # Check for TrickHLA.
   if args.thla_home :
      trickhla_home = ars.thla_home
   else:
      trickhla_home = os.environ.get( 'TRICKHLA_HOME' )
      if trickhla_home == None :
         trickhla_home = os.environ.get( 'TRICK_HLA_HOME' )
         if trickhla_home == None :
            TrickHLAMessage.faliure( 'TRICKHLA_HOME not set!' )
   
   # Make sure that the directory actually exists before to move there.
   if os.path.isdir( trickhla_home ) :
      if verbose_output :
         TrickHLAMessage.status( 'Moving to TRICKHLA_HOME: ' + trickhla_home )
      os.chdir( trickhla_home )
   else :
      TrickHLAMessage.failure( 'TrickHLA Home directory not found!' )

   # Check to make sure we are where we think we are . . .
   # In TrickHLA, that means there are docs, include, models, scripts, sims,
   # and source directories.
   error = False
   if not os.path.isdir( 'docs' ):
      error = True
      TrickHLAMessage.warning( 'No \'docs\' directory.' )
   if not os.path.isdir( 'include' ):
      error = True
      TrickHLAMessage.warning( 'No \'include\' directory.' )
   if not os.path.isdir( 'models' ):
      error = True
      TrickHLAMessage.warning( 'No \'models\' directory.' )
   if not os.path.isdir( 'scripts' ):
      error = True
      TrickHLAMessage.warning( 'No \'scripts\' directory.' )
   if not os.path.isdir( 'sims' ):
      error = True
      TrickHLAMessage.warning( 'No \'sims\' directory.' )
   if not os.path.isdir( 'source' ):
      error = True
      TrickHLAMessage.warning( 'No \'source\' directory.' )
      
   if error :
      TrickHLAMessage.failure( 'This is not the TrickHLA directory you are looking for!' )

   # Build the TrickHLA version string.
   trickhla_version = build_version_string( version_id, version_tag )
   if verbose_output :
      TrickHLAMessage.status( 'Generating documentation for TrickHLA: ' + trickhla_version )

   # Check that we can find the Doxygen command.
   doxygen_cmnd = distutils.spawn.find_executable( 'doxygen' )
   if doxygen_cmnd:
      # Check to see if Doxygen command is executable.
      if os.access( doxygen_cmnd, os.X_OK ) :
         if verbose_output :
            TrickHLAMessage.status( 'Found Doxygen command: ' +  doxygen_cmnd )
      else :
         TrickHLAMessage.failure( 'Doxygen command not executable: '  +  doxygen_cmnd )
   else :
      TrickHLAMessage.failure( 'Could not find Doxygen command!  Check your command path.' )

   # Check to see if the doxygen directory exists.
   if not os.path.isdir( 'docs/doxygen' ):
      TrickHLAMessage.failure( 'Could not find \'docs/doxygen\' directory!' )

   # Check for the Doxygen configuration files.
   dox_config = trickhla_home + "/docs/doxygen/TrickHLA_config.dox"
   if not os.path.isfile( dox_config ):
      TrickHLAMessage.failure( "Cannot find the Doxygen configuration file: "\
                              + dox_config + "!" )
   
   # generate the requested documentation.
   if combined_docs :
      
      # Set Doxygen configuration parameters.
      output_dir    = 'docs/combined'
      erro_log_file = 'docs/doxygen/combinedErr.txt'
      file_patterns = '*.cpp *.hh *.c *.h *.d *.sm S_define *.md *.markdown *.py'
      exclude       = '*/verif */build'
      latex_dir     = output_dir + '/latex'
      
      # Check if this is a clean command.
      if clean :
         cleanup_document_dir( output_dir, erro_log_file, test, verbose_output )

      # Otherwise, generate the documentation.
      else :
         if verbose_output :
            TrickHLAMessage.status( 'Generating combined TrickHLA documentation.' )

         # Set the Doxygen environment variables.
         setup_doxygen_environment( trickhla_version,
                                    output_dir,
                                    erro_log_file,
                                    file_patterns,
                                    exclude,
                                    generate_latex,
                                    verbose_output )
      
         # Generate the documentation.
         generate_docs( erro_log_file, test, verbose_output )
         
         # Build the LaTeX documentation if indicated.
         if ( generate_latex ) :
            build_latex_doc( latex_dir, test, verbose_output )
      
   # END: if combined_docs
      
   if model_docs :
      
      # Set Doxygen configuration parameters.
      output_dir    = 'docs/models'
      erro_log_file = 'docs/doxygen/modelsErr.txt'
      file_patterns = '*.cpp *.hh *.c *.h *.d *.md *.markdown'
      exclude       = '*/verif */build */scripts */sims'
      latex_dir     = output_dir + '/latex'

      # Check if this is a clean command.
      if clean :
         cleanup_document_dir( output_dir, erro_log_file, test, verbose_output )

      # Otherwise, generate the documentation.
      else :
         if verbose_output :
            TrickHLAMessage.status( 'Generating TrickHLA model documentation.' )

         # Set the Doxygen environment variables.
         setup_doxygen_environment( trickhla_version,
                                    output_dir,
                                    erro_log_file,
                                    file_patterns,
                                    exclude,
                                    generate_latex,
                                    verbose_output )
      
         # Generate the documentation.
         generate_docs( erro_log_file, test, verbose_output )
         
         # Build the LaTeX documentation if indicated.
         if ( generate_latex ) :
            build_latex_doc( latex_dir, test, verbose_output )
         
   # END: if model_docs
   
   if scripts_docs :
      
      # Set Doxygen configuration parameters.
      output_dir    = 'docs/scripts'
      erro_log_file = 'docs/doxygen/scriptsErr.txt'
      file_patterns = '*.py'
      exclude       = '*/verif */build */include */source'
      latex_dir     = output_dir + '/latex'

      # Check if this is a clean command.
      if clean :
         cleanup_document_dir( output_dir, erro_log_file, test, verbose_output )

      # Otherwise, generate the documentation.
      else :
         if verbose_output :
            TrickHLAMessage.status( 'Generating TrickHLA scripts documentation.' )

         # Set the Doxygen environment variables.
         setup_doxygen_environment( trickhla_version,
                                    output_dir,
                                    erro_log_file,
                                    file_patterns,
                                    exclude,
                                    generate_latex,
                                    verbose_output )
      
         # Generate the documentation.
         generate_docs( erro_log_file, test, verbose_output )
         
         # Build the LaTeX documentation if indicated.
         if ( generate_latex ) :
            build_latex_doc( latex_dir, test, verbose_output )
         
   # END: if scripts_docs
   
   if sims_docs :
      
      # Set Doxygen configuration parameters.
      output_dir    = 'docs/sims'
      erro_log_file = 'docs/doxygen/simsErr.txt'
      file_patterns = '*.sm S_define *.md *.markdown *.py'
      exclude       = '*/verif */build */scripts */include */source'
      latex_dir     = output_dir + '/latex'

      # Check if this is a clean command.
      if clean :
         cleanup_document_dir( output_dir, erro_log_file, test, verbose_output )

      # Otherwise, generate the documentation.
      else :
         if verbose_output :
            TrickHLAMessage.status( 'Generating TrickHLA sims documentation.' )

         # Set the Doxygen environment variables.
         setup_doxygen_environment( trickhla_version,
                                    output_dir,
                                    erro_log_file,
                                    file_patterns,
                                    exclude,
                                    generate_latex,
                                    verbose_output )
      
         # Generate the documentation.
         generate_docs( erro_log_file, test, verbose_output )
         
         # Build the LaTeX documentation if indicated.
         if ( generate_latex ) :
            build_latex_doc( latex_dir, test, verbose_output )
         
   # END: if sims_docs

   # Print out status message.
   TrickHLAMessage.success( 'Successfully built TrickHLA documentation.' )

   return


## Build up the TrickHLA version string.
#
# This routine builds up the TrickHLA version string for the Doxygen
# generated documentation.  The version string corresponds to the TrickHLA
# version.  The version tag represents a tag string corresponding to some
# identifier in the configuration management of the TrickHLA code base.
# If one is not supplied, this command will extract the current Git hash.
#
# @param  version_id   TrickHLA version ID string.
# @param  version_tag  Configuration management version tag string.
#
def build_version_string(
   version_id,
   version_tag = None ) :

   # If Tag is set then use it.
   if version_tag :
      version_string = version_id + ': ' + version_tag
   # Otherwise, get the Git hash.
   else :
      version_tag = subprocess.check_output( ['git', 'rev-parse', '--short', 'HEAD'] )
      version_string = version_id + ': Git#' + version_tag.rstrip()
   
   return version_string


## Set the Doxygen generation environment options.
#
# This routine sets the values of the environment variables used in the
# Doxygen configuration file to generate documentation for TrickHLA.
#
# @param  version        Version identification string.
# @param  output_dir     String with path to Doxygen output directory.
# @param  error_log_file String with path to Doxygen error log file.
# @param  file_patterns  String with file patterns to be processed.
# @param  exclude        String with directories to exclude.
# @param  latex          Flag to turn on or off LaTeX generation.
# @param  verbose        Flag to set if verbose outputs are on.
#
def setup_doxygen_environment(
   version,
   output_dir,
   error_log_file,
   file_patterns,
   exclude,
   latex,
   verbose ) :

   # Set the environment variables need for Doxygen configuration.
   os.environ[ 'TRICKHLA_VERSION' ] = version
   os.environ[ 'TRICKHLA_DOX_OUTPUT' ] = output_dir
   os.environ[ 'TRICKHLA_DOX_LOG' ] = error_log_file
   os.environ[ 'TRICKHLA_DOX_FILE_PATTERNS' ] = file_patterns
   os.environ[ 'TRICKHLA_DOX_EXCLUDE' ] = exclude
   if latex :
      os.environ[ 'TRICKHLA_DOX_LATEX' ] = 'YES'
   else :
      os.environ[ 'TRICKHLA_DOX_LATEX' ] = 'NO'

   if verbose :
      TrickHLAMessage.status( 'TRICKHLA_VERSION: ' + os.environ.get( 'TRICKHLA_VERSION' ) )
      TrickHLAMessage.status( 'TRICKHLA_DOX_OUTPUT: ' + os.environ.get( 'TRICKHLA_DOX_OUTPUT' ) )
      TrickHLAMessage.status( 'TRICKHLA_DOX_LOG: ' + os.environ.get( 'TRICKHLA_DOX_LOG' ) )
      TrickHLAMessage.status( 'TRICKHLA_DOX_FILE_PATTERNS: ' + os.environ.get( 'TRICKHLA_DOX_FILE_PATTERNS' ) )
      TrickHLAMessage.status( 'TRICKHLA_DOX_EXCLUDE: ' + os.environ.get( 'TRICKHLA_DOX_EXCLUDE' ) )
      TrickHLAMessage.status( 'TRICKHLA_DOX_LATEX: ' + os.environ.get( 'TRICKHLA_DOX_LATEX' ) )

   return


## Function to cleans up all the Doxygen artifacts in a directory.
#
# This routine will clean up all the identifiable Doxygen generated files
# in a given directory.
#
# @param  dir_path     The path to the directory to clean up.
# @param  error_log    The path to the error log.
# @param  test_only    Flag indicating to only show what would be done.
# @param  verbose      Flag to set if verbose outputs are on.
#
def cleanup_document_dir( dir_path, error_log,  test_only, verbose ) :
   
   # If the Doxygen documentation directory exists, then remove it.
   if os.path.isdir( dir_path ):
      if test_only :
         TrickHLAMessage.status( 'Would remove document directory: ' + dir_path )
      else :
         if verbose :
            TrickHLAMessage.status( 'Removing document directory: ' + dir_path )
         shutil.rmtree( dir_path )
   else:
      TrickHLAMessage.warning( 'Could not find document directory: ' + dir_path )
      
   # If the Doxygen error log file exists, then remove it.
   if os.path.isfile( error_log ):
      if test_only :
         TrickHLAMessage.status( 'Would remove error log: ' + error_log )
      else :
         if verbose :
            TrickHLAMessage.status( 'Removing error log: ' + error_log )
         os.remove( error_log )
   else:
      TrickHLAMessage.warning( 'Could not find error log: ' + error_log )
   
   return


## Function to build the Doxygen generated LaTeX documentation.
#
# This routine will go into the Doxygen generated LaTeX documentation
# directory and make the documentation.
#
# @param  latex_dir    The path to the LaTeX directory.
# @param  test_only    Flag indicating to only show what would be done.
# @param  verbose      Flag to set if verbose outputs are on.
#
def build_latex_doc( latex_dir, test_only, verbose ) :

   error = False  # No error yet.

   if verbose :
      TrickHLAMessage.status( 'Building the LaTeX documentation in ' + latex_dir )

   # Store the current working directory location.
   original_dir = os.path.abspath(os.path.curdir)

   # Move into the generated LaTeX directory.
   if os.path.isdir( latex_dir ):
      if test_only :
         TrickHLAMessage.status( 'Would move into the \'' + latex_dir + '\' directory.' )
      else :
         if verbose :
            TrickHLAMessage.status( 'Moving into the \'' + latex_dir + '\' directory.' )
         os.chdir( latex_dir )
   else :
      error = True
      TrickHLAMessage.warning( 'Could not find \'' + latex_dir + '\' directory!' )
      return error

   # Make the LaTeX documentation.
   os.system( 'make' )

   # Check for successful build of LaTeX documentation.
   if os.path.isfile( 'refman.pdf' ):
      if verbose :
         TrickHLAMessage.status( 'Generated the TrickHLA LaTeX documentation.' )
   else:
      error = True
      TrickHLAMessage.warning( 'Failed to build TrickHLA LaTeX documentation!' )
   
   # Move back to the original directory.
   os.chdir( original_dir )
   
   return error


## Function to execute the Doxygen command to generate documentation.
#
# This routine will execute the Doxygen command to generate the TrickHLA
# documentation.
#
# @param  error_log  The path to the Doxygen error log file.
# @param  test_only  Flag indicating to only show what would be done.
# @param  verbose    Flag to set if verbose outputs are on.
#
def generate_docs( error_log, test_only, verbose ) :
   
   command = 'doxygen docs/doxygen/TrickHLA_config.dox'

   # Have Doxygen generate the model documentation.
   if test_only :
      TrickHLAMessage.status( 'Would execute Doxygen command: ' + command )
   else :
      
      if verbose :
         TrickHLAMessage.status( 'Executing Doxygen command: ' + command )
         
      # Execute the command.
      ret = os.system( command )
      if ret != 0:
         TrickHLAMessage.failure( 'Doxygen command failed!' )

      # Check for Doxygen documentation generation errors.
      statinfo = os.stat( error_log )
      if statinfo.st_size != 0 :
         TrickHLAMessage.warning( 'Errors detected! Check error log: ' + error_log )

   return

#
# Call the main function.
#
main()