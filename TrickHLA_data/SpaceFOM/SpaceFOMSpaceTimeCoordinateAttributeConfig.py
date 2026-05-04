##############################################################################
# PURPOSE:
#    (This is a python class file used by the top level SpaceFOM input
#     file interface for specifying a Space Time Coordinate object attribute.)
#
# REFERENCE:
#    (Trick 17 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Relies on the simulation level trick module.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA/ER6) (May 2026) (--) (Initial implementation.)))
##############################################################################
import trick
from ..TrickHLA.TrickHLAAttributeConfig import *
from ..SpaceFOM.SpaceFOMSpaceTimeCoordinateFixedRecordConfig import *


class SpaceFOMSpaceTimeCoordinateAttributeConfig( TrickHLAAttributeConfig, SpaceFOMSpaceTimeCoordinateFixedRecordConfig ):

   def __init__( self,
                 stc_FOM_name,
                 stc_trick_name      = None,
                 stc_publish         = True,
                 stc_subscribe       = True,
                 stc_locally_owned   = True,
                 stc_config          = trick.TrickHLA.CONFIG_CYCLIC,
                 stc_preferred_order = trick.TrickHLA.TRANSPORT_SPECIFIED_IN_FOM ):

      TrickHLAAttributeConfig.__init__(
                                 self,
                                 FOM_name        = stc_FOM_name,
                                 trick_name      = stc_trick_name,
                                 publish         = stc_publish,
                                 subscribe       = stc_subscribe,
                                 locally_owned   = stc_locally_owned,
                                 config          = stc_config,
                                 rti_encoding    = trick.TrickHLA.ENCODING_FIXED_RECORD,
                                 preferred_order = stc_preferred_order )

      SpaceFOMSpaceTimeCoordinateFixedRecordConfig.__init__(
                                 self,
                                 stc_rec_trick_name = stc_trick_name )

      return
