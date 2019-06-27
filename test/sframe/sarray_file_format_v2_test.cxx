#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/storage/sframe_data/sarray_file_format_v2.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <timer/timer.hpp>
#include <core/random/random.hpp>

using namespace turi;

struct sarray_file_format_v2_test {
 public:
  void test_index_file(void) {
    std::string tempname = get_temp_name();
    // write a file
    group_index_file_information info;
    info.version = 2;
    info.segment_files = {get_temp_name(), get_temp_name(), get_temp_name()};
    info.nsegments = 3;
    info.columns.resize(2);
    info.columns[0].version = 2;
    info.columns[0].nsegments = 3;
    info.columns[0].segment_files = info.segment_files;
    info.columns[0].content_type = "0";
    info.columns[0].metadata["col"] = "0";
    info.columns[0].block_size = 1000;
    info.columns[0].segment_sizes = {30,20,10};
    info.columns[1].version = 2;
    info.columns[1].nsegments = 3;
    info.columns[1].segment_files = info.segment_files;
    info.columns[1].content_type = "1";
    info.columns[1].metadata["col"] = "1";
    info.columns[1].block_size = 1000;
    info.columns[1].segment_sizes = {10,20,30};

    write_array_group_index_file(tempname, info);

    {
      std::ifstream fin;
      fin.open(tempname);
      TS_ASSERT(fin.good());
      while(fin.good()) {
        std::string s;
        std::getline(fin, s);
        std::cout << s << "\n";
      }
      fin.close();
    }
    group_index_file_information info2;
    info2 = read_array_group_index_file(tempname);
    TS_ASSERT_EQUALS(info2.version, info.version);
    TS_ASSERT_EQUALS(info2.nsegments, info.nsegments);
    TS_ASSERT_EQUALS(info2.segment_files.size(), info.segment_files.size());
    for (size_t i = 0;i < info.segment_files.size(); ++i) {
      TS_ASSERT_EQUALS(info2.segment_files[i], info.segment_files[i]);
    }
    TS_ASSERT_EQUALS(info2.columns.size(), info.columns.size());
    for (size_t i = 0;i < info.columns.size(); ++i) {
      TS_ASSERT_EQUALS(info2.columns[i].version, info.columns[i].version);
      TS_ASSERT_EQUALS(info2.columns[i].content_type, info.columns[i].content_type);
      TS_ASSERT_EQUALS(info2.columns[i].nsegments, info.nsegments);
      // v2 format does not save block size
      //TS_ASSERT_EQUALS(info2.columns[i].block_size, info.columns[i].block_size);
      TS_ASSERT_EQUALS(info2.columns[i].metadata["col"], info.columns[i].metadata["col"]);
      TS_ASSERT_EQUALS(info2.columns[i].segment_files.size(), info.columns[i].segment_files.size());
      for (size_t j = 0;j < info.segment_files.size(); ++j) {
        TS_ASSERT_EQUALS(info2.columns[i].segment_files[j], info.segment_files[j] + ":" + std::to_string(i));
      }
      TS_ASSERT_EQUALS(info2.columns[i].segment_sizes.size(), info.columns[i].segment_sizes.size());
      for (size_t j = 0;j < info.segment_files.size(); ++j) {
        TS_ASSERT_EQUALS(info2.columns[i].segment_sizes[j], info.columns[i].segment_sizes[j]);
      }
    }
  }



  void test_file_format_v2_basic(void) {
    // write a file
    sarray_group_format_writer_v2<size_t> group_writer;

    // open with 4 segments, 1 column
    std::string test_file_name = get_temp_name() + ".sidx";
    group_writer.open(test_file_name, 4, 1);

    TS_ASSERT_EQUALS(group_writer.num_segments(), 4);
    for (size_t i = 0;i < 4; ++i) {
      for (size_t j = 0;j < 100; ++j) {
        group_writer.write_segment(0, i, j);
      }
    }
    // no segment 4 to write to
#ifndef NDEBUG
    TS_ASSERT_THROWS_ANYTHING(group_writer.write_segment(0, 4, 2));
#endif
    group_writer.close();
    group_writer.write_index_file();

    for (size_t i = 0;i < 4; ++i) {
      // cannot write after close
#ifndef NDEBUG
      TS_ASSERT_THROWS_ANYTHING(group_writer.write_segment(i, 0, 0));
#endif
    }
    // now see if I can read it
    sarray_format_reader_v2<size_t> reader;

    reader.open(test_file_name + ":0");
    index_file_information info = reader.get_index_info();
    // check the meta data
    TS_ASSERT_EQUALS(info.version, 2);
    // check segments and segment sizes
    TS_ASSERT_EQUALS(info.nsegments, 4);
    for (size_t i = 0; i < 4; ++i) {
      TS_ASSERT_EQUALS(info.segment_sizes[i], 100);
      TS_ASSERT_EQUALS(reader.segment_size(i), 100);
    }
    TS_ASSERT_EQUALS(info.segment_sizes.size(), 4);

    // read the data we wrote the last time
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0;j < 100; ++j) {
        std::vector<size_t> val;
        reader.read_rows(i*100 + j, i*100 + j + 1, val);
        TS_ASSERT_EQUALS(val.size(), 1);
        TS_ASSERT_EQUALS(val[0], j);
      }
    }
    // reading from nonexistant segment is an error
    reader.close();
  }


  static const size_t VERY_LARGE_SIZE = 4*1024*1024;
  void test_random_access(void) {
    // write a file
    sarray_group_format_writer_v2<size_t> group_writer;

    timer ti;
    ti.start();
    // open with 4 segments
    std::string test_file_name = get_temp_name() + ".sidx";
    group_writer.open(test_file_name, 16, 1);

    TS_ASSERT_EQUALS(group_writer.num_segments(), 16);
    // open all segments, write one sequential value spanning all segments
    size_t v = 0;
    for (size_t i = 0;i < 16; ++i) {
      for (size_t j = 0;j < VERY_LARGE_SIZE; ++j) {
        group_writer.write_segment(0, i, v);
        ++v;
      }
    }
    group_writer.close();
    group_writer.write_index_file();
    std::cout << "Written 16*4M = 64M integers to disk sequentially in: " 
              << ti.current_time() << " seconds \n";


    // random reads
    {
      ti.start();
      // now see if I can read it
      sarray_format_reader_v2<size_t> reader;
      reader.open(test_file_name + ":0");
      random::seed(10001);
      for (size_t i = 0;i < 1600; ++i) {
        size_t len = 4096;
        size_t start = random::fast_uniform<size_t>(0, 16 * VERY_LARGE_SIZE - 4097);
        std::vector<size_t> vals;
        reader.read_rows(start, start + len, vals);
        TS_ASSERT_EQUALS(vals.size(), len);
        for (size_t i = 0; i < len; ++i) {
          TS_ASSERT_EQUALS(vals[i], start + i);
        }
      }
      std::cout << "1600 random seeks of 4096 values in " 
                << ti.current_time() << " seconds\n" << std::endl;

      // test some edge cases. Read past the end
      size_t end = 16 * VERY_LARGE_SIZE;
      std::vector<size_t> vals;
      size_t ret = reader.read_rows(end - 5, 2 * end, vals);
      TS_ASSERT_EQUALS(ret, 5);
      TS_ASSERT_EQUALS(vals.size(), 5);
      for (size_t i = 0;i < vals.size(); ++i) {
        if (vals[i] != end - (5 - i)) TS_ASSERT_EQUALS(vals[i], end - (5 - i));
      }
    }

    // random sequential reads
    {
      ti.start();
      // now see if I can read it
      sarray_format_reader_v2<size_t> reader;
      reader.open(test_file_name + ":0");
      random::seed(10001);
      std::vector<size_t> start_points;
      for (size_t i = 0;i < 16; ++i) {
        start_points.push_back(
            random::fast_uniform<size_t>(0, 
                                         15 * VERY_LARGE_SIZE));
                                         // 15 so as to give some gap for reading
      }
      for (size_t i = 0;i < 100; ++i) {
        for (size_t j = 0;j < start_points.size(); ++j) {
          size_t len = 4096;
          std::vector<size_t> vals;
          reader.read_rows(start_points[j], start_points[j] + len, vals);
          TS_ASSERT_EQUALS(vals.size(), len);
          for (size_t k = 0; k < len; ++k) {
            if (vals[k] != start_points[j] + k) TS_ASSERT_EQUALS(vals[k], start_points[j] + k);
          }
          start_points[j] += len;
        }
      }
      std::cout << "1600 semi-sequential seeks of average 4096 values in " 
                << ti.current_time() << " seconds\n" << std::endl;
    }
  }

  void test_typed_random_access(void) {
    // write a file
    sarray_group_format_writer_v2<flexible_type> group_writer;

    timer ti;
    ti.start();
    // open with 4 segments
    std::string test_file_name = get_temp_name() + ".sidx";
    group_writer.open(test_file_name, 16, 1);

    TS_ASSERT_EQUALS(group_writer.num_segments(), 16);
    // open all segments, write one sequential value spanning all segments
    size_t v = 0;
    for (size_t i = 0;i < 16; ++i) {
      for (size_t j = 0;j < VERY_LARGE_SIZE; ++j) {
        group_writer.write_segment(0, i, v);
        ++v;
      }
    }
    group_writer.close();
    group_writer.write_index_file();
    std::cout << "Written 16*4M = 64M flexible_type integers to disk sequentially in: " 
              << ti.current_time() << " seconds \n";


// random reads
{
  ti.start();
  // now see if I can read it
  sarray_format_reader_v2<flexible_type> reader;
  reader.open(test_file_name + ":0");
  random::seed(10001);
  for (size_t i = 0;i < 1600; ++i) {
    size_t len = 4096;
    size_t start = random::fast_uniform<size_t>(0, 16 * VERY_LARGE_SIZE - 4097);
    std::vector<flexible_type> vals;
    reader.read_rows(start, start + len, vals);
    TS_ASSERT_EQUALS(vals.size(), len);
    for (size_t i = 0; i < len; ++i) {
      if ((size_t)(vals[i]) != start + i) TS_ASSERT_EQUALS((size_t)(vals[i]), start + i);
    }
  }
  std::cout << "1600 random seeks of 4096 flexible_type values in " 
            << ti.current_time() << " seconds\n" << std::endl;

  // test some edge cases. Read past the end
  size_t end = 16*VERY_LARGE_SIZE;
  std::vector<flexible_type> vals;
  size_t ret = reader.read_rows(end - 5, 2*end, vals);
  TS_ASSERT_EQUALS(ret, 5);
  TS_ASSERT_EQUALS(vals.size(), 5);
  for (size_t i = 0;i < vals.size(); ++i) {
    if ((size_t)vals[i] != end - (5 - i)) TS_ASSERT_EQUALS((size_t)vals[i], end - (5 - i));
  }
}

// random sequential reads
{
  ti.start();
  // now see if I can read it
  sarray_format_reader_v2<flexible_type> reader;
  reader.open(test_file_name + ":0");
  random::seed(10001);
  std::vector<size_t> start_points;
  for (size_t i = 0;i < 16; ++i) {
    start_points.push_back(
        random::fast_uniform<size_t>(0, 
                                     15 * VERY_LARGE_SIZE));
                                     // 15 so as to give some gap for reading
  }
  for (size_t i = 0;i < 100; ++i) {
    for (size_t j = 0;j < start_points.size(); ++j) {
      size_t len = 4096;
      std::vector<flexible_type> vals;
      reader.read_rows(start_points[j], start_points[j] + len, vals);
      TS_ASSERT_EQUALS(vals.size(), len);
      for (size_t k = 0; k < len; ++k) {
        if ((size_t)vals[k] != start_points[j] + k) TS_ASSERT_EQUALS((size_t)vals[k], start_points[j] + k);
      }
      start_points[j] += len;
    }
  }
  std::cout << "1600 semi-sequential seeks of average 4096 flexible_type values in " 
            << ti.current_time() << " seconds\n" << std::endl;
}

{
  ti.start();
  sarray_format_reader_v2<flexible_type> reader;
  reader.open(test_file_name + ":0");
  random::seed(10001);
  std::vector<size_t> start_points;
  for (size_t i = 0;i < 16; ++i) {
    start_points.push_back(
        random::fast_uniform<size_t>(0, 
                                     15 * VERY_LARGE_SIZE));
                                     // 15 so as to give some gap for reading
  }
  for (size_t i = 0;i < 100; ++i) {
    sframe_rows rows;
    for (size_t j = 0;j < start_points.size(); ++j) {
      size_t len = 4096;
      reader.read_rows(start_points[j], start_points[j] + len, rows);
      size_t k = 0;
      TS_ASSERT_EQUALS(rows.num_rows(), len);
      TS_ASSERT_EQUALS(rows.num_columns(), 1);
      for (const auto& val: rows) {
        if ((size_t)val[0] != start_points[j] + k) TS_ASSERT_EQUALS((size_t)val[0], start_points[j] + k);
        ++k;
      }
      TS_ASSERT_EQUALS(k, len);
      start_points[j] += len;
    }
  }
  std::cout << "1600 sframe_rows semi-sequential seeks of average 4096 flexible_type values in " 
            << ti.current_time() << " seconds\n" << std::endl;
}

{
  sarray_format_reader_v2<flexible_type> reader;
  reader.open(test_file_name + ":0");
  ti.start();
  std::vector<flexible_type> rows;
  for (size_t j = 0;j < 64; ++j) {
    size_t len = 1*1024*1024;
    reader.read_rows(j * len, (j+1)*len, rows);
    size_t k = 0;
    for (const auto& val: rows) {
      if ((size_t)val != j * len + k) TS_ASSERT_EQUALS((size_t)val, j * len + k);
      ++k;
    }
    TS_ASSERT_EQUALS(k, len);
  }
  std::cout << "64 vector read sequential seeks of 1M flexible_type values in " 
            << ti.current_time() << " seconds\n" << std::endl;
}


    {
      sarray_format_reader_v2<flexible_type> reader;
      reader.open(test_file_name + ":0");
      ti.start();
      sframe_rows rows;
      for (size_t j = 0;j < 64; ++j) {
        size_t len = 1*1024*1024;
        reader.read_rows(j * len, (j+1)*len, rows);
        size_t k = 0;
        TS_ASSERT_EQUALS(rows.num_rows(), len);
        TS_ASSERT_EQUALS(rows.num_columns(), 1);
        for (const auto& val: rows) {
           if ((size_t)val[0] != j * len + k) TS_ASSERT_EQUALS((size_t)val[0], j * len + k);
          ++k;
        }
        TS_ASSERT_EQUALS(k, len);
      }
      std::cout << "64 sframe_rows sequential seeks of average 1M flexible_type values in " 
                << ti.current_time() << " seconds\n" << std::endl;
    }
  }

};

BOOST_FIXTURE_TEST_SUITE(_sarray_file_format_v2_test, sarray_file_format_v2_test)
BOOST_AUTO_TEST_CASE(test_index_file) {
  sarray_file_format_v2_test::test_index_file();
}
BOOST_AUTO_TEST_CASE(test_file_format_v2_basic) {
  sarray_file_format_v2_test::test_file_format_v2_basic();
}
BOOST_AUTO_TEST_CASE(test_random_access) {
  sarray_file_format_v2_test::test_random_access();
}
BOOST_AUTO_TEST_CASE(test_typed_random_access) {
  sarray_file_format_v2_test::test_typed_random_access();
}
BOOST_AUTO_TEST_SUITE_END()
