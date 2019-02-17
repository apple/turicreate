project(serialization_test)

make_boost_test(serializetests.cxx REQUIRES serialization)
make_boost_test(eigen_serialization.cxx REQUIRES serialization eigen)
