project(serialization_test)

make_boost_test(serializetests.cxx REQUIRES serialization numerics)
