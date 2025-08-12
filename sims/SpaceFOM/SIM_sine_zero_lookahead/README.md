# SIM_sine_zero_lookahead

SIM_sine_zero_lookahead is a simulation that solves a sine wave using
both analytic and propagated solution. The distributed simulation is
comprised of two simulations. The A-side federate owns the analytic
sine state and the P-side federate owns the propagated sine state.
The example shows how to use and configure TrickHLA to send data using Zero-Lookahead.

Zero-Lookahead allows timestamp order data to be sent and received for 
the current granted HLA time. This does imply the moment when data is 
send and/or received is asynchronous and can happen any time within the 
granted HLA frame of time. To ensure a deterministic and repeatable 
simulation, TricHLA will use the zero-lookahead mechanism but will treat 
data exchanges as blocking I/O, but with timestamp ordered data.

---
### Configuring the Simulation

#### S_define configuration

You will need to place jobs in your simulation to explicitly send and 
receive the zero-lookahead data for a given object instance name. Because 
we use the same S_define file for both federates, is why you are seeing 
both the send and receive zero-lookahead jobs for both the Analytic and 
Propagated sine simulation objects.


```
class ZeroLookaheadAnalyticSineSimObj : public THLAFedSimObject {
 public:
   ZeroLookaheadAnalyticSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...
      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_zero_lookahead_data( "A-side-Federate.Sine.zero" );

      // Analytic sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_zero_lookahead_and_requested_data( "A-side-Federate.Sine.zero" );
   }
};
```

```
class ZeroLookaheadPropagatedSineSimObj : public THLAFedSimObject {
 public:
   ZeroLookaheadPropagatedSineSimObj( TrickHLA::Federate & thla_fed )
      : THLAFedSimObject( thla_fed )
   {
      ...
      // HLA Data will only be received if it is remotely owned by another federate.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.wait_to_receive_zero_lookahead_data( "P-side-Federate.Sine.zero" );

      // Propagated sine calculations...

      // HLA Data will only be sent if this federate owns it.
      (THLA_DATA_CYCLE_TIME, "scheduled") federate.send_zero_lookahead_and_requested_data( "P-side-Federate.Sine.zero" );
   }
};

```

#### A-side federate zero-lookahead input file configuration

**RUN_a_side_mpr/input.py:**

Configure all attributes of an object used for zero-lookahead data transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
# Load the sine specific Sine object.
from TrickHLA_data.sine.SineObject import *

sine_AZ = SineObject(
   sine_create_object      = True,
   sine_obj_instance_name  = 'A-side-Federate.Sine.zero',
   sine_trick_sim_obj_name = 'AZ',
   sine_packing            = AZ.packing,
   sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AZ )

sine_PZ = SineObject(
   sine_create_object      = False,
   sine_obj_instance_name  = 'P-side-Federate.Sine.zero',
   sine_trick_sim_obj_name = 'PZ',
   sine_packing            = PZ.packing,
   sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this cyclic sine object to the list of managed objects.
federate.add_fed_object( sine_PZ )
```

#### P-side federate zero-lookahead input file configuration
**RUN_p_side/input.py:**

Configure all attributes of an object used for zero-lookahead data 
transfers as trick.CONFIG_ZERO_LOOKAHEAD.

```
# Load the sine specific Sine object.
from TrickHLA_data.sine.SineObject import *

sine_AZ = SineObject(
   sine_create_object      = False,
   sine_obj_instance_name  = 'A-side-Federate.Sine.zero',
   sine_trick_sim_obj_name = 'AZ',
   sine_packing            = AZ.packing,
   sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_AZ )

sine_PZ = SineObject(
   sine_create_object      = True,
   sine_obj_instance_name  = 'P-side-Federate.Sine.zero',
   sine_trick_sim_obj_name = 'PZ',
   sine_packing            = PZ.packing,
   sine_attribute_config   = trick.CONFIG_ZERO_LOOKAHEAD )

# Add this sine object to the list of managed objects.
federate.add_fed_object( sine_PZ )
```

---
### Building the Simulation
In the SIM_sine_zero_lookahead directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_zero_lookahead directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From a another console, in the SIM_sine_zero_lookahead directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
