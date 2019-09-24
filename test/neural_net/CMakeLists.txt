project(neural_net_test)

if(APPLE AND 
   HAS_MPS AND 
   NOT TC_BUILD_IOS AND 
   NOT TC_BASE_SDK_VERSION VERSION_LESS 10.15)
  
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
  
  make_library(style_transfer_test_utils
    SOURCES
      style_transfer_utils.mm
      utils.mm
    REQUIRES
      unity_shared_for_testing
  )

  target_compile_options(style_transfer_test_utils PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")

  make_boost_test(style_transfer.cxx
    REQUIRES
      style_transfer_test_utils
  )

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data)
  
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/encode)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/encode/test_1)
  
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/residual)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/residual/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/decode)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/decode/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/resnet)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/resnet/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/block1)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/block1/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/block2)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/block2/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/vgg16)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/vgg16/test_1)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/loss)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/loss/test_1)

  set(GOLDEN_SET_TESTING
    data/encode/test_1/config.json
    data/encode/test_1/inputs.json
    data/encode/test_1/outputs.json
    data/encode/test_1/weights.json

    data/residual/test_1/config.json
    data/residual/test_1/inputs.json
    data/residual/test_1/outputs.json
    data/residual/test_1/weights.json

    data/decode/test_1/config.json
    data/decode/test_1/inputs.json
    data/decode/test_1/outputs.json
    data/decode/test_1/weights.json

    data/resnet/test_1/config.json
    data/resnet/test_1/inputs.json
    data/resnet/test_1/outputs.json
    data/resnet/test_1/weights.json

    data/block1/test_1/config.json
    data/block1/test_1/inputs.json
    data/block1/test_1/outputs.json
    data/block1/test_1/weights.json

    data/block2/test_1/config.json
    data/block2/test_1/inputs.json
    data/block2/test_1/outputs.json
    data/block2/test_1/weights.json

    data/vgg16/test_1/config.json
    data/vgg16/test_1/inputs.json
    data/vgg16/test_1/outputs.json
    data/vgg16/test_1/weights.json
    
    data/loss/test_1/inputs.json
    data/loss/test_1/outputs.json
    data/loss/test_1/weights.json
  )
  
  foreach(SOURCE_FILE ${GOLDEN_SET_TESTING})
    string(
      REGEX REPLACE
      ".json$|.bin$"
      ".h"
      GENERATED_FILE
      ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_FILE}
    )

    add_custom_command(
      OUTPUT ${GENERATED_FILE}
      COMMAND xxd -i ${SOURCE_FILE} > ${GENERATED_FILE}
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating ${GENERATED_FILE}"
      VERBATIM
    )

    string(
      MAKE_C_IDENTIFIER
      ${SOURCE_FILE}
      TARGET_NAME
    )

    add_custom_target(
      Generate_${TARGET_NAME}
      DEPENDS
      ${GENERATED_FILE}
    )

    add_dependencies(style_transfer_test_utils Generate_${TARGET_NAME})
  endforeach(SOURCE_FILE)
endif()
