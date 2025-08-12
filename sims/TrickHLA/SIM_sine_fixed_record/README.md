# SIM_sine_fixed_record

SIM_sine_fixed_record is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation is comprised
of two simulations. The A-side federate owns the analytic sine state and
the P-side federate owns the propagated sine state. The example shows how
to use and configure fixed record data.

---
### Configuring the Simulation

#### A-side federate fixed record configuration

From file RUN_a_side/input.py:

```
# FixedRecordTest.xml:
# MainFixedRecObject
# - field_1_string:  HLAunicodeString
# - field_2_float64: HLAfloat64LE
# - field_3_rec:     MainFixedRecord
#   + MainFixedRecord:  HLAfixedRecord
#     - elem_1_string:  HLAunicodeString
#     - elem_2_float64: HLAfloat64LE
#     - elem_3_record:  SecondaryFixedRecord
#       + SecondaryFixedRecord: HLAfixedRecord
#         - element_1_count: HLAinteger32LE
#         - element_2_name:  HLAunicodeString

# Configure the object this federate will create an HLA instance and
# publish data for.
THLA.manager.objects[2].FOM_name            = 'MainFixedRecObject'
THLA.manager.objects[2].name                = 'A-side-Federate.FixedRecord'
THLA.manager.objects[2].create_HLA_instance = True
THLA.manager.objects[2].packing             = R.packing
THLA.manager.objects[2].attr_count          = 3
THLA.manager.objects[2].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[2].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[2].attributes[0].FOM_name        = 'field_1_string'
THLA.manager.objects[2].attributes[0].trick_name      = 'R.packing.field_1_string'
THLA.manager.objects[2].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[0].publish         = True
THLA.manager.objects[2].attributes[0].subscribe       = True
THLA.manager.objects[2].attributes[0].locally_owned   = True
THLA.manager.objects[2].attributes[0].rti_encoding    = trick.ENCODING_UNICODE_STRING

THLA.manager.objects[2].attributes[1].FOM_name        = 'field_2_float64'
THLA.manager.objects[2].attributes[1].trick_name      = 'R.packing.field_2_float64'
THLA.manager.objects[2].attributes[1].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[1].publish         = True
THLA.manager.objects[2].attributes[1].subscribe       = True
THLA.manager.objects[2].attributes[1].locally_owned   = True
THLA.manager.objects[2].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[2].FOM_name        = 'field_3_rec'
THLA.manager.objects[2].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[2].publish         = True
THLA.manager.objects[2].attributes[2].subscribe       = True
THLA.manager.objects[2].attributes[2].locally_owned   = True
THLA.manager.objects[2].attributes[2].rti_encoding    = trick.ENCODING_FIXED_RECORD
THLA.manager.objects[2].attributes[2].element_count   = 3
THLA.manager.objects[2].attributes[2].elements        = trick.sim_services.alloc_type( THLA.manager.objects[2].attributes[2].element_count, 'TrickHLA::RecordElement' )
THLA.manager.objects[2].attributes[2].elements[0].trick_name    = 'R.packing.elem_1_string'
THLA.manager.objects[2].attributes[2].elements[0].rti_encoding  = trick.ENCODING_UNICODE_STRING
THLA.manager.objects[2].attributes[2].elements[1].trick_name    = 'R.packing.elem_2_float64'
THLA.manager.objects[2].attributes[2].elements[1].rti_encoding  = trick.ENCODING_LITTLE_ENDIAN
THLA.manager.objects[2].attributes[2].elements[2].rti_encoding  = trick.ENCODING_FIXED_RECORD
THLA.manager.objects[2].attributes[2].elements[2].element_count = 2
THLA.manager.objects[2].attributes[2].elements[2].elements      = trick.sim_services.alloc_type( THLA.manager.objects[2].attributes[2].elements[2].element_count, 'TrickHLA::RecordElement' )
THLA.manager.objects[2].attributes[2].elements[2].elements[0].trick_name   = 'R.packing.element_1_count'
THLA.manager.objects[2].attributes[2].elements[2].elements[0].rti_encoding = trick.ENCODING_LITTLE_ENDIAN
THLA.manager.objects[2].attributes[2].elements[2].elements[1].trick_name   = 'R.packing.element_2_name'
THLA.manager.objects[2].attributes[2].elements[2].elements[1].rti_encoding = trick.ENCODING_UNICODE_STRING
```

#### P-side federate fixed record configuration

From file RUN_p_side/input.py:

```
# FixedRecordTest.xml:
# MainFixedRecObject
# - field_1_string:  HLAunicodeString
# - field_2_float64: HLAfloat64LE
# - field_3_rec:     MainFixedRecord
#   + MainFixedRecord:  HLAfixedRecord
#     - elem_1_string:  HLAunicodeString
#     - elem_2_float64: HLAfloat64LE
#     - elem_3_record:  SecondaryFixedRecord
#       + SecondaryFixedRecord: HLAfixedRecord
#         - element_1_count: HLAinteger32LE
#         - element_2_name:  HLAunicodeString

# Configure the object this federate subscribes to but will not create an
# HLA instance for.
THLA.manager.objects[2].FOM_name            = 'MainFixedRecObject'
THLA.manager.objects[2].name                = 'A-side-Federate.FixedRecord'
THLA.manager.objects[2].create_HLA_instance = False
THLA.manager.objects[2].packing             = R.packing
THLA.manager.objects[2].attr_count          = 3
THLA.manager.objects[2].attributes          = trick.sim_services.alloc_type( THLA.manager.objects[2].attr_count, 'TrickHLA::Attribute' )

THLA.manager.objects[2].attributes[0].FOM_name        = 'field_1_string'
THLA.manager.objects[2].attributes[0].trick_name      = 'R.packing.field_1_string'
THLA.manager.objects[2].attributes[0].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[0].publish         = True
THLA.manager.objects[2].attributes[0].subscribe       = True
THLA.manager.objects[2].attributes[0].locally_owned   = False
THLA.manager.objects[2].attributes[0].rti_encoding    = trick.ENCODING_UNICODE_STRING

THLA.manager.objects[2].attributes[1].FOM_name        = 'field_2_float64'
THLA.manager.objects[2].attributes[1].trick_name      = 'R.packing.field_2_float64'
THLA.manager.objects[2].attributes[1].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[1].publish         = True
THLA.manager.objects[2].attributes[1].subscribe       = True
THLA.manager.objects[2].attributes[1].locally_owned   = False
THLA.manager.objects[2].attributes[1].rti_encoding    = trick.ENCODING_LITTLE_ENDIAN

THLA.manager.objects[2].attributes[2].FOM_name        = 'field_3_rec'
THLA.manager.objects[2].attributes[2].config          = trick.CONFIG_CYCLIC
THLA.manager.objects[2].attributes[2].publish         = True
THLA.manager.objects[2].attributes[2].subscribe       = True
THLA.manager.objects[2].attributes[2].locally_owned   = False
THLA.manager.objects[2].attributes[2].rti_encoding    = trick.ENCODING_FIXED_RECORD
THLA.manager.objects[2].attributes[2].element_count   = 3
THLA.manager.objects[2].attributes[2].elements        = trick.sim_services.alloc_type( THLA.manager.objects[2].attributes[2].element_count, 'TrickHLA::RecordElement' )
THLA.manager.objects[2].attributes[2].elements[0].trick_name    = 'R.packing.elem_1_string'
THLA.manager.objects[2].attributes[2].elements[0].rti_encoding  = trick.ENCODING_UNICODE_STRING
THLA.manager.objects[2].attributes[2].elements[1].trick_name    = 'R.packing.elem_2_float64'
THLA.manager.objects[2].attributes[2].elements[1].rti_encoding  = trick.ENCODING_LITTLE_ENDIAN
THLA.manager.objects[2].attributes[2].elements[2].rti_encoding  = trick.ENCODING_FIXED_RECORD
THLA.manager.objects[2].attributes[2].elements[2].element_count = 2
THLA.manager.objects[2].attributes[2].elements[2].elements      = trick.sim_services.alloc_type( THLA.manager.objects[2].attributes[2].elements[2].element_count, 'TrickHLA::RecordElement' )
THLA.manager.objects[2].attributes[2].elements[2].elements[0].trick_name   = 'R.packing.element_1_count'
THLA.manager.objects[2].attributes[2].elements[2].elements[0].rti_encoding = trick.ENCODING_LITTLE_ENDIAN
THLA.manager.objects[2].attributes[2].elements[2].elements[1].trick_name   = 'R.packing.element_2_name'
THLA.manager.objects[2].attributes[2].elements[2].elements[1].rti_encoding = trick.ENCODING_UNICODE_STRING
```



---
### Building the Simulation
In the SIM_sine_fixed_record directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_fixed_record directory:

```
./S_main_*.exe RUN_a_side/input.py
```

From a another console, in the SIM_sine_fixed_record directory:

```
./S_main_*.exe RUN_p_side/input.py
```
