##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level TrickHLA input
#     file interface for specifying a federate object attribute.)
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


class TrickHLAAttributeConfig( TrickHLARecordElementConfig ):

   FOM_name      = None
   publish       = True
   subscribe     = True
   locally_owned = True
   config        = trick.TrickHLA.CONFIG_CYCLIC

   def __init__( self,
                 FOM_name,
                 trick_name    = None,
                 publish       = True,
                 subscribe     = True,
                 locally_owned = True,
                 config        = trick.TrickHLA.CONFIG_CYCLIC,
                 rti_encoding  = trick.TrickHLA.ENCODING_UNKNOWN ):

      TrickHLARecordElementConfig.__init__( self, rti_encoding, trick_name )

      self.FOM_name      = FOM_name
      self.publish       = publish
      self.subscribe     = subscribe
      self.locally_owned = locally_owned
      self.config        = config

      return


   def initialize( self, attribute ):

      attribute.FOM_name      = str( self.FOM_name )
      attribute.trick_name    = str( self.trick_name )
      attribute.publish       = self.publish
      attribute.subscribe     = self.subscribe
      attribute.locally_owned = self.locally_owned
      attribute.config        = self.config
      attribute.rti_encoding  = self.rti_encoding

      if ( self.rti_encoding == trick.TrickHLA.ENCODING_FIXED_RECORD ):
         self.initialize_fixed_record( attribute, self )

      return
