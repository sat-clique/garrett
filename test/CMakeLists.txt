file(GLOB_RECURSE problem_instances "instances/*")

foreach (instance IN LISTS problem_instances)
  get_filename_component(instance_file "${instance}" NAME)
  add_test(NAME "validity-${instance_file}" COMMAND garrett "${instance}")
  set_tests_properties("validity-${instance_file}" PROPERTIES PASS_REGULAR_EXPRESSION "valid: 1")
endforeach()
