#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sarray_v2_encoded_block.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/fileio/temp_files.hpp>

using namespace turi;

struct sarray_test {
 public:
  void test_sarray_basic(void) {
    /*
     * Simple writes of 4 arrays of length 5 each
     */
    std::vector<std::vector<size_t> > data{{1,2,3,4,5},
                                           {6,7,8,9,10},
                                           {11,12,13,14,15},
                                           {16,17,18,19,20}};
    std::string test_prefix = get_temp_name();
    std::string test_file_name = test_prefix + ".sidx";
    std::cerr << "test file name: " << test_file_name << std::endl;
    sarray<size_t> array;
    array.open_for_write(test_file_name, 4);

    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for(auto val: data[i]) {
        *iter = val;
        ++iter;
      }
    }

    array.set_metadata(std::string("type"), std::string("int"));
    // check the get_file_prefix and get_file_names functions
    TS_ASSERT_EQUALS(array.get_index_file(), test_file_name);
    std::vector<std::string> files = array.get_index_info().segment_files;
    std::set<std::string> fileset;
    std::copy(files.begin(), files.end(), std::inserter(fileset, fileset.end()));
    TS_ASSERT_EQUALS(fileset.size(), 4);
    array.close();


    auto reader = array.get_reader();
    TS_ASSERT_EQUALS(reader->num_segments(), 4);
    // read the data we wrote the last time
    for (size_t i = 0; i < 4; ++i) {
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      for(auto val: data[i]) {
        TS_ASSERT_EQUALS(val, *begin);
        TS_ASSERT(begin != end);
        ++begin;
      }
      TS_ASSERT(begin == end);
    }

    // random read
    std::vector<size_t> ret;
    size_t len = reader->read_rows(6,13, ret);
    TS_ASSERT_EQUALS(len, ret.size());
    TS_ASSERT_EQUALS(len, 13 - 6);
    for (size_t i = 0;i < ret.size(); ++i) {
      TS_ASSERT_EQUALS(ret[i], 7 + i);
    }

    std::string read_type;
    TS_ASSERT(reader->get_metadata(std::string("type"), read_type));
    TS_ASSERT_EQUALS(read_type, std::string("int"));

    // serialization
    {
      std::string dirpath = "sarray_test_dir";
      turi::dir_archive dir;
      dir.open_directory_for_write(dirpath);
      turi::oarchive oarc(dir);
      oarc << array;
    }

     {
       // Load sarray back and check that the contents are right
       std::string dirpath = "sarray_test_dir";
       turi::dir_archive dir;
       dir.open_directory_for_read(dirpath);
       sarray<size_t> array2;
       turi::iarchive iarc(dir);
       iarc >> array2;
       auto reader = array2.get_reader();
       // read the data we wrote the last time
       std::vector<size_t> flattened_data; 
       for (size_t i = 0;i < data.size(); ++i) {
         flattened_data.insert(flattened_data.end(),
                               data[i].begin(), data[i].end());
       }
       size_t dataiter = 0;
       for (size_t i = 0; i < reader->num_segments(); ++i) {
         auto begin = reader->begin(i);
         auto end = reader->end(i);
         while (begin != end) {
           TS_ASSERT_EQUALS(flattened_data[dataiter], *begin);
           ++dataiter;
           ++begin;
         }
         TS_ASSERT(begin == end);
       }
     }
     turi::fileio::delete_path_recursive("sarray_test_dir");
  }

  void test_sarray_more_interesting(void) {
    /*
     * Simple writes of 3 arrays of variable length, some have empty length
     * That this goes after test_sarray_basic also sees what happens when
     * we change the number of files in the sarray
     */

    std::vector<std::vector<size_t> > data{{1,2,3,4,5,6,7,8},
                                           {},
                                           {9,10,11,12,13,14,15}};

    sarray<size_t> array;
    array.open_for_write(3);

    TS_ASSERT_EQUALS(array.num_segments(), 3);
    for (size_t i = 0;i < 3; ++i) {
      auto iter = array.get_output_iterator(i);
      for(auto val: data[i]) {
        *iter = val;
        ++iter;
      }
    }
    array.close();

    // now see if I can read it
    auto reader = array.get_reader();
    TS_ASSERT_EQUALS(reader->num_segments(), 3);
    // read the data we wrote the last time
    for (size_t i = 0; i < 3; ++i) {
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      for(auto val: data[i]) {
        TS_ASSERT_EQUALS(val, *begin);
        TS_ASSERT(begin != end);
        ++begin;
      }
      TS_ASSERT(begin == end);
    }


    // random read
    std::vector<size_t> ret;
    size_t len = reader->read_rows(6,13, ret);
    TS_ASSERT_EQUALS(len, ret.size());
    TS_ASSERT_EQUALS(len, 13 - 6);
    for (size_t i = 0;i < ret.size(); ++i) {
      TS_ASSERT_EQUALS(ret[i], 7 + i);
    }

  }


  void test_sarray_strings(void) {
    /*
     * Write some strings
     */
    std::vector<std::vector<std::string> > data{{"hello","world"},
                                           {"my","name","is","yucheng"},
                                           {},
                                           {"previous","one","is","empty"}};

    sarray<std::string> array;
    array.open_for_write(4);
    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for(auto val: data[i]) {
        *iter = val;
        ++iter;
      }
    }
    array.close();

    // now see if I can read it
    auto reader = array.get_reader();

    TS_ASSERT_EQUALS(reader->num_segments(), 4);
    // read the data we wrote the last time
    for (size_t i = 0; i < 4; ++i) {
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      for(auto val: data[i]) {
        TS_ASSERT_EQUALS(val, *begin);
        TS_ASSERT(begin != end);
        ++begin;
      }
      TS_ASSERT(begin == end);
    }
  }



  void test_sarray_transform(void) {
    //  Simple writes of 4 arrays of length 5 each
    // construct initial sarray
    std::vector<std::vector<size_t> > data{{1,2,3,4,5},
                                           {6,7,8,9,10},
                                           {11,12,13,14,15},
                                           {16,17,18,19,20}};
    size_t num_segments = data.size();
    sarray<size_t> array;
    array.open_for_write(num_segments);
    TS_ASSERT_EQUALS(array.num_segments(), num_segments);
    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for(auto val: data[i]) {
        *iter = val;
        ++iter;
      }
    }
    array.close();


    sarray<size_t> array_times_3;
    array_times_3.open_for_write(num_segments);
    sarray<size_t> array_times_3_mod_2;
    array_times_3_mod_2.open_for_write(num_segments);

    turi::transform(array, array_times_3,
                        [](size_t i) { return i * 3; });
    array_times_3.close();
    // filters to even
    turi::copy_if(array_times_3, array_times_3_mod_2,
                      [](size_t i) { return i % 2 == 0; });
    array_times_3_mod_2.close();


    // perform the same operation on the in memory data
    
    std::vector<std::vector<size_t> > data2(num_segments);
    for (size_t i = 0;i < 4; ++i) {
      std::transform(data[i].begin(), data[i].end(),
                     std::inserter(data2[i], data2[i].end()),
                     [](size_t i) { return i * 3; });
    }

    std::vector<std::vector<size_t> > data3(num_segments);
    for (size_t i = 0;i < 4; ++i) {
      std::copy_if(data2[i].begin(), data2[i].end(),
                   std::inserter(data3[i], data3[i].end()),
                   [](size_t i) { return i % 2 == 0; });
    }


    TS_ASSERT_EQUALS(array_times_3_mod_2.num_segments(), num_segments);
    auto reader = array_times_3_mod_2.get_reader();
    for (size_t i = 0; i < num_segments; ++i) {
      auto begin = reader->begin(i);
      for(auto val: data3[i]) {
        TS_ASSERT_EQUALS(val, *begin);
        ++begin;
      }
    }
  }

  void test_sarray_copy(void) {
    // construct initial sarray
    std::vector<size_t> data{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    sarray<size_t> array;
    array.open_for_write(4);
    turi::copy(data.begin(), data.end(), array);
    array.close();

    // check that the array has the correct values
    TS_ASSERT_EQUALS(array.num_segments(), 4);
    auto reader = array.get_reader();
    for (size_t i = 0; i < 4; ++i) {
      auto begin = reader->begin(i);
      for(size_t j = 0;j < 4; ++j) {
        TS_ASSERT_EQUALS(i * 4 + j, *begin);
        ++begin;
      }
    }
    std::vector<size_t> newdata;
    turi::copy(array, std::inserter(newdata, newdata.end()));

    TS_ASSERT_EQUALS(data.size(), newdata.size());
    for (size_t i = 0;i < data.size(); ++i) {
      TS_ASSERT_EQUALS(data[i], newdata[i]);
    }
  }


  void test_sarray_flexible_type_strings(void) {
     // Write some strings
    std::vector<std::vector<std::string> > data{{"hello","world"},
                                           {"my","name","is","yucheng"},
                                           {},
                                           {"previous","one","is","empty"}};

    sarray<flexible_type> array;
    array.open_for_write(4);
    array.set_type(turi::flex_type_enum::STRING);
    TS_ASSERT_EQUALS(array.num_segments(), 4);
    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for(auto val: data[i]) {
        *iter = val;
        ++iter;
      }
    }
    array.close();

    // now see if I can read it
    TS_ASSERT_EQUALS(array.num_segments(), 4);
    TS_ASSERT_EQUALS(array.get_type(), turi::flex_type_enum::STRING);
    // read the data we wrote the last time
    auto reader = array.get_reader();
    for (size_t i = 0; i < 4; ++i) {
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      for(auto val: data[i]) {
        auto sarray_val = *begin;
        TS_ASSERT_EQUALS(sarray_val.get_type(), turi::flex_type_enum::STRING);
        TS_ASSERT_EQUALS(val, sarray_val.get<flex_string>());
        TS_ASSERT(begin != end);
        ++begin;
      }
      TS_ASSERT(begin == end);
    }
  }

  void test_sarray_save() {
     // Write some strings
    std::vector<std::vector<std::string> > data{{"hello","world"},
                                           {"a","b","c","d"},
                                           {},
                                           {"previous","one","is","empty"}};
    std::vector<std::string> flattened_data;
    for(auto d: data) flattened_data.insert(flattened_data.end(), d.begin(), d.end());
    // write a flexible_type sarray
    { 
      sarray<flexible_type> array;
      array.open_for_write(4);
      array.set_type(turi::flex_type_enum::STRING);
      TS_ASSERT_EQUALS(array.num_segments(), 4);
      for (size_t i = 0;i < 4; ++i) {
        auto iter = array.get_output_iterator(i);
        for(auto val: data[i]) {
          *iter = val;
          ++iter;
        }
      }
      array.close();
      std::string index_file = get_temp_name();
      index_file += ".sidx";
      array.save(index_file);

      sarray<flexible_type> newarray(index_file);
      TS_ASSERT_EQUALS(newarray.size(), 10);
      TS_ASSERT_EQUALS(newarray.get_type(), turi::flex_type_enum::STRING);
      auto reader = newarray.get_reader();
      std::vector<flexible_type> rows;
      reader->read_rows(0, 10, rows);
      for (size_t i = 0;i < flattened_data.size(); ++i) {
        TS_ASSERT_EQUALS(flattened_data[i], (std::string)rows[i]);
      }
    }

    // write again to a string array
    { 
      sarray<std::string> array;
      array.open_for_write(4);
      TS_ASSERT_EQUALS(array.num_segments(), 4);
      for (size_t i = 0;i < 4; ++i) {
        auto iter = array.get_output_iterator(i);
        for(auto val: data[i]) {
          *iter = val;
          ++iter;
        }
      }
      array.close();
      std::string index_file = get_temp_name();
      index_file += ".sidx";
      array.save(index_file);

      sarray<std::string> newarray(index_file);
      TS_ASSERT_EQUALS(newarray.size(), 10);
      auto reader = newarray.get_reader();
      std::vector<std::string> rows;
      reader->read_rows(0, 10, rows);
      for (size_t i = 0;i < flattened_data.size(); ++i) {
        TS_ASSERT_EQUALS(flattened_data[i], (std::string)rows[i]);
      }
    }
  }

  void test_sarray_save_empty() {
    // flexible_type array
    { 
      sarray<flexible_type> array;
      array.open_for_write(4);
      array.set_type(turi::flex_type_enum::STRING);
      array.close();

      std::string index_file = get_temp_name();
      index_file += ".sidx";
      array.save(index_file);

      sarray<flexible_type> newarray(index_file);
      TS_ASSERT_EQUALS(newarray.size(), 0);
    }

    // string array
    { 
      sarray<std::string> array;
      array.open_for_write(4);
      array.close();

      std::string index_file = get_temp_name();
      index_file += ".sidx";
      array.save(index_file);

      sarray<std::string> newarray(index_file);
      TS_ASSERT_EQUALS(newarray.size(), 0);
    }
  }

  void test_sarray_append(void) {
    sarray<size_t> array_out;
    std::vector<size_t> data{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    {
      // construct initial sarray
      sarray<size_t> array, array2;
      array.open_for_write(4);
      turi::copy(data.begin(), data.end(), array);
      array.close();
      array2 = array;

      array_out = array_out.append(array);
      TS_ASSERT_EQUALS(array_out.num_segments(), 4);
      TS_ASSERT_EQUALS(array_out.size(), data.size());

      // validate state of array_out
      std::vector<size_t> newdata;
      turi::copy(array_out, std::inserter(newdata, newdata.end()));
      TS_ASSERT_EQUALS(newdata.size(), data.size());
      for (size_t i = 0;i < data.size(); ++i) {
        TS_ASSERT_EQUALS(data[i], newdata[i]);
      }
      
      // copy array2 into array_out
      array_out = array_out.append(array);
      TS_ASSERT_EQUALS(array_out.num_segments(), 8);
      TS_ASSERT_EQUALS(array_out.size(), 2 * data.size());

      // validate state array_out
      newdata.clear();
      turi::copy(array_out, std::inserter(newdata, newdata.end()));
      TS_ASSERT_EQUALS(newdata.size(), 2 * data.size());
      for (size_t i = 0;i < newdata.size(); ++i) {
        TS_ASSERT_EQUALS(data[i % data.size()], newdata[i]);
      }
      // validate state of array and array2
      newdata.clear();
      turi::copy(array, std::inserter(newdata, newdata.end()));
      TS_ASSERT_EQUALS(newdata.size(), data.size());
      for (size_t i = 0;i < newdata.size(); ++i) {
        TS_ASSERT_EQUALS(data[i % data.size()], newdata[i]);
      }
      // validate state of array and array2
      newdata.clear();
      turi::copy(array2, std::inserter(newdata, newdata.end()));
      TS_ASSERT_EQUALS(newdata.size(), data.size());
      for (size_t i = 0;i < newdata.size(); ++i) {
        TS_ASSERT_EQUALS(data[i % data.size()], newdata[i]);
      }
    }
    // make sure I can still access array_out after destruction of 
    // array and array2
    std::vector<size_t> newdata;
    turi::copy(array_out, std::inserter(newdata, newdata.end()));
    TS_ASSERT_EQUALS(newdata.size(), 2 * data.size());
    for (size_t i = 0;i < newdata.size(); ++i) {
      TS_ASSERT_EQUALS(data[i % data.size()], newdata[i]);
    }
  }

  void test_sarray_small_append(void) {
    std::vector<flexible_type> data{flexible_type(1.0)};
    sarray<flexible_type> array;
    array.open_for_write(4);
    turi::copy(data.begin(), data.end(), array);
    array.close();

    sarray<flexible_type> array2 = array.append(array);
    auto reader = array2.get_reader();
    std::vector<flexible_type> rval;
    reader->read_rows(0, 2, rval);
    TS_ASSERT_EQUALS(rval.size(), 2);
    TS_ASSERT_EQUALS(rval[0], data[0]);
    TS_ASSERT_EQUALS(rval[1], data[0]);
  }

  void test_sarray_many_small_append(void) {
    std::vector<flexible_type> data{flexible_type(1.0)};
    sarray<flexible_type> array;
    array.open_for_write(1);
    turi::copy(data.begin(), data.end(), array);
    array.close();

    sarray<flexible_type> array2 = array.append(array);
    for (size_t i = 0;i < 510; ++i) {
      array2 = array2.append(array);
    }
    auto reader = array2.get_reader();
    std::vector<flexible_type> rval;
    reader->read_rows(0, 512, rval);
    TS_ASSERT_EQUALS(array2.size(), 512);
    TS_ASSERT_EQUALS(rval.size(), 512);
    for (size_t i = 0;i < 512; ++i) {
      TS_ASSERT_EQUALS(rval[i], data[0]);
    }
    TS_ASSERT(array2.get_index_info().segment_files.size() <= 256);

    // try it again changing the compaction threshold so that we trigger
    // the slow compact route
    auto OLD_SFRAME_COMPACTION_THRESHOLD = SFRAME_COMPACTION_THRESHOLD; 
    auto OLD_FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT = FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT;
    for (size_t i = 0;i < 512; ++i) {
      array2 = array2.append(array);
    }
    TS_ASSERT_EQUALS(array2.size(), 1024);
    reader = array2.get_reader();
    reader->read_rows(0, 1024, rval);
    TS_ASSERT_EQUALS(rval.size(), 1024);
    for (size_t i = 0;i < 1024; ++i) {
      TS_ASSERT_EQUALS(rval[i], data[0]);
    }
    TS_ASSERT(array2.get_index_info().segment_files.size() <= SFRAME_COMPACTION_THRESHOLD);

    SFRAME_COMPACTION_THRESHOLD = OLD_SFRAME_COMPACTION_THRESHOLD; 
    FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT = OLD_FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT;
  }

  void test_sarray_recursive_append(void) {
    std::vector<flexible_type> data{flexible_type(1.0)};
    sarray<flexible_type> array;
    array.open_for_write(1);
    turi::copy(data.begin(), data.end(), array);
    array.close();

    for (size_t i = 0;i < 20; ++i) {
      array = array.append(array);
    }
    TS_ASSERT_EQUALS(array.size(), 1048576);
    TS_ASSERT(array.get_index_info().segment_files.size() <= SFRAME_COMPACTION_THRESHOLD);
    auto reader = array.get_reader();
    std::vector<flexible_type> rval;
    reader->read_rows(0, 1048576, rval);
    TS_ASSERT_EQUALS(rval.size(), 1048576);
    for (size_t i = 0;i < 1048576; ++i) {
      TS_ASSERT_EQUALS(rval[i], data[0]);
    }

  }
  void validate_test_sarray_logical_segments(std::unique_ptr<sarray_reader<size_t> > reader,
                                             size_t nsegments) {
    TS_ASSERT_EQUALS(reader->num_segments(), nsegments);
    // read the data we wrote the last time
    std::vector<size_t> outdata;
    for (size_t i = 0; i < nsegments; ++i) {
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      while(begin != end){
        outdata.push_back(*begin);
        ++begin;
      }
    }
    for (size_t i = 0;i < outdata.size(); ++i) {
      TS_ASSERT_EQUALS(outdata[i], i);
    }
  }

  void test_sarray_logical_segments(void) {
    // test the logical segmentation system.
    sarray<size_t> array_out;
    std::vector<size_t> data{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    // construct initial sarray
    sarray<size_t> array;
    array.open_for_write(4);
    turi::copy(data.begin(), data.end(), array);
    array.close();

    validate_test_sarray_logical_segments(array.get_reader(1), 1); // 1 segment
    validate_test_sarray_logical_segments(array.get_reader(8), 8); // 8 segments
    validate_test_sarray_logical_segments(array.get_reader(200), 200); // 200 segments

    // custom segment lengths
    std::vector<size_t> custom_sizes{3,0,5,8};
    auto reader = array.get_reader(custom_sizes);
    TS_ASSERT_EQUALS(reader->num_segments(), 4);
    for(size_t i = 0;i < custom_sizes.size(); ++i) {
      TS_ASSERT_EQUALS(reader->segment_length(i), custom_sizes[i]);
    }
    validate_test_sarray_logical_segments(array.get_reader(custom_sizes), 4); // 200 segments

  }

  void test_sarray_v2_encoded_block(void) {
    // write the initial sarray
    std::string test_prefix = get_temp_name();
    std::string test_file_name = test_prefix + ".sidx";
    sarray<flexible_type> array;
    array.open_for_write(test_file_name, 4);
    array.set_type(flex_type_enum::INTEGER);

    int ctr = 0;
    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for (size_t val = 0; val < 10000; ++val) {
        *iter = ctr;
        ++ctr;
        ++iter;
      }
    }
    array.set_metadata(std::string("type"), std::string("int"));
    array.close();
    auto& bm = turi::v2_block_impl::block_manager::get_instance();
    ctr = 0;
    for (auto segfiles: array.get_index_info().segment_files) {
      auto coladdress = bm.open_column(segfiles);
      for (size_t i = 0;i < bm.num_blocks_in_column(coladdress); ++i) {
        turi::v2_block_impl::block_address addr;
        std::get<0>(addr) = std::get<0>(coladdress);
        std::get<1>(addr) = std::get<1>(coladdress);
        std::get<2>(addr) = i;
        auto binfo = bm.get_block_info(addr);
        auto block_contents = bm.read_block(addr);
        v2_block_impl::encoded_block eblock(binfo, std::move(*block_contents));
        auto range = eblock.get_range();
        std::vector<flexible_type> values;
        values.resize(eblock.size());
        range.decode_to(&values[0], values.size());
        for (auto val: values) {
          TS_ASSERT_EQUALS(val.get_type(), flex_type_enum::INTEGER);
          TS_ASSERT_EQUALS(val.get<flex_int>(), ctr);
          ++ctr;
        }
      }
    }
    TS_ASSERT_EQUALS(ctr, 10000 * 4);
  }



  void test_sarray_sframe_rows(void) {
    // write the initial sarray
    std::string test_prefix = get_temp_name();
    std::string test_file_name = test_prefix + ".sidx";
    sarray<flexible_type> array;
    array.open_for_write(test_file_name, 4);
    array.set_type(flex_type_enum::INTEGER);

    int ctr = 0;
    for (size_t i = 0;i < 4; ++i) {
      auto iter = array.get_output_iterator(i);
      for (size_t val = 0; val < 10000; ++val) {
        *iter = ctr;
        ++ctr;
        ++iter;
      }
    }
    array.set_metadata(std::string("type"), std::string("int"));
    array.close();
    auto reader = array.get_reader(1);
    sframe_rows rows;
    ctr = 0;
    for (size_t i = 0;i < reader->size(); i += 256) {
      size_t rend = std::min(i + 256, reader->size());
      reader->read_rows(i, rend, rows);
      TS_ASSERT_EQUALS(rows.num_rows(), rend - i);
      TS_ASSERT_EQUALS(rows.num_columns(), 1);
      for(const auto& r: rows) {
        TS_ASSERT_EQUALS(r.size(), 1);
        TS_ASSERT_EQUALS(r[0].get<flex_int>(), ctr);
        ++ctr;
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_sarray_test, sarray_test)
BOOST_AUTO_TEST_CASE(test_sarray_basic) {
  sarray_test::test_sarray_basic();
}
BOOST_AUTO_TEST_CASE(test_sarray_more_interesting) {
  sarray_test::test_sarray_more_interesting();
}
BOOST_AUTO_TEST_CASE(test_sarray_strings) {
  sarray_test::test_sarray_strings();
}
BOOST_AUTO_TEST_CASE(test_sarray_transform) {
  sarray_test::test_sarray_transform();
}
BOOST_AUTO_TEST_CASE(test_sarray_copy) {
  sarray_test::test_sarray_copy();
}
BOOST_AUTO_TEST_CASE(test_sarray_flexible_type_strings) {
  sarray_test::test_sarray_flexible_type_strings();
}
BOOST_AUTO_TEST_CASE(test_sarray_save) {
  sarray_test::test_sarray_save();
}
BOOST_AUTO_TEST_CASE(test_sarray_save_empty) {
  sarray_test::test_sarray_save_empty();
}
BOOST_AUTO_TEST_CASE(test_sarray_append) {
  sarray_test::test_sarray_append();
}
BOOST_AUTO_TEST_CASE(test_sarray_small_append) {
  sarray_test::test_sarray_small_append();
}
BOOST_AUTO_TEST_CASE(test_sarray_many_small_append) {
  sarray_test::test_sarray_many_small_append();
}
BOOST_AUTO_TEST_CASE(test_sarray_recursive_append) {
  sarray_test::test_sarray_recursive_append();
}
BOOST_AUTO_TEST_CASE(test_sarray_logical_segments) {
  sarray_test::test_sarray_logical_segments();
}
BOOST_AUTO_TEST_CASE(test_sarray_v2_encoded_block) {
  sarray_test::test_sarray_v2_encoded_block();
}
BOOST_AUTO_TEST_CASE(test_sarray_sframe_rows) {
  sarray_test::test_sarray_sframe_rows();
}
BOOST_AUTO_TEST_SUITE_END()
