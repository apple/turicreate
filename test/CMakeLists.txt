project(TuriTest)

enable_testing()

subdirs(
        toolkits
        ml_data
        optimization
        user_pagefault
        util
        serialization
        logger
        timer
        random
        # fault
        nanosockets
        fileio
        shmipc
        cppipc
        flexible_type
        parallel
        lambda
        sframe
        sgraph
        lazy_eval
        sframe_query_engine
        unity
        sketches
        generics
        table_printer
        network
        process
  )
if(${TC_BUILD_CAPI}) 
  message("Building C API Tests.")
  subdirs(capi)
else()
  message("Skipping build of C APIs Tests.")
endif()

