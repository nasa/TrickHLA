####################################################################################################
# PURPOSE:
#    (These are python input file classes to connect HLA FOM parameters to simulation model
#     variables for a CabinAtmoConserveParameters object.
#
# REFERENCE:
#    ()
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Inherits from the base TrickHLAObjectConfig class)
#     (Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Jason Harvey) (CACI) (October 2024) (--) (Initial)))
####################################################################################################
import trick
from TrickHLA_data.TrickHLA.TrickHLAObjectConfig import *
from TrickHLA_data.TrickHLA.TrickHLAAttributeConfig import *

class ConserveParamsObjectConfig( TrickHLAObjectConfig ):

    def __init__( self,
                  thla_federate_name,
                  model_name,                       # trick variable name of the CabinAtmoConserveParameters object
                  isOwner,                          # True if this side owns the HLA object
                  FOM_type = 'ConservationParams',  # FOM objectClass type
                  threadId = None ):                # Trick child thread

        # Call the base class constructor and tell it the FOM object class.
        TrickHLAObjectConfig.__init__( self,
                                       thla_create               = isOwner, # this locally owns
                                       thla_instance_name        = thla_federate_name,
                                       thla_FOM_name             = FOM_type,
                                       thla_lag_comp_instance    = None,
                                       thla_lag_comp_type        = None,
                                       thla_ownership_instance   = None,
                                       thla_deleted_instance     = None,
                                       thla_conditional_instance = None,
                                       thla_packing_instance     = None,
                                       thla_object               = None,   #thla_manager_object
                                       thla_thread_IDs           = threadId )
      
        # Build the object attribute list.
        self.add_attributes( model_name )

    def add_attributes( self, model_name ):
        # Map terms of a FluidDistIfFOM message to FluidDistributed2WayBus attributes:
        #                 FOM attribute name, model variable name, encoding type 
        Attribute_Map = [('energy',   '.energy',   trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('moles',    '.moles',    trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesN2',  '.molesN2',  trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesO2',  '.molesO2',  trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesH2O', '.molesH2O', trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesCO2', '.molesCO2', trick.TrickHLA.ENCODING_LITTLE_ENDIAN)]

        for attr in Attribute_Map:
            attribute = TrickHLAAttributeConfig( FOM_name      = attr[0],
                                                 trick_name    = model_name + attr[1],
                                                 publish       = True,
                                                 subscribe     = True,
                                                 locally_owned = self.hla_create,
                                                 config        = trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                                 rti_encoding  = attr[2] )
            self.add_attribute( attribute )
