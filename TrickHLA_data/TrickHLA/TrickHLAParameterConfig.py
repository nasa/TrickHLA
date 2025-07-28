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
#    (((Edwin Z. Crues) (NASA/ER7) (Jan 2019) (--) (SpaceFOM support and testing.))
#     ((Dan Dexter) (NASA/ER6) (June 2025) (--) (Added fixed record support.)))
##############################################################################
import trick
from ..TrickHLA.TrickHLARecordElementConfig import *


class TrickHLAParameterConfig( TrickHLARecordElementConfig ):

   FOM_name = None

   def __init__( self,
                 FOM_name,
                 trick_name   = None,
                 rti_encoding = trick.ENCODING_UNKNOWN ):

      TrickHLARecordElementConfig.__init__( self, rti_encoding, trick_name )

      self.FOM_name = FOM_name

      return


   def initialize( self, parameter ):

      parameter.FOM_name     = str( self.FOM_name )
      parameter.trick_name   = str( self.trick_name )
      parameter.rti_encoding = self.rti_encoding

      if ( self.rti_encoding == trick.TrickHLA.ENCODING_FIXED_RECORD ):
         self.initialize_fixed_record( parameter, self )

      return
