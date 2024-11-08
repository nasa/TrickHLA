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
from Modified_data.TrickHLA.TrickHLAObjectConfig import *
from Modified_data.TrickHLA.TrickHLAAttributeConfig import *

class ConserveParamsObjectConfig(TrickHLAObjectConfig):

    def __init__( self,
                  thla_federate_name,
                  model_name,                       # trick variable name of the CabinAtmoConserveParameters object
                  isOwner,                          # True if this side owns the HLA object
                  FOM_type = 'ConservationParams',  # FOM objectClass type
                  threadId = None ):                # Trick child thread

        # Call the base class constructor and tell it the FOM object class.
        TrickHLAObjectConfig.__init__( self,
                                       isOwner,            #thla_create (this locally owns)
                                       thla_federate_name, #thla_instance_name
                                       FOM_type,           #thla_FOM_name
                                       None,               #thla_lag_comp_instance
                                       None,               #thla_lag_comp_type
                                       None,               #thla_ownership_instance
                                       None,               #thla_deleted_instance
                                       None,               #thla_conditional_instance
                                       None,               #thla_packing_instance
                                       None,               #thla_manager_object
                                       threadId )          #thla_thread_IDs
      
        # Build the object attribute list.
        self.add_attributes(model_name)

    def add_attributes( self, model_name ):
        # Map terms of a FluidDistIfFOM message to FluidDistributed2WayBus attributes:
        #                 FOM attribute name, model variable name, encoding type 
        Attribute_Map = [('energy',           '.energy',           trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('moles',            '.moles',            trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesN2',          '.molesN2',          trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesO2',          '.molesO2',          trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesH2O',         '.molesH2O',         trick.TrickHLA.ENCODING_LITTLE_ENDIAN),
                         ('molesCO2',         '.molesCO2',         trick.TrickHLA.ENCODING_LITTLE_ENDIAN)]

        for attr in Attribute_Map:
            attribute = TrickHLAAttributeConfig( attr[0],
                                                 model_name + attr[1],
                                                 True,
                                                 True,
                                                 self.hla_create,
                                                 trick.TrickHLA.CONFIG_INITIALIZE + trick.TrickHLA.CONFIG_CYCLIC,
                                                 attr[2] )
            self.add_attribute( attribute )
