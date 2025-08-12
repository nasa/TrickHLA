# SIM_sine_fixed_record

SIM_sine_fixed_record is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation is comprised
of two simulations. The A-side federate owns the analytic sine state and
the P-side federate owns the propagated sine state. The example shows how
to use and configure fixed record data.

---
### Configuring the Simulation

#### A-side federate fixed record configuration

From file RUN_a_side_mpr/input.py:

```
# Load the FixedRecordTest specific object.
from TrickHLA_data.FixedRecTest.FixedRecordTestObject import *
...
# Set up for FixedRecordTest data.
fixed_rec_R = FixedRecordTestObject(
   fixed_rec_create_object      = True,
   fixed_rec_obj_instance_name  = 'A-side-Federate.FixedRecord',
   fixed_rec_trick_sim_obj_name = 'R',
   fixed_rec_packing            = R.packing )

# Add this fixed-record test object to the list of managed objects.
federate.add_fed_object( fixed_rec_R )
```

#### P-side federate fixed record configuration

From file RUN_p_side/input.py:

```
# Load the FixedRecordTest specific object.
from TrickHLA_data.FixedRecTest.FixedRecordTestObject import *
...
# Set up for FixedRecordTest data.
fixed_rec_R = FixedRecordTestObject(
   fixed_rec_create_object      = False,
   fixed_rec_obj_instance_name  = 'A-side-Federate.FixedRecord',
   fixed_rec_trick_sim_obj_name = 'R',
   fixed_rec_packing            = R.packing )

# Add this fixed-record test object to the list of managed objects.
federate.add_fed_object( fixed_rec_R )
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
./S_main_*.exe RUN_a_side_mpr/input.py  --verbose on
```

From a another console, in the SIM_sine_fixed_record directory:

```
./S_main_*.exe RUN_p_side/input.py  --verbose on
```
