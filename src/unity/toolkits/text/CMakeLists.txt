project(unity_toolkits)

add_library(unity_text OBJECT
  unity_text.cpp
  topic_model.cpp
  cgs.cpp
  alias.cpp
  perplexity.cpp
  class_registrations.cpp
)
add_dependencies(unity_text boost)