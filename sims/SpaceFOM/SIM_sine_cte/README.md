# SIM_sine_cte

SIM_sine_cte is a simulation that solves a sine wave using both
analytic and propagated solution. The distributed simulation 
is comprised of two simulations. The A-side federate owns the
analytic sine state and the P-side federate owns the propagated
sine state. The example shows how to use and configure TrickHLA to
use Central Timing Equipment (CTE).

---
### Configuring the Simulation

#### S_define configuration

Adding a CTE implementation to your simulation. The example shows
two different CTE implementations. TSync is a hardware based CTE
implementation and the other uses the system clock that is
synchronized by way of Network Time Protocol (NTP) or Precision
Time Protocol (PTP).


```
// Should the TSync CTE card be used as an external Trick clock.
#define NO_TSYNC_CTE

#ifdef TSYNC_CTE
##include "TrickHLA/TSyncCTETimeline.hh"
#else
##include "TrickHLA/TimeOfDayCTETimeline.hh"
#endif

...

class THLAInitSimObject : public Trick::SimObject {
 public:
#ifdef TSYNC_CTE
   TrickHLA::TSyncCTETimeline       cte_timeline;
#else
   TrickHLA::TimeOfDayCTETimeline   cte_timeline;
#endif
   }
};

```


#### A-side federate input file configuration

**RUN_a_side_mpr/input.py:**

Configure CTE timeline.

```
# Set the CTE timeline and change the Trick real time clock to use it.
THLA.execution_control.cte_timeline = THLA_INIT.cte_timeline
```

#### P-side federate input file configuration
**RUN_p_side/input.py:**

Configure CTE timeline.

```
# Set the CTE timeline and change the Trick real time clock to use it.
THLA.execution_control.cte_timeline = THLA_INIT.cte_timeline
```

---
### Building the Simulation
In the SIM_sine_cte directory, type **trick-CP** to build the simulation executable. When it's complete, you should see:

```
Trick Build Process Complete
```

---
### Running the Simulation
In the SIM_sine_cte directory:

```
./S_main_*.exe RUN_a_side_mpr/input.py --verbose on
```

From another terminal, in the SIM_sine_cte directory:

```
./S_main_*.exe RUN_p_side/input.py --verbose on
```
