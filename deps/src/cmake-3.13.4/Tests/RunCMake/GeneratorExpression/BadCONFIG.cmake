add_custom_target(check ALL COMMAND check
  $<CONFIG:.>
  $<CONFIG:Foo,Bar>
  $<CONFIG:Foo-Bar>
  $<$<CONFIG:Foo-Nested>:foo>
  VERBATIM)
