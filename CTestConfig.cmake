# Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
#
# See the LICENSE.txt file for license information. Please report all
# bugs and problems to the public mailing list <gmsh@geuz.org>.

set(CTEST_PROJECT_NAME "Gmsh")
set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE_CDASH TRUE)
set(CTEST_DROP_SITE "onelab.info")
set(CTEST_DROP_LOCATION "/CDash/submit.php?project=Gmsh")
set(CTEST_TRIGGER_SITE "")
set(SITE ${GMSH_HOST})
set(BUILDNAME "${GMSH_OS}-${GMSH_PACKAGER}")

set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS "1000")
set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS "1000")
