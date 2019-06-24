project(unity_toolkits)

make_library(unity_style_transfer OBJECT
    SOURCES
     	style_transfer.m
     	sub_layers/decoding.m
     	sub_layers/encoding.m
     	sub_layers/residual.m
    REQUIRES
        unity_core
        unity_ml_model
)
