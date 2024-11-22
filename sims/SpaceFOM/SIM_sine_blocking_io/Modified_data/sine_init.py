##############################################################################
# PURPOSE:
#    (This is an input file python routine to setup the initial values of the
#     sine state.)
#
# REFERENCE:
#    (Trick 10 documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Assumes that trick is available globally.))
#
# PROGRAMMERS:
#    (((Dan Dexter) (NASA) (May 2013) (--) (Initial implementation.)))
##############################################################################

# Analytic sine data
AB.sim_data.time  = 0.0
AB.sim_data.value = 0.0
AB.sim_data.dvdt  = 0.392699
AB.sim_data.amp   = 2.0
AB.sim_data.phase = 0.0
AB.sim_data.freq  = 0.1963495

# Analytic sine truth state
AB.truth_data.time  = 0.0
AB.truth_data.value = 0.0
AB.truth_data.dvdt  = 0.392699
AB.truth_data.amp   = 2.0
AB.truth_data.phase = 0.0
AB.truth_data.freq  = 0.1963495

# Propagated sine data
PB.sim_data.time  = 0.0
PB.sim_data.value = 0.0
PB.sim_data.dvdt  = 0.392699
PB.sim_data.amp   = 1.0
PB.sim_data.phase = 0.0
PB.sim_data.freq  = 0.392699

# Propagated sine truth state
PB.truth_data.time  = 0.0
PB.truth_data.value = 0.0
PB.truth_data.dvdt  = 0.392699
PB.truth_data.amp   = 1.0
PB.truth_data.phase = 0.0
PB.truth_data.freq  = 0.392699


# Analytic cyclic sine data
AC.sim_data.time  = 0.0
AC.sim_data.value = 0.0
AC.sim_data.dvdt  = 0.392699
AC.sim_data.amp   = 2.0
AC.sim_data.phase = 0.0
AC.sim_data.freq  = 0.1963495

# Analytic cyclic sine truth state
AC.truth_data.time  = 0.0
AC.truth_data.value = 0.0
AC.truth_data.dvdt  = 0.392699
AC.truth_data.amp   = 2.0
AC.truth_data.phase = 0.0
AC.truth_data.freq  = 0.1963495

# Propagated cyclic sine data
PC.sim_data.time  = 0.0
PC.sim_data.value = 0.0
PC.sim_data.dvdt  = 0.392699
PC.sim_data.amp   = 1.0
PC.sim_data.phase = 0.0
PC.sim_data.freq  = 0.392699

# Propagated cyclic sine truth state
PC.truth_data.time  = 0.0
PC.truth_data.value = 0.0
PC.truth_data.dvdt  = 0.392699
PC.truth_data.amp   = 1.0
PC.truth_data.phase = 0.0
PC.truth_data.freq  = 0.392699
