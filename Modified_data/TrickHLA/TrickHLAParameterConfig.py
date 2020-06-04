##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level TrickHLA input
#     file interface for specifying a federate interaction parameter.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.)))
##############################################################################
import trick

class TrickHLAParameterConfig( object ):

   FOM_name      = None
   trick_name    = None
   rti_encoding  = trick.ENCODING_UNICODE_STRING
   

   def __init__( self,
      FOM_name,
      trick_name,
      rti_encoding  = trick.ENCODING_UNICODE_STRING ):

      self.FOM_name      = FOM_name
      self.trick_name    = trick_name
      self.rti_encoding  = rti_encoding

      return


   def initialize( self, parameter ):

      parameter.FOM_name      = self.FOM_name
      parameter.trick_name    = self.trick_name
      parameter.rti_encoding  = self.rti_encoding

      return

