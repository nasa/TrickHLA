############################################################################
# @file trickhla_models.py
# @brief Defines the list of active models in TrickHLA.
#
# This is a Python module provides lists of the active models in the
# TrickHLA include and source directories.
############################################################################
# @revs_title
# @revs_begin
# @rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, April 2019, --, Initial creation.}
# @revs_end
############################################################################

# Create dictionary of registered TrickHLA models
trickhla_models_includes = {
   'TrickHLA':'include/TrickHLA',
   'SpaceFOM':'include/SpaceFOM',
   'JEOD':'include/JEOD',
   'IMSim':'include/IMSim'
}
trickhla_models_source = {
   'TrickHLA':'source/TrickHLA',
   'SpaceFOM':'source/SpaceFOM',
   'JEOD':'source/JEOD',
   'IMSim':'source/IMSim'
}
