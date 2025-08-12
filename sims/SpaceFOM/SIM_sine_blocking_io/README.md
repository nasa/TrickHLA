# SIM_sine_blocking_io

SIM_sine_blocking_io is a simulation that solves a sine wave
using both analytic and propagated solution. The distributed simulation 
is comprised of two simulations. The A-side federate owns the analytic
sine state and the P-side federate owns the propagated sine state.
The example shows how to use and configure TrickHLA to send data using blocking I/O.

Blocking I/O allows non-timestamp order data to be sent and received
for a specified object instance name.

---
### Configuring the Simulation

#### S_define configuration

You will need to place jobs in your simulation to explicitly send and
receive the blocking I/O data for a given object instance name.
Because we use the same S_define file for both federates, is why you
are seeing both the send and receive blocking I/O jobs for both the
Analytic and Propagated sine simulation objects.


```
class BlockingIOAnalyticSineSimObj : public THLAFedSimObject {
 public:
   BlockingIOAnalyticSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...
      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_blocking_io_data( "A-side-Federate.Sine.blocking_io" );

      // Analytic sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_blocking_io_data( "A-side-Federate.Sine.blocking_io" );
   }
};
```

```
class BlockingIOPropagatedSineSimObj : public THLAFedSimObject {
 public:
   BlockingIOPropagatedSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...

      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_blocking_io_data( "P-side-Federate.Sine.blocking_io" );

      // Propagated sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_blocking_io_data( "P-side-Federate.Sine.blocking_io" );

      ...
   }
};
```

#### A-side federate blocking I/O input file configuration

**RUN_a_side_mpr/input.py:**

Configure all attributes of an object used for blocking I/O data 
transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
# Load the sine specific Sine object.
from TrickHLA_data.sine.SineObject import *

sine_AB = SineObject(
   sine_create_object      = True,
   sine_obj_instance_name  = 'A-side-Federate.Sine.blocking_io',
   sine_trick_sim_obj_name = 'AB',
   sine_packing            = AB.packing,
   sine_attribute_config   = trick.CONFIG_BLOCKING_IO )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AB )

sine_PB = SineObject(
   sine_create_object      = False,
   sine_obj_instance_name  = 'P-side-Federate.Sine.blocking_io',
   sine_trick_sim_obj_name = 'PB',
   sine_packing            = PB.packing,
   sine_attribute_config   = trick.CONFIG_BLOCKING_IO )

# Add this cyclic sine object to the list of managed objects.
federate.add_fed_object( sine_PB )
```

#### P-side federate blocking I/O input file configuration
**RUN_p_side/input.py:**

Configure all attributes of an object used for blocking I/O data transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
# Load the sine specific Sine object.
from TrickHLA_data.sine.SineObject import *

sine_AB = SineObject(
   sine_create_object      = False,
   sine_obj_instance_name  = 'A-side-Federate.Sine.blocking_io',
   sine_trick_sim_obj_name = 'AB',
   sine_packing            = AB.packing,
   sine_attribute_config   = trick.CONFIG_BLOCKING_IO )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AB )

sine_PB = SineObject(
   sine_create_object      = True,
   sine_obj_instance_name  = 'P-side-Federate.Sine.blocking_io',
   sine_trick_sim_obj_name = 'PB',
   sine_packing            = PB.packing,
   sine_attribute_config   = trick.CONFIG_BLOCKING_IO )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_PB )
```

---
### Building the Simulation
In the SIM_sine_blocking_io directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_blocking_io directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From a another console, in the SIM_sine_blocking_io directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
