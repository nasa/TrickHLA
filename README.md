# TrickHLA is an IEEE 1516 High Level Architecture (HLA) Simulation Interoperability Middleware for the Trick Simulation Environment

The TrickHLA software supports the IEEE-1516 High Level Architecture (HLA) 
simulation interoperability standard for the 
[Trick Simulation Environment](https://github.com/nasa/trick/). The TrickHLA 
software abstracts away the details of using HLA, allowing the user to concentrate 
on the simulation and not worry about having to be an HLA distributed simulation 
expert. The TrickHLA software is data driven and provides a simple Application 
Programming Interface (API) making it relatively easy to take an existing Trick 
simulation and make it an HLA distributed simulation.

## Student Work

This fork is just for fun to explore the code and find a way to keep moving
forward until we have RTI licenses!

## Installing, Building, and Running:

These instructions will walk you through the process of installing TrickHLA on 
your computer. Instructions are also provided for building an example two 
Federate simulation and then running it.

### Apple Silicon

Some exports to put in your .zshrc (exact values will depend on paths to repos on your machine, whether you've installed packages with Homebrew or something else, etc):

```
export PYTHON_VERSION=3
export RTI_HOME="$HOME/Developer/rtihome/prti1516e"
export RTI_VENDOR="Pitch_HLA_Evolved"
export TRICK_CXXFLAGS="-g -std=c++11 -fsanitize=address -I/opt/homebrew/include -L/opt/homebrew/lib -Wno-unused-command-line-argument"
export TRICK_CFLAGS="-g -fsanitize=address -I/opt/homebrew/include -L/opt/homebrew/lib -Wno-unused-command-line-argument"
export TRICK_LDFLAGS="-g -std=c++11 -fsanitize=address -L/opt/homebrew/lib"
export TRICK_ICGFLAGS=""
export TRICK_SWIG_FLAGS=""
export TRICK_EXCLUDE="/opt/homebrew"
export TRICK_HOME="$HOME/Developer/trick"
export TRICKHLA_HOME="$HOME/Developer/TrickHLA"
```

### Dependencies

TrickHLA requires the Trick Simulation Environment and a fully compliant 
IEEE 1516-2010 Runtime Infrastructure (RTI) in order to function. 

1) Install the Trick Simulation Environment by following the [Trick Install Guide](https://nasa.github.io/trick/documentation/install_guide/Install-Guide).

2) Install an IEEE 1516-2010 RTI using the installer and instructions provided by the vendor.

### Getting TrickHLA
Clone TrickHLA from the master branch.

git clone https://github.com/nasa/TrickHLA.git

### Environment Setup

1) Define $TRICK_HOME to point to where you installed Trick.

2) Add $TRICK_HOME/bin to your $PATH.

3) Define $TRICKHLA_HOME to point to the TrickHLA clone directory.

4) Define $RTI_HOME to point to your RTI install directory.

5) Define $RTI_VENDOR to be one of either <i>Pitch_HLA_Evolved</i> or <i>MAK_HLA_Evolved</i> depending on the RTI you are using.

6) Optionally, define $RTI_JAVA_HOME to override the Java Runtime Environment (JRE) used by the RTI if it uses one.

### Building an Example Simulation

1) cd $TRICKHLA_HOME/sims/SpaceFOM/SIM_sine

2) trick-CP

### Running an Example Simulation

The sine wave example simulation is a two federate distributed simulation that 
solves a sine wave using both an analytic and a propagated solution. 
The [SISO SpaceFOM International Standard](https://cdn.ymaws.com/www.sisostandards.org/resource/resmgr/standards_products/siso-std-018-2020_srfom.pdf) is used
in this example for execution control.

1) Make sure a fully compliant IEEE 1516-2010 RTI is running on the local host.

2) From a terminal, run the Analytic solution Federate:<br/>
./S_main*.exe RUN_a_side_mpr/input.py --verbose on

3) From a second terminal, run the Propagated solution Federate:<br/>
./S_main*.exe RUN_p_side/input.py --verbose on

---

## License:
TrickHLA is released under the [NASA Open Source Agreement Version 1.3](https://github.com/nasa/TrickHLA/blob/master/LICENSE.txt).

## Copyright:
Copyright 2020-2024 United States Government as represented by the Administrator of the National Aeronautics and Space Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.

## Disclaimers:
No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS, RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS RESULTING FROM USE OF THE SUBJECT SOFTWARE. FURTHER, GOVERNMENT AGENCY DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE, IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."

Waiver and Indemnity: RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT. IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM, RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW. RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE, UNILATERAL TERMINATION OF THIS AGREEMENT.

## Responsible Organization:
Simulation and Graphics Branch, Mail Code ER7  
Software, Robotics & Simulation Division  
NASA, Johnson Space Center  
2101 NASA Parkway, Houston, TX  77058  

