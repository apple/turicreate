#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <core/util/test_macros.hpp>
#include <string>
#include <core/storage/fileio/temp_files.hpp>

using namespace turi;


struct temp_file_test  {

 public:

  void test_temp_file() {
    // give me 3 temp names
    std::string filea = get_temp_name();
    std::string fileb = get_temp_name();
    std::string filec = get_temp_name();

    // create file a. it will just be the file itself
    { 
      std::ofstream fout(filea.c_str()); fout.close();
      TS_ASSERT(delete_temp_file(filea));
      // check that filea is gone
      std::ifstream fin(filea.c_str());
      TS_ASSERT(fin.fail());
      // repeated deletion fails
      TS_ASSERT(delete_temp_file(filea) == false);
    }


    // create file b. it will have on prefix
    {
      fileb = fileb + ".cogito";
      std::ofstream fout(fileb.c_str()); fout.close();
      delete_temp_file(fileb);
      // check that fileb is gone
      std::ifstream fin(fileb.c_str());
      TS_ASSERT(fin.fail());
      // repeated deletion fails
      TS_ASSERT(delete_temp_file(fileb) == false);
    } 

    // file c is a lot of prefixes. and tests that we can delete a bunch of 
    // stuff
    {
      std::vector<std::string> filecnames{filec + "pika",
        filec + ".chickpeas",
        filec + ".gyro",
        filec + ".salamander"};
      for(std::string f : filecnames) {
        std::ofstream fout(f.c_str()); 
        fout.close();
      }
      delete_temp_files(filecnames);
      // check that they are all gone
      for(std::string f : filecnames) {
        std::ifstream fin(f.c_str()); 
        TS_ASSERT(fin.fail());
        // repeated deletion fails
        TS_ASSERT(delete_temp_file(f) == false);
      }
    }
  }
};


BOOST_FIXTURE_TEST_SUITE(_temp_file_test, temp_file_test)
BOOST_AUTO_TEST_CASE(test_temp_file) {
  temp_file_test::test_temp_file();
}
BOOST_AUTO_TEST_SUITE_END()
