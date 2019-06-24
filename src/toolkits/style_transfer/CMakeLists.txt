project(unity_toolkits)

make_library(unity_style_transfer OBJECT
    SOURCES
     	style_transfer.m
    REQUIRES
        unity_core
        unity_ml_model
)
