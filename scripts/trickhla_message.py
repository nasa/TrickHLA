############################################################################
# @class TrickHLAMessage
# @file trickhla_message.py
# @brief Defines the general TrickHLA Python messaging class.
#
# This is a Python module used to define a consistent message handling
# approach for the Python based scripts that are used to manage the TrickHLA
# code base and repository.
############################################################################
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, April 2019, --, Initial creation.}
# @rev_entry{ Dan Dexter, NASA ER6, TrickHLA, March 2020, --, Changed to use system \
# default text color for Status messages.}
# @revs_end
############################################################################
import sys
import os

class TrickHLAMessage():

   _COMMENT_COLOR = '\033[95m'
   _STATUS_COLOR  = '\033[93m'
   _SUCCESS_COLOR = '\033[32m'
   _WARNING_COLOR = '\033[94m'
   _FAILURE_COLOR = '\033[91m'
   _END_COLOR     = '\033[0m'

   _COMMENT_TAG = 'COMMENT: '
   _STATUS_TAG  = 'STATUS: '
   _SUCCESS_TAG = 'SUCCESS: '
   _WARNING_TAG = 'WARNING: '
   _FAILURE_TAG = os.linesep + 'ERROR: '

   _EOL = os.linesep

   @staticmethod
   def comment( message ):
      print ( TrickHLAMessage._COMMENT_COLOR + TrickHLAMessage._COMMENT_TAG \
       +message + TrickHLAMessage._END_COLOR )

   @staticmethod
   def status( message ):
      # Use the system default text color.
      print ( TrickHLAMessage._STATUS_TAG + message + TrickHLAMessage._END_COLOR )

   @staticmethod
   def success( message ):
      print ( TrickHLAMessage._SUCCESS_COLOR + TrickHLAMessage._SUCCESS_TAG \
       +message + TrickHLAMessage._END_COLOR )

   @staticmethod
   def warning( message ):
      print ( TrickHLAMessage._WARNING_COLOR + TrickHLAMessage._WARNING_TAG \
       +message + TrickHLAMessage._END_COLOR )

   @staticmethod
   def failure( message ):
      sys.exit( TrickHLAMessage._FAILURE_COLOR + TrickHLAMessage._FAILURE_TAG \
                +message + TrickHLAMessage._END_COLOR + TrickHLAMessage._EOL )
