
def HLA_ball_config( THLA_object,
                     sim_obj_name,
                     obj_name,
                     obj_packing,
                     obj_create = True ):
   
   # Configure the object this federate will create an HLA instance and
   # publish data for.
   THLA_object.FOM_name            = 'Ball'
   THLA_object.name                = obj_name
   THLA_object.create_HLA_instance = obj_create
   THLA_object.packing             = obj_packing
   THLA_object.lag_comp            = None
   THLA_object.lag_comp_type       = trick.LAG_COMPENSATION_NONE
   THLA_object.ownership           = None
   THLA_object.conditional         = None
   THLA_object.deleted             = None
   THLA_object.attr_count          = 6
   THLA_object.attributes          = trick.sim_services.alloc_type( THLA_object.attr_count, 'TrickHLA::Attribute' )
   
   THLA_object.attributes[0].FOM_name        = 'name'
   THLA_object.attributes[0].trick_name      = sim_obj_name + '.packing.name'
   THLA_object.attributes[0].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
   THLA_object.attributes[0].publish         = True
   THLA_object.attributes[0].locally_owned   = True
   THLA_object.attributes[0].rti_encoding    = trick.ENCODING_UNICODE_STRING

   THLA_object.attributes[1].FOM_name        = 'time'
   THLA_object.attributes[1].trick_name      = sim_obj_name + '.packing.time'
   THLA_object.attributes[1].config          = trick.CONFIG_CYCLIC
   THLA_object.attributes[1].publish         = True
   THLA_object.attributes[1].subscribe       = True
   THLA_object.attributes[1].locally_owned   = True
   THLA_object.attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[2].FOM_name        = 'position'
   THLA_object.attributes[2].trick_name      = sim_obj_name + '.packing.position'
   THLA_object.attributes[2].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
   THLA_object.attributes[2].publish         = True
   THLA_object.attributes[2].subscribe       = True
   THLA_object.attributes[2].locally_owned   = True
   THLA_object.attributes[2].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[3].FOM_name        = 'velocity'
   THLA_object.attributes[3].trick_name      = sim_obj_name + '.packing.velocity'
   THLA_object.attributes[3].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
   THLA_object.attributes[3].publish         = True
   THLA_object.attributes[3].subscribe       = True
   THLA_object.attributes[3].locally_owned   = True
   THLA_object.attributes[3].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[4].FOM_name        = 'acceleration'
   THLA_object.attributes[4].trick_name      = sim_obj_name + '.packing.acceleration'
   THLA_object.attributes[4].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
   THLA_object.attributes[4].publish         = True
   THLA_object.attributes[4].subscribe       = True
   THLA_object.attributes[4].locally_owned   = True
   THLA_object.attributes[4].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   
   THLA_object.attributes[5].FOM_name        = 'force'
   THLA_object.attributes[5].trick_name      = sim_obj_name + '.packing.external_force'
   THLA_object.attributes[5].config          = trick.CONFIG_INITIALIZE + trick.CONFIG_CYCLIC
   THLA_object.attributes[5].publish         = True
   THLA_object.attributes[5].subscribe       = True
   THLA_object.attributes[5].locally_owned   = True
   THLA_object.attributes[5].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN
   