#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>
#include <typeinfo>
#include <boost/filesystem.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/parallel_csv_parser.hpp>
#include <core/storage/sframe_data/csv_line_tokenizer.hpp>
#include <core/storage/sframe_data/csv_writer.hpp>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>
#include <core/storage/sframe_data/sframe_saving.hpp>

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<double>)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<std::string>)
using namespace turi;
struct sframe_test  {
  std::string test_writer_prefix;
  std::string test_writer_dbl_prefix;
  std::string test_writer_str_prefix;
  std::string test_writer_add_col_prefix;
  std::string test_writer_arr_prefix;
  std::string test_writer_dt_prefix;
  std::string test_writer_ndarr_prefix;
  std::string test_writer_seg_size_err_prefix;

  public:
    sframe_test() {
      sarray<flexible_type> test_writer;
      sarray<flexible_type> test_writer_dbl;
      sarray<flexible_type> test_writer_str;
      sarray<flexible_type> test_writer_add_col;
      sarray<flexible_type> test_writer_seg_size_err;
      sarray<flexible_type> test_writer_arr;
      sarray<flexible_type> test_writer_dt;
      sarray<flexible_type> test_writer_ndarr;

      this->test_writer_prefix = get_temp_name() + ".sidx";
      this->test_writer_dbl_prefix = get_temp_name() + ".sidx";
      this->test_writer_str_prefix = get_temp_name() + ".sidx";
      this->test_writer_add_col_prefix = get_temp_name() + ".sidx";
      this->test_writer_arr_prefix = get_temp_name() + ".sidx";
      this->test_writer_dt_prefix = get_temp_name() + ".sidx";
      this->test_writer_ndarr_prefix = get_temp_name() + ".sidx";
      this->test_writer_seg_size_err_prefix = get_temp_name() + ".sidx";

      std::vector<std::vector<size_t> > data{{1,2,3,4,5},
                                             {6,7,8,9,10},
                                             {11,12,13,14,15},
                                             {16,17,18,19,20}};

      test_writer.open_for_write(this->test_writer_prefix, 4);
      test_writer_dbl.open_for_write(this->test_writer_dbl_prefix, 4);
      test_writer_str.open_for_write(this->test_writer_str_prefix, 4);
      test_writer_add_col.open_for_write(this->test_writer_add_col_prefix, 4);
      test_writer_arr.open_for_write(this->test_writer_arr_prefix, 4);
      test_writer_dt.open_for_write(this->test_writer_dt_prefix, 4);
      test_writer_ndarr.open_for_write(this->test_writer_ndarr_prefix, 4);

      test_writer.set_type(flex_type_enum::INTEGER);
      test_writer_dbl.set_type(flex_type_enum::FLOAT);
      test_writer_str.set_type(flex_type_enum::STRING);
      test_writer_add_col.set_type(flex_type_enum::FLOAT);
      test_writer_arr.set_type(flex_type_enum::VECTOR);
      test_writer_dt.set_type(flex_type_enum::DATETIME);
      test_writer_ndarr.set_type(flex_type_enum::ND_VECTOR);

      for (size_t i = 0; i < 4; ++i) {
        auto iter = test_writer.get_output_iterator(i);
        auto iter_dbl = test_writer_dbl.get_output_iterator(i);
        auto iter_str = test_writer_str.get_output_iterator(i);
        auto iter_add_col = test_writer_add_col.get_output_iterator(i);
        auto iter_arr = test_writer_arr.get_output_iterator(i);
        auto iter_dt = test_writer_dt.get_output_iterator(i);
        auto iter_ndarr = test_writer_ndarr.get_output_iterator(i);
        for(auto val: data[i]) {
          *iter = val;
          *iter_dbl = val;
          *iter_str = std::to_string(val);
          *iter_add_col = val;
          *iter_dt = flex_date_time(val);
          *iter_arr = flex_vec(10,val);
          *iter_ndarr = flex_nd_vec(std::vector<double>(10,val), {2,5});
          ++iter;
          ++iter_dbl;
          ++iter_str;
          ++iter_add_col;
          ++iter_arr;
          ++iter_dt;
          ++iter_ndarr;
        }
      }

      test_writer.close();
      test_writer_dbl.close();
      test_writer_str.close();
      test_writer_add_col.close();
      test_writer_arr.close();
      test_writer_dt.close();
      test_writer_ndarr.close();

      std::vector<std::vector<size_t> > data2{{1,2,3,4},
                                             {5, 6,7,8,9,10,11,12},
                                             {13,14,15},
                                             {16,17,18,19,20}};

      test_writer_seg_size_err.open_for_write(this->test_writer_seg_size_err_prefix, 4);
      for (size_t i = 0; i < 4; ++i) {
        auto iter = test_writer_seg_size_err.get_output_iterator(i);
        for(auto val : data2[i]) {
          *iter = val;
          ++iter;
        }
      }

      test_writer_seg_size_err.close();
    }

    void test_sframe_construction() {
      // Create an sarray from on-disk representation
      auto tmp_ptr = new sarray<flexible_type>(test_writer_prefix);
      std::shared_ptr<sarray<flexible_type> > sa_ptr(tmp_ptr);
      std::vector<std::shared_ptr<sarray<flexible_type> > > v;

      // Create 3 identical columns
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);

      // Create an sframe where the first column is named and the rest
      // get an automatic name
      std::vector<std::string> name_vector;
      name_vector.push_back(std::string("the_cool_column"));
      // Test that empty strings are handled correctly
      name_vector.push_back(std::string(""));

      // ...and test that the name_vector doesn't have to be the same size
      // as the vector v.
      sframe sf(v, name_vector);

      TS_ASSERT_EQUALS(sf.num_segments(), sa_ptr->num_segments());
      TS_ASSERT_EQUALS(sf.num_columns(), 3);

      size_t num_rows = 0;
      for(size_t i = 0; i < sa_ptr->num_segments(); ++i) {
        num_rows += sa_ptr->segment_length(i);
      }
      TS_ASSERT_EQUALS(sf.num_rows(), num_rows);

      std::string x("X");
      for(size_t i = 0; i < sf.num_columns(); ++i) {
        if(i == 0) {
          TS_ASSERT_EQUALS(sf.column_name(i), std::string("the_cool_column"));
        } else {
          // Test automatic column names
          TS_ASSERT_EQUALS(sf.column_name(i),
              std::string(x + std::to_string(i+1)));
        }
        TS_ASSERT_EQUALS(sf.column_type(i), flex_type_enum::INTEGER);
      }
      // verify contents of the sframe
      std::vector<std::vector<flexible_type> > frame;
      turi::copy(sf, std::inserter(frame, frame.end()));
      TS_ASSERT_EQUALS(frame.size(), 20);
      for (size_t i = 0;i < frame.size(); ++i) {
        TS_ASSERT_EQUALS(frame[i].size(), 3);
        for (size_t j = 0; j < frame[i].size(); ++j) {
          if ((size_t)(frame[i][j]) != i + 1) TS_ASSERT_EQUALS((size_t)(frame[i][j]), i + 1);
        }
      }

      // Test that I can add a misaligned segment
      auto tmp_ptr_seg = new sarray<flexible_type>(test_writer_seg_size_err_prefix);
      std::shared_ptr<sarray<flexible_type> > seg_size_ptr(tmp_ptr_seg);
      v.push_back(seg_size_ptr);
      sframe sf2(v);

      // and that the contents match up
      frame.clear();
      turi::copy(sf2, std::inserter(frame, frame.end()));
      TS_ASSERT_EQUALS(frame.size(), 20);
      for (size_t i = 0;i < frame.size(); ++i) {
        TS_ASSERT_EQUALS(frame[i].size(), 4);
        for (size_t j = 0; j < frame[i].size(); ++j) {
          if ((size_t)(frame[i][j]) != i + 1) TS_ASSERT_EQUALS((size_t)(frame[i][j]), i + 1);
        }
      }

      // Unique column name
      name_vector.push_back(std::string("the_cool_column"));
      TS_ASSERT_THROWS_ANYTHING(sframe sf3(v, name_vector));
    }

    void test_empty_sframe() {
      sframe sf;
      sf.open_for_write({"hello","world","pika"},
                  {flex_type_enum::FLOAT, flex_type_enum::FLOAT, flex_type_enum::INTEGER});
      sf.close();
      TS_ASSERT(sf.is_opened_for_read() == true);
      auto reader = sf.get_reader();
      TS_ASSERT_EQUALS(sf.size(), 0);

      sframe sf2 = sf.select_columns({"hello","world"});
      TS_ASSERT(sf2.is_opened_for_read() == true);
      auto reader2 = sf2.get_reader();
      TS_ASSERT_EQUALS(sf2.size(), 0);
    }

    void test_sframe_save() {
      // Create an sarray from on-disk representation
      auto tmp_ptr = new sarray<flexible_type>(test_writer_prefix);
      std::shared_ptr<sarray<flexible_type> > sa_ptr(tmp_ptr);
      std::vector<std::shared_ptr<sarray<flexible_type> > > v;

      // Create 3 identical columns
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);

      // Create SFrame with auto-named columns
      sframe *sf = new sframe(v);
      size_t exp_num_rows = sf->num_rows();
      size_t exp_num_cols = sf->num_columns();

      // Normal use case is to give an index file in a persistent place,
      // but that could cause errors in a unit test
      std::string index_file = get_temp_name();
      index_file += ".frame_idx";

      std::cerr << index_file;

      // Save in a different spot
      sf->save(index_file);

      std::vector<std::vector<flexible_type> > frame;
      turi::copy(*sf, std::inserter(frame, frame.end()));


      // Get rid of the original copy (to make sure the saved one is legit)
      delete sf;

      // Check that new files are in their spot
      TS_ASSERT(boost::filesystem::exists(index_file));

      // Load sframe back and check that the contents are right
      sframe *sf2 = new sframe(index_file);
      TS_ASSERT_EQUALS(sf2->num_rows(), exp_num_rows);
      TS_ASSERT_EQUALS(sf2->num_columns(), exp_num_cols);

      std::vector<std::vector<flexible_type> > new_frame;
      turi::copy(*sf2, std::inserter(new_frame, new_frame.end()));
      TS_ASSERT_EQUALS(new_frame.size(), frame.size());
      for (size_t i = 0;i < frame.size(); ++i) {
        TS_ASSERT_EQUALS(new_frame[i].size(), frame[i].size());
        for (size_t j = 0; j < frame[i].size(); ++j) {
          TS_ASSERT_EQUALS(new_frame[i][j], frame[i][j]);
        }
      }

      // serialize sf2
      {
        std::string dirpath = "sframe_test_dir";
        turi::dir_archive dir;
        dir.open_directory_for_write(dirpath);
        turi::oarchive oarc(dir);
        oarc << (*sf2);
      }
      delete sf2;

      {
        // Load sframe back and check that the contents are right
        std::string dirpath = "sframe_test_dir";
        turi::dir_archive dir;
        dir.open_directory_for_read(dirpath);
        sframe sf3;
        turi::iarchive iarc(dir);
        iarc >> sf3;
        TS_ASSERT_EQUALS(sf3.num_rows(), exp_num_rows);
        TS_ASSERT_EQUALS(sf3.num_columns(), exp_num_cols);

        std::vector<std::vector<flexible_type> > new_frame;
        turi::copy(sf3, std::inserter(new_frame, new_frame.end()));
        TS_ASSERT_EQUALS(new_frame.size(), frame.size());
        for (size_t i = 0;i < frame.size(); ++i) {
          TS_ASSERT_EQUALS(new_frame[i].size(), frame[i].size());
          for (size_t j = 0; j < frame[i].size(); ++j) {
            if (new_frame[i][j] != frame[i][j]) TS_ASSERT_EQUALS(new_frame[i][j], frame[i][j]);
          }
        }
      }
    }


    void test_sframe_save_reference_no_copy() {
      // Create an sarray from on-disk representation
      auto tmp_ptr = new sarray<flexible_type>(test_writer_prefix);
      std::shared_ptr<sarray<flexible_type> > sa_ptr(tmp_ptr);
      std::vector<std::shared_ptr<sarray<flexible_type> > > v;

      // Create 3 identical columns
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);
      v.push_back(sa_ptr);

      // Create SFrame with auto-named columns
      sframe *sf = new sframe(v);
      size_t exp_num_rows = sf->num_rows();
      size_t exp_num_cols = sf->num_columns();

      // Normal use case is to give an index file in a persistent place,
      // but that could cause errors in a unit test
      std::string base_name = get_temp_name();
      auto index_file = base_name + ".frame_idx";

      std::cerr << index_file;

      // Save in a different spot
      sframe_save_weak_reference(*sf, index_file);

      std::vector<std::vector<flexible_type> > frame;
      turi::copy(*sf, std::inserter(frame, frame.end()));


      // Check that new files are in their spot
      TS_ASSERT(boost::filesystem::exists(index_file));


      // Load sframe back and check that the contents are right
      sframe *sf2 = new sframe(index_file);
      TS_ASSERT_EQUALS(sf2->num_rows(), exp_num_rows);
      TS_ASSERT_EQUALS(sf2->num_columns(), exp_num_cols);

      std::vector<std::vector<flexible_type> > new_frame;
      turi::copy(*sf2, std::inserter(new_frame, new_frame.end()));
      TS_ASSERT_EQUALS(new_frame.size(), frame.size());
      for (size_t i = 0;i < frame.size(); ++i) {
        TS_ASSERT_EQUALS(new_frame[i].size(), frame[i].size());
        for (size_t j = 0; j < frame[i].size(); ++j) {
          if (new_frame[i][j] != frame[i][j]) TS_ASSERT_EQUALS(new_frame[i][j], frame[i][j]);
        }
      }
    }

    void test_sframe_save_reference_one_copy() {
      /*
       * Create a saved sframe of 2 columns.
       * create one more cache column.
       * save a reference to this new sframe
       */
      // the original sarray we are going to replicate
      auto tmp_ptr = new sarray<flexible_type>(test_writer_prefix);
      std::shared_ptr<sarray<flexible_type> > sa_ptr(tmp_ptr);
      // save an sframe of 2 columns
      auto base_sframe = get_temp_name() + ".frame_idx";
      {
        // Create an sarray from on-disk representation
        std::vector<std::shared_ptr<sarray<flexible_type> > > v;
        v.push_back(sa_ptr);
        v.push_back(sa_ptr);
        // Create SFrame with auto-named columns
        sframe sf(v);
        sf.save(base_sframe);

      }
      sframe bsf(base_sframe);
      // create an inmemory sarray and append to it
      std::shared_ptr<sarray<flexible_type> > sa2 = std::make_shared<sarray<flexible_type>>();
      sa2->open_for_write(1);
      sframe_rows rows;
      sa_ptr->get_reader()->read_rows(0, sa_ptr->size(), rows);
      (*sa2->get_output_iterator(0)) = rows;
      sa2->close();
      auto sf = bsf.add_column(sa2);
      size_t exp_num_rows = sf.num_rows();
      size_t exp_num_cols = sf.num_columns();

      // Normal use case is to give an index file in a persistent place,
      // but that could cause errors in a unit test
      std::string base_name = get_temp_name();
      auto index_file = base_name + ".frame_idx";

      std::cerr << index_file;

      // Save in a different spot
      sframe_save_weak_reference(sf, index_file);

      std::vector<std::vector<flexible_type> > frame;
      turi::copy(sf, std::inserter(frame, frame.end()));


      // Check that new files are in their spot
      TS_ASSERT(boost::filesystem::exists(index_file));


      // Load sframe back and check that the contents are right
      sframe *sf2 = new sframe(index_file);
      TS_ASSERT_EQUALS(sf2->num_rows(), exp_num_rows);
      TS_ASSERT_EQUALS(sf2->num_columns(), exp_num_cols);

      // 3rd cikynb 
      TS_ASSERT_DIFFERS(parse_v2_segment_filename(sf2->get_index_info().column_files[2]).first, 
                       parse_v2_segment_filename(bsf.get_index_info().column_files[1]).first);

      std::vector<std::vector<flexible_type> > new_frame;
      turi::copy(*sf2, std::inserter(new_frame, new_frame.end()));
      TS_ASSERT_EQUALS(new_frame.size(), frame.size());
      for (size_t i = 0;i < frame.size(); ++i) {
        TS_ASSERT_EQUALS(new_frame[i].size(), frame[i].size());
        for (size_t j = 0; j < frame[i].size(); ++j) {
          if (new_frame[i][j] != frame[i][j]) TS_ASSERT_EQUALS(new_frame[i][j], frame[i][j]);
        }
      }
      delete sf2;
    }

    void test_sframe_save_empty_columns() {
      sframe sf;
      sf.open_for_write({"col1", "col2"}, {flex_type_enum::INTEGER, flex_type_enum::STRING});
      sf.close();
      std::string index = get_temp_name() + ".frame_idx";
      sf.save(index);

      sframe newsf(index);
      TS_ASSERT_EQUALS(newsf.size(), 0);
    }

    void test_sframe_save_really_empty() {
      sframe sf;
      sf.open_for_write(std::vector<std::string>(), std::vector<flex_type_enum>());
      sf.close();
      std::string index = get_temp_name() + ".frame_idx";
      sf.save(index);

      sframe newsf(index);
      TS_ASSERT_EQUALS(newsf.size(), 0);
    }

    void test_sframe_dataframe_conversion() {
      std::vector<flexible_type> int_col{0,1,2,3,4,5};
      std::vector<flexible_type> float_col{.0,.1,.2,.3,.4,.5};
      std::vector<flexible_type> str_col{".0",".1",".2",".3",".4",".5"};
      std::vector<flexible_type> vec_col{{.0},{.1},{.2},{.3},{.4},{.5}};
      dataframe_t df;
      df.set_column("int_col", int_col, flex_type_enum::INTEGER);
      df.set_column("float_col", float_col, flex_type_enum::FLOAT);
      df.set_column("str_col", str_col, flex_type_enum::STRING);
      df.set_column("vec_col", vec_col, flex_type_enum::VECTOR);

      // Test df -> sf
      sframe sf(df);
      TS_ASSERT_EQUALS(sf.num_rows(), 6);
      TS_ASSERT_EQUALS(sf.num_columns(), 4);
      std::vector<flex_type_enum> expected_types{flex_type_enum::INTEGER, flex_type_enum::FLOAT, flex_type_enum::STRING, flex_type_enum::VECTOR};
      std::vector<std::string> expected_names{"int_col", "float_col", "str_col", "vec_col"};

      for (size_t i = 0; i < sf.num_columns(); ++i) {
        TS_ASSERT_EQUALS(sf.column_type(i), expected_types[i]);
        TS_ASSERT_EQUALS(sf.column_name(i), expected_names[i]);
      }

      size_t ctr = 0;
      auto reader = sf.get_reader();
      for (size_t i = 0; i < reader->num_segments(); ++i) {
        auto iter = reader->begin(i);
        while(iter != reader->end(i)) {
          std::vector<flexible_type> row = *iter;
          TS_ASSERT_EQUALS(row.size(), reader->num_columns());
          for (size_t j = 0; j < row.size(); ++j) {
            if (j == 0) {
              TS_ASSERT_EQUALS(row[j], int_col[ctr]);
            } else if (j == 1) {
              TS_ASSERT_EQUALS(row[j], float_col[ctr]);
            } else if (j == 2){
              TS_ASSERT_EQUALS(row[j], str_col[ctr]);
            } else {
              TS_ASSERT_EQUALS(row[j], vec_col[ctr]);
            }
          }
          ++iter;
          ++ctr;
        }
      }
      // Test sf -> df
      dataframe_t df2 = sf.to_dataframe();
      TS_ASSERT_EQUALS(df2.names, df.names);
      TS_ASSERT_EQUALS(df2.types, df.types);
      TS_ASSERT_EQUALS(df2.values, df.values);
    }

    void test_sframe_dataframe_conversion_with_na() {
      std::vector<flexible_type> int_col{0,1,2,3,4,5};
      std::vector<flexible_type> float_col{.0,.1,.2,.3,.4,.5};
      std::vector<flexible_type> str_col{".0",".1",".2",".3",".4",".5"};
      std::vector<flexible_type> vec_col{{.0},{.1},{.2},{.3},{.4},{.5}};
      // set the last row to NA
      int_col[int_col.size() - 1].reset(flex_type_enum::UNDEFINED);
      float_col[float_col.size() - 1].reset(flex_type_enum::UNDEFINED);
      str_col[str_col.size() - 1].reset(flex_type_enum::UNDEFINED);
      vec_col[vec_col.size() - 1].reset(flex_type_enum::UNDEFINED);
      dataframe_t df;
      df.set_column("int_col", int_col, flex_type_enum::INTEGER);
      df.set_column("float_col", float_col, flex_type_enum::FLOAT);
      df.set_column("str_col", str_col, flex_type_enum::STRING);
      df.set_column("vec_col", vec_col, flex_type_enum::VECTOR);

      // Test df -> sf
      sframe sf(df);
      TS_ASSERT_EQUALS(sf.num_rows(), 6);
      TS_ASSERT_EQUALS(sf.num_columns(), 4);
      std::vector<flex_type_enum> expected_types{flex_type_enum::INTEGER, flex_type_enum::FLOAT, flex_type_enum::STRING, flex_type_enum::VECTOR};
      std::vector<std::string> expected_names{"int_col", "float_col", "str_col", "vec_col"};

      for (size_t i = 0; i < sf.num_columns(); ++i) {
        TS_ASSERT_EQUALS(sf.column_type(i), expected_types[i]);
        TS_ASSERT_EQUALS(sf.column_name(i), expected_names[i]);
      }
      auto reader = sf.get_reader();
      size_t ctr = 0;
      for (size_t i = 0; i < reader->num_segments(); ++i) {
        auto iter = reader->begin(i);
        while(iter != reader->end(i)) {
          std::vector<flexible_type> row = *iter;
          TS_ASSERT_EQUALS(row.size(), reader->num_columns());
          for (size_t j = 0; j < row.size(); ++j) {
            if (ctr < 5) {
              if (j == 0) {
                TS_ASSERT_EQUALS(row[j], int_col[ctr]);
              } else if (j == 1) {
                TS_ASSERT_EQUALS(row[j], float_col[ctr]);
              } else if (j == 2){
                TS_ASSERT_EQUALS(row[j], str_col[ctr]);
              } else {
                TS_ASSERT_EQUALS(row[j], vec_col[ctr]);
              }
            } if (ctr == 5) {
              TS_ASSERT_EQUALS(row[j].get_type(), flex_type_enum::UNDEFINED);
            }
          }
          ++iter;
          ++ctr;
        }
      }

      // Test sf -> df
      dataframe_t df2 = sf.to_dataframe();
      TS_ASSERT_EQUALS(df2.names, df.names);
      TS_ASSERT_EQUALS(df2.types, df.types);
      // we can't compare values because UNDEFINED != UNDEFINED.
      // annoyingly. So we have to do this explicitly.
      for (const auto& col: df.values) {
        const auto& col2 = *(df2.values.find(col.first));
        TS_ASSERT_EQUALS(col.first, col2.first);
        TS_ASSERT_EQUALS(col.second.size(), col2.second.size());
        for (size_t i = 0;i < col.second.size(); ++i) {
          TS_ASSERT_EQUALS(col.second[i].get_type(), col2.second[i].get_type());
          if (col.second[i].get_type() != flex_type_enum::UNDEFINED) {
            TS_ASSERT_EQUALS(col.second[i], col2.second[i]);
          }
        }
      }
    }

    void test_sframe_iterate() {
      // Create an sframe
      std::vector<std::shared_ptr<sarray<flexible_type>>>
        v{std::make_shared<sarray<flexible_type>>(test_writer_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_dbl_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_str_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_arr_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_dt_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_ndarr_prefix)
        };

      sframe sf(v);
      size_t nrows = sf.num_rows();

      auto reader = sf.get_reader();

      for(size_t i = 0; i < reader->num_segments(); ++i) {
        auto iter = reader->begin(i);
        auto end_iter = reader->end(i);
        TS_ASSERT(iter != end_iter);
        TS_ASSERT(iter == iter);
        size_t startrow = 0;
        for (size_t j = 0; j < i; ++j) {
          startrow += reader->segment_length(j);
        }
        size_t rowid = startrow;
        while(iter != end_iter) {
          std::vector<flexible_type> expected {flex_int(rowid+1),
                                               flex_float(rowid+1),
                                               flex_string(std::to_string(rowid+1)),
                                               flex_vec(10, rowid+1),
                                               flex_date_time(rowid+1),
                                               flex_nd_vec(std::vector<double>(10, rowid+1), {2,5})};
          sframe::value_type actual = *iter;
          TS_ASSERT_EQUALS(actual.size(), expected.size());
          for (size_t j = 0; j < actual.size(); ++j) {
            TS_ASSERT_EQUALS(actual[j], expected[j]);
          }
          ++iter;
          ++rowid;
        }
      }

      // Test that not resetting iterators throws an exception
      TS_ASSERT_THROWS_ANYTHING(reader->begin(0));

      reader->reset_iterators();

      parallel_for(0, reader->num_segments(),
          [&](size_t segmentid) {
            auto iter = reader->begin(segmentid);
            auto end_iter = reader->end(segmentid);
            TS_ASSERT(iter != end_iter);
            TS_ASSERT(iter == iter);
            size_t startrow = 0;
            for (size_t i = 0; i < segmentid; ++i) {
              startrow += reader->segment_length(i);
            }
            size_t rowid = startrow;
            while(iter != end_iter) {
            std::vector<flexible_type> expected {flex_int(rowid+1),
                                                 flex_float(rowid+1),
                                                 flex_string(std::to_string(rowid+1)),
                                                 flex_vec(10, rowid+1),
                                                 flex_date_time(rowid+1),
                                                 flex_nd_vec(std::vector<double>(10, rowid+1), {2,5})};
              TS_ASSERT_EQUALS(iter->size(), expected.size());
              for (size_t j = 0; j < iter->size(); ++j) {
                TS_ASSERT_EQUALS(iter->at(j), expected[j]);
              }
              ++iter;
              ++rowid;
            }
          });


      // make 15 threads, each read 5 rows
      parallel_for(0, (size_t)15,
          [&](size_t startrow) {
            std::vector<std::vector<flexible_type> > ret;
            size_t nrows = reader->read_rows(startrow, startrow + 5, ret);
            TS_ASSERT_EQUALS(nrows, 5);
            TS_ASSERT_EQUALS(ret.size(), 5);
            for (size_t i = 0;i < ret.size(); ++i) {
              size_t rowid = i + startrow;
              std::vector<flexible_type> expected {flex_int(rowid+1),
                                                   flex_float(rowid+1),
                                                   flex_string(std::to_string(rowid+1)),
                                                   flex_vec(10, rowid+1),
                                                   flex_date_time(rowid+1),
                                                   flex_nd_vec(std::vector<double>(10, rowid+1), {2,5})};
              TS_ASSERT_EQUALS(ret[i].size(), expected.size());
              for (size_t j = 0; j < ret[i].size(); ++j) {
                TS_ASSERT_EQUALS(ret[i].at(j), expected[j]);
              }
            }
          });

      // once again using the sframe_rows datastructure
      // make 15 threads, each read 5 rows
      parallel_for(0, (size_t)15,
          [&](size_t startrow) {
            sframe_rows rows;
            size_t nrows = reader->read_rows(startrow, startrow + 5, rows);
            TS_ASSERT_EQUALS(nrows, 5);
            TS_ASSERT_EQUALS(rows.num_rows(), 5);
            TS_ASSERT_EQUALS(rows.num_columns(), 6);
            size_t i = 0;
            for (const auto& ret: rows) {
              size_t rowid = i + startrow;
              std::vector<flexible_type> expected {flex_int(rowid+1),
                                                   flex_float(rowid+1),
                                                   flex_string(std::to_string(rowid+1)),
                                                   flex_vec(10, rowid+1),
                                                   flex_date_time(rowid+1),
                                                   flex_nd_vec(std::vector<double>(10, rowid+1), {2,5})};
              TS_ASSERT_EQUALS(ret.size(), expected.size());
              for (size_t j = 0; j < ret.size(); ++j) {
                TS_ASSERT_EQUALS(ret.at(j), expected[j]);
              }
              ++i;
            }
          });

      // randomaccess
      // once again using the sframe_rows datastructure
      // make 15 threads, each read 5 rows
      parallel_for(0, (size_t)15,
          [&](size_t seed) {
            size_t startrow = (std::hash<unsigned long>()((unsigned long)seed)) % nrows;
            sframe_rows rows;
            size_t nrows = reader->read_rows(startrow, startrow + 5, rows);
            TS_ASSERT_EQUALS(nrows, 5);
            TS_ASSERT_EQUALS(rows.num_rows(), 5);
            TS_ASSERT_EQUALS(rows.num_columns(), 6);
            size_t i = 0;
            for (const auto& ret: rows) {
              size_t rowid = i + startrow;
              std::vector<flexible_type> expected {flex_int(rowid+1),
                                                   flex_float(rowid+1),
                                                   flex_string(std::to_string(rowid+1)),
                                                   flex_vec(10, rowid+1),
                                                   flex_date_time(rowid+1),
                                                   flex_nd_vec(std::vector<double>(10, rowid+1), {2,5})};
              TS_ASSERT_EQUALS(ret.size(), expected.size());
              for (size_t j = 0; j < ret.size(); ++j) {
                TS_ASSERT_EQUALS(ret.at(j), expected[j]);
              }
              ++i;
            }
          });


      // Test other exceptions throwing
      TS_ASSERT_THROWS_ANYTHING(reader->begin(3543));
      TS_ASSERT_THROWS_ANYTHING(reader->end(3543));
    }

    void copy_sarray(sarray<flexible_type>& src,
                     sarray<flexible_type>& dst,
                     size_t ndst_segments) {
      auto src_reader = src.get_reader(1);
      dst.open_for_write(ndst_segments);
      turi::copy(src_reader->begin(0), src_reader->end(0), dst);
      dst.close();
    }


    void validate_test_sframe_logical_segments(std::unique_ptr<sframe_reader> reader,
                                               size_t nsegments) {
      TS_ASSERT_EQUALS(reader->num_segments(), nsegments);
      // read the data we wrote the last time
      std::vector<std::vector<flexible_type> > outdata;
      for (size_t i = 0; i < nsegments; ++i) {
        auto begin = reader->begin(i);
        auto end = reader->end(i);
        while(begin != end){
          outdata.push_back(*begin);
          ++begin;
        }
      }
      TS_ASSERT_EQUALS(outdata.size(), 20);
      for (size_t i = 0;i < outdata.size(); ++i) {
        std::vector<flexible_type> expected {flex_int(i+1), flex_float(i+1)};
        auto actual = outdata[i];
        TS_ASSERT_EQUALS(actual.size(), expected.size());
        for (size_t j = 0; j < actual.size(); ++j) {
          TS_ASSERT_EQUALS(actual[j], expected[j]);
        }
      }
    }

    void test_sframe_logical_segments() {
      // Copy integers to some other target with 4 segments
      sarray<flexible_type> src_integers(test_writer_prefix);
      std::shared_ptr<sarray<flexible_type> > integers(new sarray<flexible_type>);
      copy_sarray(src_integers, *integers, 4);
      for (size_t i = 0;i < 4; ++i) TS_ASSERT(integers->segment_length(i) > 0);

      // Copy doubles to some other target with 6 segments
      sarray<flexible_type> src_doubles(test_writer_dbl_prefix);
      std::shared_ptr<sarray<flexible_type> > doubles(new sarray<flexible_type>);
      copy_sarray(src_doubles, *doubles, 6);
      for (size_t i = 0;i < 6; ++i) TS_ASSERT(doubles->segment_length(i) > 0);

      sframe sf(std::vector<std::shared_ptr<sarray<flexible_type>>>{integers, doubles});

      validate_test_sframe_logical_segments(sf.get_reader(), 4);
      validate_test_sframe_logical_segments(sf.get_reader(8), 8);
      validate_test_sframe_logical_segments(sf.get_reader(200), 200);
      std::vector<size_t> custom_sizes{4,0,6,10};
      validate_test_sframe_logical_segments(sf.get_reader(custom_sizes), 4);
    }


    void test_sframe_write() {
      // Build data
      std::vector<std::string> words {"hello", "this", "is", "a",
        "test", "of", "writing", "an", "sframe", "let's", "have", "some",
        "more", "words", "for", "good", "measure"};

      std::vector<std::vector<flexible_type>> data_rows;

      for(size_t i = 0; i < words.size(); ++i) {
        data_rows.push_back({i, double(i)+0.5, words[i]});
      }


      std::vector<flex_type_enum> column_types {flex_type_enum::INTEGER,
                                                flex_type_enum::FLOAT,
                                                flex_type_enum::STRING};
      std::vector<std::string> column_names {"nums", "decimal_nums", "words"};


      // Write a new sframe from a vector of data
      for(size_t num_segments = 1; num_segments <= 10; ++num_segments) {
        sframe frame;
        frame.open_for_write(column_names, column_types, "", num_segments);

        // Throw if open before closed
        TS_ASSERT_THROWS_ANYTHING(
            frame.open_for_write({"hello","world"},
                                 {flex_type_enum::INTEGER,
                                   flex_type_enum::STRING}));

        // Add my data rows to an sframe
        turi::copy(data_rows.begin(), data_rows.end(), frame);

        // Not used for anything, just to see if exceptions are thrown when
        // I do bad stuff.
        auto output_iter = frame.get_output_iterator(0);

        // Row of wrong size
        TS_ASSERT_THROWS_ANYTHING((
              *output_iter=std::vector<flexible_type>({1,2.0,"3","extra"})));

        frame.close();
#ifndef NDEBUG
        // Write after close
        TS_ASSERT_THROWS_ANYTHING((
              *output_iter=std::vector<flexible_type>({1,2.0,"3"})));
#endif

        TS_ASSERT_EQUALS(frame.num_segments(), num_segments);
        TS_ASSERT_EQUALS(frame.num_columns(), column_types.size());
        for(size_t i = 0; i < frame.num_columns(); ++i) {
          TS_ASSERT_EQUALS(column_names[i], frame.column_name(i));
          TS_ASSERT_EQUALS(column_types[i], frame.column_type(i));
        }


        // Check the data of the sframe
        size_t cntr = 0;
        auto reader = frame.get_reader();
        for(size_t i = 0; i < reader->num_segments(); ++i) {
          for(auto iter = reader->begin(i); iter != reader->end(i); ++iter, ++cntr) {
            std::vector<flexible_type> expected = data_rows[cntr];
            sframe::value_type actual = *iter;
            TS_ASSERT_EQUALS(iter->size(), expected.size());

            for(size_t j = 0; j < actual.size(); ++j) {
              TS_ASSERT_EQUALS(actual[j], expected[j]);
            }
          }
        }
      }
    }

    void test_select_column() {
      // Create an sframe
      std::vector<std::shared_ptr<sarray<flexible_type>>>
        v{std::make_shared<sarray<flexible_type>>(test_writer_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_dbl_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_str_prefix)};

      sframe sf(v);

      for(size_t i = 0; i < sf.num_columns(); ++i) {
        auto column = sf.select_column(i);
        size_t index = 0;
        auto reader = column->get_reader();
        for(size_t j = 0; j < reader->num_segments(); ++j) {
          auto iter = reader->begin(j);
          while(iter != reader->end(j)) {
            if (i == 0) {
              TS_ASSERT_EQUALS(*iter, flex_int(index+1));
            } else if (i == 1) {
              TS_ASSERT_EQUALS(*iter, flex_float(index+1));
            } else {
              TS_ASSERT_EQUALS(*iter, flex_string(std::to_string(index+1)));
            }
            ++index;
            ++iter;
          }
        }
      }
    }

    void test_add_column() {
      std::vector<std::shared_ptr<sarray<flexible_type>>>
        v{std::make_shared<sarray<flexible_type>>(test_writer_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_dbl_prefix),
          std::make_shared<sarray<flexible_type>>(test_writer_str_prefix)};

      sframe sf(v);

      auto sa_ptr_add_col =
        std::make_shared<sarray<flexible_type>>(test_writer_add_col_prefix);

      // Column in the original sframe that is the same as the new column
      size_t src_col = 1;
      sframe sf2 = sf.add_column(sa_ptr_add_col, std::string("copy_col"));
      TS_ASSERT_EQUALS(sf2.num_columns(), sf.num_columns()+1);

      size_t dst_col = sf2.num_columns()-1;
      TS_ASSERT_EQUALS(sf2.column_name(dst_col), "copy_col");
      TS_ASSERT_EQUALS(sf2.column_type(dst_col), sf2.column_type(src_col));
      TS_ASSERT_EQUALS(sf2.column_type(dst_col), sf.column_type(src_col));

      auto reader = sf2.get_reader();
      for(size_t i = 0; i < reader->num_segments(); ++i) {
        auto iter = reader->begin(i);
        auto end_iter = reader->end(i);
        while(iter != end_iter) {
          const auto& val = *iter;
          TS_ASSERT_EQUALS(val[src_col], val[dst_col]);
          ++iter;
        }
      }

      reader->reset_iterators();

      parallel_for(0, sf2.num_segments(),
                   [&](size_t segmentid) {
                      auto iter = reader->begin(segmentid);
                      while(iter != reader->end(segmentid)) {
                        const auto& val = *iter;
                        TS_ASSERT_EQUALS(val[src_col], val[dst_col]);
                        ++iter;
                      }
                   });

      // Test unique column name checking
      TS_ASSERT_THROWS_ANYTHING(sf2.add_column(sa_ptr_add_col,
            std::string("X1")));

    }

    // helper for the test below
    void check_basic_csv_values(sframe& frame) {
     std::vector<std::vector<flexible_type> > vals;
     turi::copy(frame, std::inserter(vals, vals.end()));

     TS_ASSERT_EQUALS(vals.size(), 3);
     TS_ASSERT_EQUALS(vals[0].size(), 6);
     TS_ASSERT_EQUALS(vals[1].size(), 6);
     TS_ASSERT_EQUALS(vals[2].size(), 6);

     TS_ASSERT_DELTA(vals[0][0], 1.1, 1E-5);
     TS_ASSERT_DELTA(vals[1][0], 2.2, 1E-5);
     TS_ASSERT_DELTA(vals[2][0], 3.3, 1E-5);

     TS_ASSERT_EQUALS(vals[0][1], 1);
     TS_ASSERT_EQUALS(vals[1][1], 2);
     TS_ASSERT_EQUALS(vals[2][1], 3);

     TS_ASSERT_EQUALS(vals[0][2], "one");
     TS_ASSERT_EQUALS(vals[1][2], "two");
     TS_ASSERT_EQUALS(vals[2][2], "three");

     {
       auto v1 = vals[0][3].get<flex_vec>();
       auto v2 = vals[1][3].get<flex_vec>();
       auto v3 = vals[2][3].get<flex_vec>();
       TS_ASSERT_EQUALS(v1.size(), 3);
       TS_ASSERT_EQUALS(v2.size(), 3);
       TS_ASSERT_EQUALS(v3.size(), 3);
       for (size_t i = 0;i < 3; ++i) {
         TS_ASSERT_EQUALS(v1[i], 1.0);
         TS_ASSERT_EQUALS(v2[i], 2.0);
         TS_ASSERT_EQUALS(v3[i], 3.0);
       }
     }


     {
       auto v1 = vals[0][4].get<flex_dict>();
       auto v2 = vals[1][4].get<flex_dict>();
       auto v3 = vals[2][4].get<flex_dict>();
       TS_ASSERT_EQUALS(v1.size(), 2);
       TS_ASSERT_EQUALS(v2.size(), 2);
       TS_ASSERT_EQUALS(v3.size(), 2);
       TS_ASSERT_EQUALS((int)v1[0].first, 1);
       TS_ASSERT_EQUALS((int)v1[0].second, 1);
       TS_ASSERT_EQUALS((int)v2[0].first, 2);
       TS_ASSERT_EQUALS((int)v2[0].second, 2);
       TS_ASSERT_EQUALS((int)v3[0].first, 3);
       TS_ASSERT_EQUALS((int)v3[0].second, 3);
       TS_ASSERT_EQUALS((std::string)v1[1].first, "a");
       TS_ASSERT_EQUALS((std::string)v1[1].second, "a");
       TS_ASSERT_EQUALS((std::string)v2[1].first, "b");
       TS_ASSERT_EQUALS((std::string)v2[1].first, "b");
       TS_ASSERT_EQUALS((std::string)v3[1].second, "c");
       TS_ASSERT_EQUALS((std::string)v3[1].second, "c");
     }


     {
       auto v1 = vals[0][5].get<flex_list>();
       auto v2 = vals[1][5].get<flex_list>();
       auto v3 = vals[2][5].get<flex_list>();
       TS_ASSERT_EQUALS(v1.size(), 2);
       TS_ASSERT_EQUALS(v2.size(), 2);
       TS_ASSERT_EQUALS(v3.size(), 2);
       TS_ASSERT_EQUALS((std::string)v1[0], "a");
       TS_ASSERT_EQUALS((std::string)v1[1], "a");
       TS_ASSERT_EQUALS((std::string)v2[0], "b");
       TS_ASSERT_EQUALS((std::string)v2[1], "b");
       TS_ASSERT_EQUALS((std::string)v3[0], "c");
       TS_ASSERT_EQUALS((std::string)v3[1], "c");
     }
    }


    // helper for the test below
    void check_basic_csv_string_values(sframe& frame) {
     std::vector<std::vector<flexible_type> > vals;
     turi::copy(frame, std::inserter(vals, vals.end()));

     TS_ASSERT_EQUALS(vals.size(), 3);
     TS_ASSERT_EQUALS(vals[0].size(), 6);
     TS_ASSERT_EQUALS(vals[1].size(), 6);
     TS_ASSERT_EQUALS(vals[2].size(), 6);

     TS_ASSERT_EQUALS(std::string(vals[0][0]), "1.1");
     TS_ASSERT_EQUALS(std::string(vals[1][0]), "2.2");
     TS_ASSERT_EQUALS(std::string(vals[2][0]), "3.3");

     TS_ASSERT_EQUALS(std::string(vals[0][1]), "1");
     TS_ASSERT_EQUALS(std::string(vals[1][1]), "2");
     TS_ASSERT_EQUALS(std::string(vals[2][1]), "3");

     TS_ASSERT_EQUALS(std::string(vals[0][2]), "one");
     TS_ASSERT_EQUALS(std::string(vals[1][2]), "two");
     TS_ASSERT_EQUALS(std::string(vals[2][2]), "three");


     TS_ASSERT_EQUALS(std::string(vals[0][3]), "[1,1,1]");
     TS_ASSERT_EQUALS(std::string(vals[1][3]), "[2,2,2]");
     TS_ASSERT_EQUALS(std::string(vals[2][3]), "[3,3,3]");

     TS_ASSERT_EQUALS(std::string(vals[0][4]), "{1:1,\"a\":\"a\"}");
     TS_ASSERT_EQUALS(std::string(vals[1][4]), "{2:2,\"b\":\"b\"}");
     TS_ASSERT_EQUALS(std::string(vals[2][4]), "{3:3,\"c\":\"c\"}");

     TS_ASSERT_EQUALS(std::string(vals[0][5]), "[a,a]");
     TS_ASSERT_EQUALS(std::string(vals[1][5]), "[b,b]");
     TS_ASSERT_EQUALS(std::string(vals[2][5]), "[c,c]");
    }

   void test_column_name_wrangling() {
     std::string basic_csv_file = get_temp_name() + ".csv";
     std::ofstream fout(basic_csv_file);
     fout << "A,A,A.1,B,C,D\n"
          << "1.1,1,one,[1,1,1],{1:1,\"a\":\"a\"},[a,a]\n"
          << "2.2,2,two,[2,2,2],{2:2,\"b\":\"b\"},[b,b]\n"
          << " 3.3,3,three,[3,3,3],{3:3,\"c\":\"c\"},[c,c]\n";

     fout.close();
     // parse should make 2nd column A.2
     // we also omit the hint for A.1. It should default to string
     csv_line_tokenizer tokenizer;
     tokenizer.delimiter = ',';
     tokenizer.init();
     sframe frame;
     frame.init_from_csvs(basic_csv_file,
                          tokenizer,
                          true,   // header
                          false, // continue on failure
                          false,  // do not store errors
                          {{"A", flex_type_enum::FLOAT}, // hints
                            {"A.2", flex_type_enum::INTEGER},
                            {"A.1", flex_type_enum::STRING},
                            {"B", flex_type_enum::VECTOR},
                            {"C",flex_type_enum::DICT},
                            {"D",flex_type_enum::LIST}});
     TS_ASSERT_EQUALS(frame.num_rows(), 3);
     TS_ASSERT_EQUALS(frame.num_columns(), 6);
     TS_ASSERT_EQUALS(frame.column_name(0), "A");
     TS_ASSERT_EQUALS(frame.column_name(1), "A.2");
     TS_ASSERT_EQUALS(frame.column_name(2), "A.1");
     TS_ASSERT_EQUALS(frame.column_name(3), "B");
     TS_ASSERT_EQUALS(frame.column_name(4), "C");
     TS_ASSERT_EQUALS(frame.column_name(5), "D");
     TS_ASSERT_EQUALS(frame.column_type(0), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(frame.column_type(1), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(frame.column_type(2), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(frame.column_type(3), flex_type_enum::VECTOR);
     TS_ASSERT_EQUALS(frame.column_type(4), flex_type_enum::DICT);
     TS_ASSERT_EQUALS(frame.column_type(5), flex_type_enum::LIST);

     check_basic_csv_values(frame);
   }



   void run_groupby_aggregate_sum_test(size_t NUM_GROUPS,
                                       size_t NUM_ROWS,
                                       size_t BUFFER_SIZE) {
     // create an SFrame with 5 columns, string, int, float, int, unused
     sframe input;
     input.open_for_write({"str","int","float","int2","unused","vector"},
                          {flex_type_enum::STRING, flex_type_enum::INTEGER,
                          flex_type_enum::FLOAT, flex_type_enum::INTEGER,
                          flex_type_enum::INTEGER, flex_type_enum::VECTOR},
                          "", 4 /* 4 segments*/);
     // we will emit the following into the sframe
     // "0", 0, 0.0, 1, 2, {0.0, 0.0, ...}
     // "1", 1, 0.5, 2, 3, {1.0, 1.0, ...}
     // "2", 2, 1.0, 3, 4, {2.0, 2.0, ...}
     // ...
     // "99", 99, 49.5, 100, 101, {99.0, 99.0, ...}
     // "0", 100, 50.0, 101, 102, {100.0, 100.0, ...}
     // "1", 101, 50.5, 102, 103, {101.0, 101.0, ...}
     // etc.

     // we will groupby column 0 and sum the next 4 columns, summing the 4th
     // column (int2) twice. And ignore the last column
     std::cout << "Generating input data: " << std::endl;
     std::unordered_map<std::string, flexible_type> group_results[4];
     for (size_t i = 0;i < NUM_GROUPS; ++i) {
         std::string key = std::to_string(i % NUM_GROUPS);
         group_results[0][key].reset(flex_type_enum::INTEGER);
         group_results[1][key].reset(flex_type_enum::FLOAT);
         group_results[2][key].reset(flex_type_enum::INTEGER);
         group_results[3][key].reset(flex_type_enum::VECTOR);
         group_results[3][key] = std::vector<double> (10,0);
     }
     for (size_t i = 0;i < NUM_ROWS; ++i) {
       auto iter = input.get_output_iterator(i % 4);
       std::vector<flexible_type> flex(6);
       std::string key = std::to_string(i % NUM_GROUPS);
       flex[0] = key;
       flex[1] = i;
       flex[2] = (double)i / 2.0;
       flex[3] = i + 1;
       flex[4] = i + 2;
       flex[5] = std::vector<double> (10,i);
       (*iter) = flex;
       ++iter;
       group_results[0][key] += i;
       group_results[1][key] += (double)i / 2.0;
       group_results[2][key] += i + 1;
       group_results[3][key] += flex[5];
     }
     input.close();
     std::cout << "Starting groupby: " << std::endl;
     turi::timer ti;
     sframe output = turi::groupby_aggregate(input,
                                       {"str"},
                                       {"intsum","floatsum","","",""},
                                       {{{"int"}, std::make_shared<groupby_operators::sum>()},
                                       {{"float"}, std::make_shared<groupby_operators::sum>()},
                                       {{"int2"}, std::make_shared<groupby_operators::sum>()},
                                       {{"int2"}, std::make_shared<groupby_operators::sum>()},
                                       {{"vector"}, std::make_shared<groupby_operators::vector_sum>()}},
                                       BUFFER_SIZE);
     std::cout << "Groupby done in: " << ti.current_time() << " seconds" << std::endl;
     TS_ASSERT_EQUALS(output.num_columns(), 6);
     TS_ASSERT_EQUALS(output.num_rows(), NUM_GROUPS);
     TS_ASSERT_EQUALS(output.column_name(0), "str");
     TS_ASSERT_EQUALS(output.column_name(1), "intsum");
     TS_ASSERT_EQUALS(output.column_name(2), "floatsum");
     TS_ASSERT_EQUALS(output.column_name(3), "Sum of int2");
     TS_ASSERT_EQUALS(output.column_name(4), "Sum of int2.1");
     TS_ASSERT_EQUALS(output.column_name(5), "Vector Sum of vector");
     TS_ASSERT_EQUALS(output.column_type(0), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(1), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(2), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(3), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(4), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(5), flex_type_enum::VECTOR);

     std::vector<std::vector<flexible_type> > ret;
     int rows_read = output.get_reader()->read_rows(0, output.num_rows(), ret);
     TS_ASSERT_EQUALS(rows_read, NUM_GROUPS);
     // make sure every key is covered and is unique
     std::set<std::string> allkeys;
     for(auto& row : ret) {
       std::string key = row[0];
       allkeys.insert(key);
       TS_ASSERT_EQUALS(int(group_results[0][key]), int(row[1]));
       TS_ASSERT_EQUALS(float(group_results[1][key]), float(row[2]));
       TS_ASSERT_EQUALS(int(group_results[2][key]), int(row[3]));
       TS_ASSERT_EQUALS(int(group_results[2][key]), int(row[4]));
       TS_ASSERT_EQUALS(group_results[3][key].get<flex_vec>(), row[5].get<flex_vec>());
     }
     TS_ASSERT_EQUALS(allkeys.size(), NUM_GROUPS);
   }


   void run_multikey_groupby_aggregate_sum_test(size_t NUM_GROUPS,
                                                size_t NUM_ROWS,
                                                size_t BUFFER_SIZE) {
     // create an SFrame with 7 columns, string, int, float, int, unused, vec
     sframe input;
     input.open_for_write({"str1","str2","int","float","int2","unused", "vector"},
                          {flex_type_enum::STRING, flex_type_enum::STRING,
                          flex_type_enum::INTEGER,
                          flex_type_enum::FLOAT, flex_type_enum::INTEGER,
                          flex_type_enum::INTEGER, flex_type_enum::VECTOR},
                          "", 4 /* 4 segments*/);
     // we will emit the following into the sframe
     // "", "0", 0, 0.0, 1, 2, {0.0, 0.0, ...}
     // "", "1", 1, 0.5, 2, 3, {1.0, 1.0, ...}
     // "", "2", 2, 1.0, 3, 4, {2.0, 2.0, ...}
     // ...
     // "9", "9", 99, 49.5, 100, 101, {99.0, 99.0, ...}
     // "10", "0", 100, 50.0, 101, 102, {100.0, 100.0, ...}
     // "10", "1", 101, 50.5, 102, 103, {101.0, 101.0, ...}
     // etc.
     // basically the string counter is split into 2 pieces. The concatenation
     // of the first two key columns should make up the actual counter value.

     // we will groupby the string columns and sum the next 3 columns, summing
     // the 4th column (int2) twice. And ignore the last column
     std::cout << "Generating input data: " << std::endl;
     std::unordered_map<std::string, flexible_type> group_results[4];
     for (size_t i = 0;i < NUM_GROUPS; ++i) {
         std::string key = std::to_string(i % NUM_GROUPS);
         group_results[0][key].reset(flex_type_enum::INTEGER);
         group_results[1][key].reset(flex_type_enum::FLOAT);
         group_results[2][key].reset(flex_type_enum::INTEGER);
         group_results[3][key].reset(flex_type_enum::VECTOR);
         group_results[3][key] = std::vector<double> (10, 0);
     }
     for (size_t i = 0;i < NUM_ROWS; ++i) {
       auto iter = input.get_output_iterator(i % 4);
       std::vector<flexible_type> flex(7);
       std::string key = std::to_string(i % NUM_GROUPS);
       flex[0] = key.substr(0, key.length() - 1);
       flex[1] = key.substr(key.length() - 1);
       flex[2] = i;
       flex[3] = (double)i / 2.0;
       flex[4] = i + 1;
       flex[5] = i + 2;
       flex[6] = std::vector<double> (10, i);
       (*iter) = flex;
       ++iter;
       group_results[0][key] += i;
       group_results[1][key] += (double)i / 2.0;
       group_results[2][key] += i + 1;
       group_results[3][key] += flex[6];
     }
     input.close();
     std::cout << "Starting multikey groupby: " << std::endl;
     turi::timer ti;
     sframe output = turi::groupby_aggregate(input,
                                       {"str1", "str2"},
                                       {"intsum", "floatsum", "", "", ""},
                                       {{{"int"}, std::make_shared<groupby_operators::sum>()},
                                       {{"float"}, std::make_shared<groupby_operators::sum>()},
                                       {{"int2"}, std::make_shared<groupby_operators::sum>()},
                                       {{"int2"}, std::make_shared<groupby_operators::sum>()},
                                       {{"vector"}, std::make_shared<groupby_operators::vector_sum>()}},
                                       BUFFER_SIZE);
     std::cout << "Groupby done in: " << ti.current_time() << " seconds" << std::endl;
     TS_ASSERT_EQUALS(output.num_columns(), 7);
     TS_ASSERT_EQUALS(output.num_rows(), NUM_GROUPS);
     TS_ASSERT_EQUALS(output.column_name(0), "str1");
     TS_ASSERT_EQUALS(output.column_name(1), "str2");
     TS_ASSERT_EQUALS(output.column_name(2), "intsum");
     TS_ASSERT_EQUALS(output.column_name(3), "floatsum");
     TS_ASSERT_EQUALS(output.column_name(4), "Sum of int2");
     TS_ASSERT_EQUALS(output.column_name(5), "Sum of int2.1");
     TS_ASSERT_EQUALS(output.column_name(6), "Vector Sum of vector");
     TS_ASSERT_EQUALS(output.column_type(0), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(1), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(2), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(3), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(4), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(5), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS(output.column_type(6), flex_type_enum::VECTOR);

     std::vector<std::vector<flexible_type> > ret;
     int rows_read = output.get_reader()->read_rows(0, output.num_rows(), ret);
     TS_ASSERT_EQUALS(rows_read, NUM_GROUPS);
     // make sure every key is covered and is unique
     std::set<std::string> allkeys;
     for(auto& row : ret) {
       std::string key = row[0] + row[1];
       allkeys.insert(key);
       TS_ASSERT_EQUALS(int(group_results[0][key]), int(row[2]));
       TS_ASSERT_EQUALS(float(group_results[1][key]), float(row[3]));
       TS_ASSERT_EQUALS(int(group_results[2][key]), int(row[4]));
       TS_ASSERT_EQUALS(int(group_results[2][key]), int(row[5]));
       TS_ASSERT_EQUALS(group_results[3][key].get<flex_vec>(), row[6].get<flex_vec>());
     }
     TS_ASSERT_EQUALS(allkeys.size(), NUM_GROUPS);
   }

   void run_groupby_aggregate_average_test(size_t NUM_GROUPS,
                                       size_t NUM_ROWS,
                                       size_t BUFFER_SIZE) {
     // create an SFrame with 6 columns, string, int, float, int,unused, vec
     sframe input;
     input.open_for_write({"str","int","float","int2","unused","vector"},
                          {flex_type_enum::STRING, flex_type_enum::FLOAT,
                          flex_type_enum::FLOAT, flex_type_enum::FLOAT,
                          flex_type_enum::FLOAT, flex_type_enum::VECTOR},
                          "", 4 /* 4 segments*/);
     // we will emit the following into the sframe
     // "0", 0, 0.0, 1, 2, {0.0,0.0,0.0,..}
     // "1", 1, 0.5, 2, 3, {1.0, 1.0, 1.0, ...}
     // "2", 2, 1.0, 3, 4, {2.0, 2.0, 2.0, ...}
     // ...
     // "99", 99, 49.5, 100, 101, {99.0, 99.0, 99.0, ...}
     // "0", 100, 50.0, 101, 102, {100.0, 100.0, ...}
     // "1", 101, 50.5, 102, 103, {101.0, 101.0, ...}
     // etc.

     // we will groupby column 0 and average the next 4 columns, averaging the 4th
     // column (int2) twice. And ignore the last column
     std::cout << "Generating input data: " << std::endl;
     std::unordered_map<std::string, flexible_type> group_results[4];
     for (size_t i = 0;i < NUM_GROUPS; ++i) {
         std::string key = std::to_string(i % NUM_GROUPS);
         group_results[0][key].reset(flex_type_enum::FLOAT);
         group_results[1][key].reset(flex_type_enum::FLOAT);
         group_results[2][key].reset(flex_type_enum::FLOAT);
         group_results[3][key].reset(flex_type_enum::VECTOR);
         group_results[3][key] = std::vector<double> (10, 0);
     }
     for (size_t i = 0;i < NUM_ROWS; ++i) {
       auto iter = input.get_output_iterator(i % 4);
       std::vector<flexible_type> flex(6);
       std::string key = std::to_string(i % NUM_GROUPS);
       flex[0] = key;
       flex[1] = i;
       flex[2] = (double)i / 2.0;
       flex[3] = i + 1;
       flex[4] = i + 2;
       flex[5] = std::vector<double> (10, i);
       (*iter) = flex;
       ++iter;
       group_results[0][key] += i;
       group_results[1][key] += (double)i / 2.0;
       group_results[2][key] += i + 1;
       group_results[3][key] += flex[5]; 
     }
     input.close();
     std::cout << "Starting groupby: " << std::endl;
     turi::timer ti;
     sframe output = turi::groupby_aggregate(input,
                                       {"str"},
                                       {"intavg","floatavg","","",""},
                                       {{{"int"}, std::make_shared<groupby_operators::average>()},
                                       {{"float"}, std::make_shared<groupby_operators::average>()},
                                       {{"int2"}, std::make_shared<groupby_operators::average>()},
                                       {{"int2"}, std::make_shared<groupby_operators::average>()},
                                       {{"vector"}, std::make_shared<groupby_operators::vector_average>()}},
                                       BUFFER_SIZE);
     std::cout << "Groupby done in: " << ti.current_time() << " seconds" << std::endl;
     TS_ASSERT_EQUALS(output.num_columns(), 6);
     TS_ASSERT_EQUALS(output.num_rows(), NUM_GROUPS);
     TS_ASSERT_EQUALS(output.column_name(0), "str");
     TS_ASSERT_EQUALS(output.column_name(1), "intavg");
     TS_ASSERT_EQUALS(output.column_name(2), "floatavg");
     TS_ASSERT_EQUALS(output.column_name(3), "Avg of int2");
     TS_ASSERT_EQUALS(output.column_name(4), "Avg of int2.1");
     TS_ASSERT_EQUALS(output.column_name(5), "Vector Avg of vector");
     TS_ASSERT_EQUALS(output.column_type(0), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(1), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(2), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(3), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(4), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(5), flex_type_enum::VECTOR);

     std::vector<std::vector<flexible_type> > ret;
     int rows_read = output.get_reader()->read_rows(0, output.num_rows(), ret);
     TS_ASSERT_EQUALS(rows_read, NUM_GROUPS);
     // make sure every key is covered and is unique
     std::set<std::string> allkeys;
     for(auto& row : ret) {
       std::string key = row[0];
       allkeys.insert(key);
       TS_ASSERT_DELTA(double(group_results[0][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[1]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[1][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[2]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[2][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[3]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[2][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[4]), 1E-5);
       flex_vec fv1 = (group_results[3][key]*(NUM_GROUPS/double(NUM_ROWS))).get<flex_vec>();
       flex_vec fv2 = row[5].get<flex_vec>();
       TS_ASSERT_EQUALS(fv1.size(), fv2.size());
       for (size_t i = 0;i <fv1.size(); ++i) {
         TS_ASSERT_DELTA(fv1[i], fv2[i] , 1E-5);
       }
     }
     TS_ASSERT_EQUALS(allkeys.size(), NUM_GROUPS);
   }


   void run_multikey_groupby_aggregate_average_test(size_t NUM_GROUPS,
                                                size_t NUM_ROWS,
                                                size_t BUFFER_SIZE) {
     // create an SFrame with 7 columns, string, string, int, float, int, unused
     sframe input;
     input.open_for_write({"str1","str2","int","float","int2","unused","vector"},
                          {flex_type_enum::STRING, flex_type_enum::STRING,
                          flex_type_enum::FLOAT,
                          flex_type_enum::FLOAT, flex_type_enum::FLOAT,
                          flex_type_enum::FLOAT, flex_type_enum::VECTOR},
                          "", 4 /* 4 segments*/);
     // we will emit the following into the sframe
     // "", "0", 0, 0.0, 1, 2, {0.0, 0.0, 0.0, ...}
     // "", "1", 1, 0.5, 2, 3, {1.0, 1.0, 1.0, ...}
     // "", "2", 2, 1.0, 3, 4, {2.0, 2.0, 2.0, ...}
     // ...
     // "9", "9", 99, 49.5, 100, 101, {99.0, 99.0, ...}
     // "10", "0", 100, 50.0, 101, 102, {100.0, ...}
     // "10", "1", 101, 50.5, 102, 103, {101.0, ...}
     // etc.
     // basically the string counter is split into 2 pieces. The concatenation
     // of the first two key columns should make up the actual counter value.

     // we will groupby the string columns and average the next 3 columns, averaging
     // the 4th column (int2) twice. And ignore the last column
     std::cout << "Generating input data: " << std::endl;
     std::unordered_map<std::string, flexible_type> group_results[4];
     for (size_t i = 0;i < NUM_GROUPS; ++i) {
         std::string key = std::to_string(i % NUM_GROUPS);
         group_results[0][key].reset(flex_type_enum::FLOAT);
         group_results[1][key].reset(flex_type_enum::FLOAT);
         group_results[2][key].reset(flex_type_enum::FLOAT);
         group_results[3][key].reset(flex_type_enum::VECTOR);
         group_results[3][key] = std::vector<double> (10,0);
     }
     for (size_t i = 0;i < NUM_ROWS; ++i) {
       auto iter = input.get_output_iterator(i % 4);
       std::vector<flexible_type> flex(7);
       std::string key = std::to_string(i % NUM_GROUPS);
       flex[0] = key.substr(0, key.length() - 1);
       flex[1] = key.substr(key.length() - 1);
       flex[2] = i;
       flex[3] = (double)i / 2.0;
       flex[4] = i + 1;
       flex[5] = i + 2;
       flex[6] = std::vector<double> (10,i);
       (*iter) = flex;
       ++iter;
       group_results[0][key] += i;
       group_results[1][key] += (double)i / 2.0;
       group_results[2][key] += i + 1;
       group_results[3][key] += flex[6];
     }
     input.close();
     std::cout << "Starting multikey groupby: " << std::endl;
     turi::timer ti;
     sframe output = turi::groupby_aggregate(input,
                                       {"str1", "str2"},
                                       {"intavg", "floatavg", "", "",""},
                                       {{{"int"}, std::make_shared<groupby_operators::average>()},
                                       {{"float"}, std::make_shared<groupby_operators::average>()},
                                       {{"int2"}, std::make_shared<groupby_operators::average>()},
                                       {{"int2"}, std::make_shared<groupby_operators::average>()},
                                       {{"vector"}, std::make_shared<groupby_operators::vector_average>()}},
                                       BUFFER_SIZE);
     std::cout << "Groupby done in: " << ti.current_time() << " seconds" << std::endl;
     TS_ASSERT_EQUALS(output.num_columns(), 7);
     TS_ASSERT_EQUALS(output.num_rows(), NUM_GROUPS);
     TS_ASSERT_EQUALS(output.column_name(0), "str1");
     TS_ASSERT_EQUALS(output.column_name(1), "str2");
     TS_ASSERT_EQUALS(output.column_name(2), "intavg");
     TS_ASSERT_EQUALS(output.column_name(3), "floatavg");
     TS_ASSERT_EQUALS(output.column_name(4), "Avg of int2");
     TS_ASSERT_EQUALS(output.column_name(5), "Avg of int2.1");
     TS_ASSERT_EQUALS(output.column_name(6), "Vector Avg of vector");
     TS_ASSERT_EQUALS(output.column_type(0), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(1), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(output.column_type(2), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(3), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(4), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(5), flex_type_enum::FLOAT);
     TS_ASSERT_EQUALS(output.column_type(6), flex_type_enum::VECTOR);
     
     std::vector<std::vector<flexible_type> > ret;
     int rows_read = output.get_reader()->read_rows(0, output.num_rows(), ret);
     TS_ASSERT_EQUALS(rows_read, NUM_GROUPS);
     // make sure every key is covered and is unique
     std::set<std::string> allkeys;
     for(auto& row : ret) {
       std::string key = row[0] + row[1];
       allkeys.insert(key);
       TS_ASSERT_DELTA(double(group_results[0][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[2]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[1][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[3]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[2][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[4]), 1E-5);
       TS_ASSERT_DELTA(double(group_results[2][key])*(NUM_GROUPS/double(NUM_ROWS)), double(row[5]), 1E-5);

       flex_vec fv1 = (group_results[3][key]*(NUM_GROUPS/double(NUM_ROWS))).get<flex_vec>();
       flex_vec fv2 = row[6].get<flex_vec>();
       TS_ASSERT_EQUALS(fv1.size(), fv2.size());
       for (size_t i = 0;i <fv1.size(); ++i) {
         TS_ASSERT_DELTA(fv1[i], fv2[i] , 1E-5);
       }
     }
     TS_ASSERT_EQUALS(allkeys.size(), NUM_GROUPS);
   }


   void test_sframe_groupby_aggregate() {
     //small number of groups
     run_groupby_aggregate_sum_test(100, 100000, 100);
     run_groupby_aggregate_average_test(100, 100000, 100);
     //big buffer
     run_groupby_aggregate_sum_test(100, 100000, 1000);
     run_groupby_aggregate_average_test(100, 100000, 1000);
     //very small data
     run_groupby_aggregate_sum_test(10, 100, 1000);
     run_groupby_aggregate_average_test(10, 100, 1000);
     //very small buffer
     run_groupby_aggregate_sum_test(1000, 100000, 10);
     run_groupby_aggregate_average_test(1000, 100000, 10);
     //very very small buffer
     run_groupby_aggregate_sum_test(100000, 100000, 2);
     run_groupby_aggregate_average_test(100000, 100000, 2);
   
   }

   void test_sframe_multikey_groupby_aggregate() {
     //small number of groups
     run_multikey_groupby_aggregate_sum_test(100, 100000, 100);
     run_multikey_groupby_aggregate_average_test(100, 100000, 100);
     //big buffer
     run_multikey_groupby_aggregate_sum_test(100, 100000, 1000);
     run_multikey_groupby_aggregate_average_test(100, 100000, 1000);
     //very small data
     run_multikey_groupby_aggregate_sum_test(10, 100, 1000);
     run_multikey_groupby_aggregate_average_test(10, 100, 1000);
     //very small buffer
     run_multikey_groupby_aggregate_sum_test(1000, 100000, 10);
     run_multikey_groupby_aggregate_average_test(1000, 100000, 10);
     //very very small buffer
     run_multikey_groupby_aggregate_sum_test(100000, 100000, 2);
     run_multikey_groupby_aggregate_average_test(100000, 100000, 2);
  
   }

   void test_sframe_groupby_aggregate_negative_tests() {
     sframe input;
     input.open_for_write({"str","int","float"},
                          {flex_type_enum::STRING, flex_type_enum::INTEGER,
                          flex_type_enum::FLOAT},
                          "", 4 /* 4 segments*/);
     // we will emit the following into the sframe
     // "0", 0, 0.0
     // "1", 1, 1.0
     // "2", 2, 2.0
     // ...
     // "9", 99, 99.0
     // "0", 100, 100.0
     // "1", 101, 101.1
     // etc.
     // actual data doesn't really matter. This is just data for negative tests
     std::cout << "Generating input data: " << std::endl;
     for (size_t i = 0;i < 1000 ; ++i) {
       auto iter = input.get_output_iterator(i % 4);
       std::vector<flexible_type> flex(3);
       std::string key = std::to_string(i % 10);
       flex[0] = key;
       flex[1] = i;
       flex[2] = i;
       (*iter) = flex;
       ++iter;
     }
     input.close();
     // sum on strings shall fail
     TS_ASSERT_THROWS_ANYTHING(turi::groupby_aggregate(input,
                             {"int"},
                             {""},
                             {{{"str"}, std::make_shared<groupby_operators::sum>()}}));
     // multiple identical keys
     TS_ASSERT_THROWS_ANYTHING(turi::groupby_aggregate(input,
                             {"str", "str"},
                             {""},
                             {{{"int"}, std::make_shared<groupby_operators::sum>()}}));
     // nonexistant column
     TS_ASSERT_THROWS_ANYTHING(turi::groupby_aggregate(input,
                             {"pika", "str"},
                             {""},
                             {{{"int"}, std::make_shared<groupby_operators::sum>()}}));
     // nonexistant column
     TS_ASSERT_THROWS_ANYTHING(turi::groupby_aggregate(input,
                             {"str"},
                             {""},
                             {{{"pika"}, std::make_shared<groupby_operators::sum>()}}));
   }


   void run_sframe_aggregate_operators_test(std::shared_ptr<group_aggregate_value> val,
                                            const std::vector<size_t>& vals,
                                            const std::vector<flex_type_enum>& input_types,
                                            size_t expected_result) {
     std::vector<group_aggregate_value*> parallel_vals;
     auto ret = val->set_input_types(input_types);
     TS_ASSERT_EQUALS(ret, flex_type_enum::INTEGER);
     // make a collection of partial aggregators
     for (size_t i = 0;i < 4; ++i) {
       parallel_vals.push_back(val->new_instance());
     }
     for (size_t i = 0;i < 4; ++i) {
       auto parallel_vals_i_raw = parallel_vals[i];
       auto val_raw = val.get();
       TS_ASSERT(typeid(*parallel_vals_i_raw) == typeid(*val_raw));
     }
     // perform the partial aggregation
     for (size_t i = 0; i < vals.size(); ++i) {
       parallel_vals[i % 4]->add_element({vals[i]});
     }
     // combine the partial aggregates
     for (size_t i = 1; i < parallel_vals.size(); ++i) {
       parallel_vals[0]->combine(*parallel_vals[i]);
     }
     // check if values are good
     flexible_type final_val = parallel_vals[0]->emit();
     TS_ASSERT_EQUALS(final_val.get_type(), flex_type_enum::INTEGER);
     TS_ASSERT_EQUALS((size_t)final_val, expected_result);
     for (size_t i = 0;i < 4; ++i) {
       delete parallel_vals[i];
     }
   }

   void test_sframe_aggregate_operators() {
     std::vector<size_t> vals;
     for (size_t i = 0;i < 100000; ++i) vals.push_back(i);

     size_t min = vals[0], max = vals[0], count = 0, sum = 0;
     for (size_t val: vals) {
       min = std::min(min, val);
       max = std::max(max, val);
       ++count;
       sum += val;
     }

     run_sframe_aggregate_operators_test(std::make_shared<groupby_operators::sum>(),
                                         vals,
                                         {flex_type_enum::INTEGER},
                                         sum);
     run_sframe_aggregate_operators_test(std::make_shared<groupby_operators::min>(),
                                         vals,
                                         {flex_type_enum::INTEGER},
                                         min);
     run_sframe_aggregate_operators_test(std::make_shared<groupby_operators::max>(),
                                         vals,
                                         {flex_type_enum::INTEGER},
                                         max);
     run_sframe_aggregate_operators_test(std::make_shared<groupby_operators::count>(),
                                         vals,
                                         {},
                                         count);
   }


   void append_some_data_to_sframe(sframe& sframe_out) {
     std::vector<flexible_type> int_col{0,1,2,3,4,5};
     std::vector<flexible_type> float_col{0,1,2,3,4,5};
     std::vector<flexible_type> str_col{"0","1","2","3","4","5"};
     dataframe_t df;
     df.set_column("int_col", int_col, flex_type_enum::INTEGER);
     df.set_column("float_col", float_col, flex_type_enum::FLOAT);
     df.set_column("str_col", str_col, flex_type_enum::STRING);
     sframe sf(df);
     sframe_out = sframe_out.append(sf);
     // make sure sf is still accessible
     std::vector<std::vector<flexible_type> > result;
     turi::copy(sf, std::inserter(result, result.end()));
     TS_ASSERT_EQUALS(result.size(), 6);
     for (size_t i = 0;i < result.size(); ++i) {
       TS_ASSERT_EQUALS(result[i][0], i);
       TS_ASSERT_EQUALS(result[i][1], (double)i);
       TS_ASSERT_EQUALS(result[i][2], std::to_string(i));
     }
   }


   void test_sframe_append() {
     // Create an sframe
     sframe sframe_out;
     sframe frame2;

     append_some_data_to_sframe(frame2);
     sframe_out = sframe_out.append(frame2);

     // check that the copy is good
     TS_ASSERT_EQUALS(sframe_out.size(), 6);
     TS_ASSERT_EQUALS(sframe_out.num_columns(), 3);
     std::vector<std::vector<flexible_type> > result;
     turi::copy(sframe_out, std::inserter(result, result.end()));
     TS_ASSERT_EQUALS(result.size(), 6);
     for (size_t i = 0;i < result.size(); ++i) {
       TS_ASSERT_EQUALS(result[i][0], i);
       TS_ASSERT_EQUALS(result[i][1], (double)i);
       TS_ASSERT_EQUALS(result[i][2], std::to_string(i));
     }

     // check that frame2 is still good
     TS_ASSERT_EQUALS(frame2.size(), 6);
     TS_ASSERT_EQUALS(frame2.num_columns(), 3);
     result.clear();
     turi::copy(frame2, std::inserter(result, result.end()));
     TS_ASSERT_EQUALS(result.size(), 6);
     for (size_t i = 0;i < result.size(); ++i) {
       TS_ASSERT_EQUALS(result[i][0], i);
       TS_ASSERT_EQUALS(result[i][1], (double)i);
       TS_ASSERT_EQUALS(result[i][2], std::to_string(i));
     }

     // do it again

     append_some_data_to_sframe(sframe_out);

     // check that the move is good
     TS_ASSERT_EQUALS(sframe_out.size(), 2 * 6);
     TS_ASSERT_EQUALS(sframe_out.num_columns(), 3);
     result.clear();
     turi::copy(sframe_out, std::inserter(result, result.end()));
     TS_ASSERT_EQUALS(result.size(), 2 * 6);
     for (size_t i = 0;i < result.size(); ++i) {
       TS_ASSERT_EQUALS(result[i][0], i % 6);
       TS_ASSERT_EQUALS(result[i][1], (double)(i % 6));
       TS_ASSERT_EQUALS(result[i][2], std::to_string(i % 6));
     }
   }
   void test_sarray_recursive_append(void) {
     std::vector<flexible_type> int_col{0};
     std::vector<flexible_type> float_col{0};
     std::vector<flexible_type> str_col{"0"};
     dataframe_t df;
     df.set_column("int_col", int_col, flex_type_enum::INTEGER);
     df.set_column("float_col", float_col, flex_type_enum::FLOAT);
     df.set_column("str_col", str_col, flex_type_enum::STRING);
     sframe sf(df);

     for (size_t i = 0;i < 20; ++i) {
       sf = sf.append(sf);
     }
     TS_ASSERT_EQUALS(sf.size(), 1048576);
     auto reader = sf.get_reader();
     sframe_rows rows;
     reader->read_rows(0, 1048576, rows);
     TS_ASSERT_EQUALS(rows.num_rows(), 1048576);
     for (auto& row: rows) {
       TS_ASSERT_EQUALS(row[0], int_col[0]);
       TS_ASSERT_EQUALS(row[1], float_col[0]);
       TS_ASSERT_EQUALS(row[2], str_col[0]);
     }
   }

   void test_sframe_rows() {
     std::vector<std::vector<flexible_type> > data{{1,2,3,4,5},
                                                   {6,7,8,9,10},
                                                   {11,12,13,14,15},
                                                   {16,17,18,19,20}};
     sframe_rows rows;
     rows.clear();
     rows.add_decoded_column(std::make_shared<std::vector<flexible_type>>(data[0]));
     TS_ASSERT_EQUALS(rows.num_rows(), 5);
     TS_ASSERT_EQUALS(rows.num_columns(), 1);
     size_t i = 0;
     for (auto& row: rows) {
       TS_ASSERT_EQUALS(row[0], data[0][i]);
       ++i;
     }
   }
   void test_sframe_ndarray() {
     flex_nd_vec fortran({0,5,1,6,2,7,3,8,4,9},
                          {2,5},
                          {1,2});
     flex_nd_vec c({0,1,2,3,4,5,6,7,8,9},
                          {2,5},
                          {5,1});
     flex_nd_vec subarray({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                          {2,2},
                          {1,4}); // top left corner of array
     flex_nd_vec subarray2({0,1,2,3,
                            4,5,6,7,
                            8,9,10,11,
                            12,13,14,15,16},
                          {2,2},
                          {1,4},
                          2); // top right corner of array
     flex_nd_vec zero_stride({0,1,2,3,4,5,6,7,8,9},
                             {2,5},
                             {1,0});
     flex_nd_vec d4({0,2,4,1,3,5},
                     {3,1,1,2},
                     {1,0,0,3}); 

     std::vector<flexible_type> values{fortran,c,subarray,subarray2,zero_stride,d4};
     std::string fname = get_temp_name() + ".sidx";

     sarray<flexible_type> sa;
     sa.open_for_write(fname, 1);
     sa.set_type(flex_type_enum::ND_VECTOR);
     auto iter = sa.get_output_iterator(0);
     std::copy(values.begin(), values.end(), iter);
     sa.close();

     TS_ASSERT_EQUALS(sa.size(), values.size());
     // now check for reversibility by reading it back
     for(auto& v: values) {
       v.mutable_get<flex_nd_vec>() = v.get<flex_nd_vec>().compact();
     }
     sframe_rows rows;
     sa.get_reader()->read_rows(0, sa.size(), rows);

     auto values_iter = values.begin();
     for (const auto& ret: rows) {
       TS_ASSERT_EQUALS(ret[0] == *values_iter, true);
       ++values_iter;
     }
   }
};

BOOST_FIXTURE_TEST_SUITE(_sframe_test, sframe_test)
BOOST_AUTO_TEST_CASE(test_sframe_construction) {
  sframe_test::test_sframe_construction();
}
BOOST_AUTO_TEST_CASE(test_empty_sframe) {
  sframe_test::test_empty_sframe();
}
BOOST_AUTO_TEST_CASE(test_sframe_save) {
  sframe_test::test_sframe_save();
}
BOOST_AUTO_TEST_CASE(test_sframe_save_reference_no_copy) {
  sframe_test::test_sframe_save_reference_no_copy();
}
BOOST_AUTO_TEST_CASE(test_sframe_save_reference_one_copy) {
  sframe_test::test_sframe_save_reference_one_copy();
}
BOOST_AUTO_TEST_CASE(test_sframe_save_empty_columns) {
  sframe_test::test_sframe_save_empty_columns();
}
BOOST_AUTO_TEST_CASE(test_sframe_save_really_empty) {
  sframe_test::test_sframe_save_really_empty();
}
BOOST_AUTO_TEST_CASE(test_sframe_dataframe_conversion) {
  sframe_test::test_sframe_dataframe_conversion();
}
BOOST_AUTO_TEST_CASE(test_sframe_dataframe_conversion_with_na) {
  sframe_test::test_sframe_dataframe_conversion_with_na();
}
BOOST_AUTO_TEST_CASE(test_sframe_iterate) {
  sframe_test::test_sframe_iterate();
}
BOOST_AUTO_TEST_CASE(test_sframe_logical_segments) {
  sframe_test::test_sframe_logical_segments();
}
BOOST_AUTO_TEST_CASE(test_sframe_write) {
  sframe_test::test_sframe_write();
}
BOOST_AUTO_TEST_CASE(test_select_column) {
  sframe_test::test_select_column();
}
BOOST_AUTO_TEST_CASE(test_add_column) {
  sframe_test::test_add_column();
}
BOOST_AUTO_TEST_CASE(test_column_name_wrangling) {
  sframe_test::test_column_name_wrangling();
}
BOOST_AUTO_TEST_CASE(test_sframe_groupby_aggregate) {
  sframe_test::test_sframe_groupby_aggregate();
}
BOOST_AUTO_TEST_CASE(test_sframe_multikey_groupby_aggregate) {
  sframe_test::test_sframe_multikey_groupby_aggregate();
}
BOOST_AUTO_TEST_CASE(test_sframe_groupby_aggregate_negative_tests) {
  sframe_test::test_sframe_groupby_aggregate_negative_tests();
}
BOOST_AUTO_TEST_CASE(test_sframe_aggregate_operators) {
  sframe_test::test_sframe_aggregate_operators();
}
BOOST_AUTO_TEST_CASE(test_sframe_append) {
  sframe_test::test_sframe_append();
}
BOOST_AUTO_TEST_CASE(test_sframe_rows) {
  sframe_test::test_sframe_rows();
}
BOOST_AUTO_TEST_CASE(test_sframe_ndarray) {
  sframe_test::test_sframe_ndarray();
}
BOOST_AUTO_TEST_SUITE_END()
