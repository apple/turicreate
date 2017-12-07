cmake_host_system_information
-----------------------------

Query host system specific information.

::

  cmake_host_system_information(RESULT <variable> QUERY <key> ...)

Queries system information of the host system on which cmake runs.
One or more ``<key>`` can be provided to select the information to be
queried.  The list of queried values is stored in ``<variable>``.

``<key>`` can be one of the following values:

::

  NUMBER_OF_LOGICAL_CORES   = Number of logical cores.
  NUMBER_OF_PHYSICAL_CORES  = Number of physical cores.
  HOSTNAME                  = Hostname.
  FQDN                      = Fully qualified domain name.
  TOTAL_VIRTUAL_MEMORY      = Total virtual memory in megabytes.
  AVAILABLE_VIRTUAL_MEMORY  = Available virtual memory in megabytes.
  TOTAL_PHYSICAL_MEMORY     = Total physical memory in megabytes.
  AVAILABLE_PHYSICAL_MEMORY = Available physical memory in megabytes.
