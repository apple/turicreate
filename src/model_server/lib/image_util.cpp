/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/image_util.hpp>

#include <algorithm>
#include <string>

#include <core/data/image/image_util_impl.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/query_engine/util/aggregates.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>

namespace turi{

namespace image_util {

template <typename T>
void copy_image_to_memory_impl(const image_type& input, T* outptr,
                               const std::vector<size_t>& outstrides,
                               const std::vector<size_t>& outshape,
                               bool channel_last) {
  ASSERT_EQ(outstrides.size(), 3);
  ASSERT_EQ(outshape.size(), 3);
  size_t stride_h, stride_w, stride_c;
  size_t height, width, channels;
  if (channel_last) {
    // Format: HWC
    stride_h = outstrides[0];
    stride_w = outstrides[1];
    stride_c = outstrides[2];
    height = outshape[0];
    width = outshape[1];
    channels = outshape[2];
  } else {
    // Format: CHW
    stride_c = outstrides[0];
    stride_h = outstrides[1];
    stride_w = outstrides[2];
    channels = outshape[0];
    height = outshape[1];
    width = outshape[2];
  }

  // Resize.
  flexible_type resized = image_util::resize_image(input, width, height,
                                                   channels, /* decode */ true);
  const image_type& img = resized.get<flex_image>();

  // Copy.
  size_t cnt = 0;
  const unsigned char* raw_data = img.get_image_data();
  for (size_t i = 0; i < img.m_height; ++i) {
    for (size_t j = 0; j < img.m_width; ++j) {
      for (size_t k = 0; k < img.m_channels; ++k) {
        outptr[i * stride_h + j * stride_w + k * stride_c] =
            static_cast<T>(raw_data[cnt++]);
      }
    }
  }

  // Further optimization is possible (but not trivial) by combining the resize
  // operation and the copy operation, removing an intermediate buffer.
}

void copy_image_to_memory(const image_type& input, float* outptr,
                          const std::vector<size_t>& outstrides,
                          const std::vector<size_t>& outshape,
                          bool channel_last) {
  copy_image_to_memory_impl(input, outptr, outstrides, outshape, channel_last);
}

void copy_image_to_memory(const image_type& input, unsigned char* outptr,
                          const std::vector<size_t>& outstrides,
                          const std::vector<size_t>& outshape,
                          bool channel_last) {
  copy_image_to_memory_impl(input, outptr, outstrides, outshape, channel_last);
}

  /**
  * Return flex_vec flexible type that is sum of all images with data in vector form.
  */

  flexible_type sum(std::shared_ptr<unity_sarray> unity_data){
    log_func_entry();

    if (unity_data->size() > 0){
      bool failure = false;
      size_t reference_size;
      size_t failure_size;
      auto reductionfn =
        [&failure, &reference_size, &failure_size]
        (const flexible_type& in, std::pair<bool, flexible_type>& sum)->bool {
          if (in.get_type() != flex_type_enum::UNDEFINED) {
            flexible_type tmp_img = in;
            image_util_detail::decode_image_impl(tmp_img.mutable_get<flex_image>());
            flexible_type f(flex_type_enum::VECTOR);
            f.soft_assign(tmp_img);
            if (sum.first == false) {
              // initial val
              sum.first = true;
              sum.second = f;
            } else if (sum.second.size() == f.size()) {
              // accumulation
              sum.second += f;
            } else {
              // length mismatch. fail
              failure = true;
              reference_size = sum.second.size();
              failure_size = f.size();
              return false;
            }
          }
          return true;
        };

      auto combinefn =
        [&failure, &reference_size, &failure_size]
        (const std::pair<bool, flexible_type>& f,
         std::pair<bool, flexible_type>& sum)->bool {
          if (sum.first == false) {
            // initial state
            sum = f;
          } else if (f.first == false) {
            // there is no f to add.
            return true;
          } else if (sum.second.size() == f.second.size()) {
            // accumulation
            sum.second += f.second;
          } else {
            // length mismatch
            failure = true;
            reference_size = sum.second.size();
            failure_size = f.second.size();
            return false;
          }
          return true;
        };
      std::pair<bool, flexible_type> start_val{false, flex_vec()};
      std::pair<bool, flexible_type> sum_val =
        query_eval::reduce<std::pair<bool, flexible_type>>(unity_data->get_planner_node(),
                                                           reductionfn,
                                                           combinefn,
                                                           start_val);

      if(failure){
        std::string error =
            std::string("Cannot perform sum or average over images of different sizes. ")
            + std::string("Found images of total size (ie. width * height * channels) ")
            + std::string(" of both ") + std::to_string(reference_size)
            + std::string( "and ") + std::to_string(failure_size)
            + std::string(". Please use graplab.image_analysis.resize() to make images a uniform")
            + std::string(" size.");
        log_and_throw(error);
      }
      return sum_val.second;
    }

    log_and_throw("Input image sarray is empty");
    __builtin_unreachable();
  }


/**
 * Construct an image of mean pixel values from a pointer to a unity_sarray.
 */

flexible_type generate_mean(std::shared_ptr<unity_sarray> unity_data) {
  log_func_entry();


  std::vector<flexible_type> sample_img = unity_data->_head(1);
  flex_image meta_img;
  meta_img = sample_img[0];
  size_t channels = meta_img.m_channels;
  size_t height = meta_img.m_height;
  size_t width = meta_img.m_width;
  size_t image_size = width * height * channels;
  size_t num_images = unity_data->size();

  //Perform sum
  flexible_type mean = sum(unity_data);

  //Divide for mean images.
  mean /= num_images;

  flexible_type ret;
  flex_image img;
  char* data_bytes = new char[image_size];
  for(size_t i = 0; i < image_size; ++i){
    data_bytes[i] = static_cast<unsigned char>(mean[i]);
  }
  img.m_image_data_size = image_size;
  img.m_channels = channels;
  img.m_height = height;
  img.m_width = width;
  img.m_image_data.reset(data_bytes);
  img.m_version = IMAGE_TYPE_CURRENT_VERSION;
  img.m_format = Format::RAW_ARRAY;
  ret = img;
  return ret;
}


/**
 * Construct a single image from url, and format hint.
 */
flexible_type load_image(const std::string& url, const std::string format) {
  flexible_type ret(flex_type_enum::IMAGE);
  ret = read_image(url, format);
  return ret;
};

namespace {

size_t load_images_impl(std::vector<std::string>& all_files,
                        sarray<flexible_type>::iterator& image_iter,
                        sarray<flexible_type>::iterator& path_iter,
                        const std::string& format,
                        bool with_path, bool ignore_failure, size_t thread_id) {
  timer mytimer;

  atomic<size_t> cnt = 0;
  double previous_time = 0;
  int previous_cnt = 0;
  bool cancel = false;

  auto iter = all_files.begin();
  auto end = all_files.end();
  while(iter < end && !cancel) {
    flexible_type img(flex_type_enum::IMAGE);
    // read a single image
    try {
      img = read_image(*iter, format);
      *image_iter = img;
      ++image_iter;
      if (with_path) {
        *path_iter = *iter;
        ++path_iter;
      }
      ++cnt;
    } catch (std::string error) {
      logprogress_stream << error << "\t" << " file: " << sanitize_url(*iter) << std::endl;
      if (!ignore_failure)
        throw;
    } catch (const char* error) {
      logprogress_stream << error << "\t" << " file: " << sanitize_url(*iter) << std::endl;
      if (!ignore_failure)
        throw;
    } catch (...) {
      logprogress_stream << "Unknown error reading image \t file: " << sanitize_url(*iter) << std::endl;
      if (!ignore_failure)
        throw;
    }
    ++iter;
    // output progress
    if (thread_id == 0) {
      double current_time = mytimer.current_time();
      size_t current_cnt = cnt;
      if (current_time - previous_time > 5) {
        logprogress_stream << "Read " << current_cnt << " images in " << current_time << " secs\t"
          << "speed: " << double(current_cnt - previous_cnt) / (current_time - previous_time)
          << " file/sec" << std::endl;
        previous_time = current_time;
        previous_cnt = current_cnt;
      }
    }
    // check for user interrupt ctrl-C
    if (cppipc::must_cancel()) {
      cancel = true;
    }
  } // end of while

  if (cancel)  {
    log_and_throw("Cancelled by user");
  }
  return cnt;
}

std::vector<std::string> get_directory_files(std::string url, bool recursive) {
  typedef std::vector<std::pair<std::string, fileio::file_status>> path_status_vec_t;
  path_status_vec_t path_status_vec = fileio::get_directory_listing(url);
  std::vector<std::string> ret;
  for (const auto& path_status : path_status_vec) {
    if (recursive && path_status.second == fileio::file_status::DIRECTORY) {
      auto tmp = get_directory_files(path_status.first, recursive);
      ret.insert(ret.end(), tmp.begin(), tmp.end());
    } else if (path_status.second == fileio::file_status::REGULAR_FILE){
      ret.push_back(path_status.first);
    }
  }
  return ret;
}

bool lacks_image_extension(const std::string& url) {

  // Return true unless the url ends with any of these strings.
  const std::initializer_list<const char*> extensions =
      {".jpg", ".jpeg", ".png"};

  // Define a predicate (over extensions) that performs case-insensitive
  // matching against the url.
  auto ends_url = [&url](const char* extension) {
    return boost::algorithm::iends_with(url, extension);
  };

  return std::none_of(extensions.begin(), extensions.end(), ends_url);
}

}  // namespace

/**
 * Construct an sframe of flex_images, with url pointing to directory where images reside.
 */
std::shared_ptr<unity_sframe> load_images(std::string url, std::string format, bool with_path, bool recursive,
                                          bool ignore_failure, bool random_order) {
    log_func_entry();

    std::vector<std::string> all_files;

    // See what's at the user-provided location.
    auto status = fileio::get_file_status(url);
    switch (status.first) {

    case fileio::file_status::MISSING:
      log_and_throw_io_failure(sanitize_url(url) + " not found. Err: " + status.second);

    case fileio::file_status::REGULAR_FILE:
      all_files.push_back(url);
      break;

    case fileio::file_status::DIRECTORY:
      all_files = get_directory_files(url, recursive);
      if (format != "JPG" && format != "PNG") {
        // We will deduce file formats from file extensions. Prune the list of
        // files to those supported.
        auto first_to_remove = std::remove_if(
            all_files.begin(), all_files.end(), lacks_image_extension);
        if (first_to_remove == all_files.begin() && !all_files.empty()) {
          logprogress_stream << "Directory " << sanitize_url(url)
                             << " does not contain any files with supported"
                             << " image extensions: .jpg .jpeg .png"
                             << std::endl;
        }
        all_files.erase(first_to_remove, all_files.end());
      }
      break;

    case fileio::file_status::FS_UNAVAILABLE:
      log_and_throw_io_failure("Error getting file system status for "
                               + sanitize_url(url) + ". Err: " + status.second);
    }

    std::vector<std::string> column_names;
    std::vector<flex_type_enum> column_types;

    sframe image_sframe;
    auto path_sarray = std::make_shared<sarray<flexible_type>>();
    auto image_sarray = std::make_shared<sarray<flexible_type>>();

    // Parallel read dioes not seems to help, and it slow IO down when there is only one disk.
    // We can expose this option in the future for parallel disk IO or RAID.
    // size_t num_threads = thread_pool::get_instance().size();
    size_t num_threads = 1;
    column_names = {"path","image"};
    path_sarray->open_for_write(num_threads + 1); // open one more segment for appending recursive results
    image_sarray->open_for_write(num_threads + 1); // ditto
    path_sarray->set_type(flex_type_enum::STRING);
    image_sarray->set_type(flex_type_enum::IMAGE);

    if (random_order) {
      std::random_shuffle(all_files.begin(), all_files.end());
    } else {
      std::sort(all_files.begin(), all_files.end());
    }

    size_t files_per_thread = all_files.size() / num_threads;
    parallel_for(0, num_threads, [&](size_t thread_id) {
      auto path_iter = path_sarray->get_output_iterator(thread_id);
      auto image_iter = image_sarray->get_output_iterator(thread_id);

      size_t begin = files_per_thread * thread_id;
      size_t end = (thread_id + 1) == num_threads ? all_files.size() : begin + files_per_thread;

      std::vector<std::string> subset(all_files.begin() + begin, all_files.begin() + end);
      load_images_impl(subset, image_iter, path_iter, format, with_path, ignore_failure, thread_id);
    });

    image_sarray->close();
    path_sarray->close();

    if (with_path){
      std::vector<std::shared_ptr<sarray<flexible_type> > > sframe_columns {path_sarray, image_sarray};
      image_sframe = sframe(sframe_columns, {"path","image"});
    } else {
      std::vector<std::shared_ptr<sarray<flexible_type> > > sframe_columns {image_sarray};
      image_sframe = sframe(sframe_columns, {"image"});
    }

    std::shared_ptr<unity_sframe> image_unity_sframe(new unity_sframe());
    image_unity_sframe->construct_from_sframe(image_sframe);

    return image_unity_sframe;
}

/**
 * Decode the image into raw pixels
 */
flexible_type decode_image(const flexible_type& image) {
  if (image.get<flex_image>().is_decoded()) {
    return image;
  }
  flexible_type ret = image;
  flex_image& img = ret.mutable_get<flex_image>();
  turi::decode_image_inplace(img);
  return ret;
};

/**
 * Encode the image into compressed format.(losslessly)
 */
flexible_type encode_image(const flexible_type& image) {
  if (!image.get<flex_image>().is_decoded()) {
    return image;
  }
  flexible_type ret = image;
  flex_image& img = ret.mutable_get<flex_image>();
  turi::encode_image_inplace(img);
  return ret;
};


/**
 * Decode an sarray of flex_images into raw pixels
 */
std::shared_ptr<unity_sarray> decode_image_sarray(std::shared_ptr<unity_sarray> image_sarray) {
  auto fn = [](const flexible_type& f)->flexible_type {
              return decode_image(f);
            };
  auto ret = image_sarray->transform_lambda(fn, flex_type_enum::IMAGE, true, 0);
  return std::static_pointer_cast<unity_sarray>(ret);
};

/**
 * Reisze an sarray of flex_images with the new size.
 */
flexible_type resize_image(const flexible_type& input, size_t resized_width,
			   size_t resized_height, size_t resized_channels,
			   bool decode, int resample_method) {
  if (input.get_type() != flex_type_enum::IMAGE){
    std::string error = "Cannot resize non-image type";
    log_and_throw(error);
  }
  flex_image image = input.get<flex_image>();
  auto has_desired_size = [&] {
    return image.m_width == resized_width && image.m_height == resized_height && image.m_channels == resized_channels;
  };

  // Is this resize a no-op?
  if (has_desired_size() && image.is_decoded() == decode) {
    return input;
  }

  // Decode if necessary.
  if (!image.is_decoded()) {
    image_util_detail::decode_image_impl(image);
  }

  // Resize if necessary.
  if (!has_desired_size()) {
    char* resized_data;
    image_util_detail::resize_image_impl(
        reinterpret_cast<const char*>(image.get_image_data()),
	image.m_width, image.m_height, image.m_channels,
	resized_width, resized_height, resized_channels,
	&resized_data, resample_method);
    image.m_width = resized_width;
    image.m_height = resized_height;
    image.m_channels = resized_channels;
    image.m_format = Format::RAW_ARRAY;
    image.m_image_data_size = resized_height * resized_width * resized_channels;
    image.m_image_data.reset(resized_data);
  }

  // Encode if necessary.
  if (!decode) {
    image_util_detail::encode_image_impl(image);
  }

  return image;
};


/**
 * Resize an sarray of flex_image with the new size.
 */
std::shared_ptr<unity_sarray> resize_image_sarray(
    std::shared_ptr<unity_sarray> image_sarray,
    size_t resized_width,
    size_t resized_height,
    size_t resized_channels,
    bool decode,
    int resample_method) {
  log_func_entry();
  auto fn = [=](const flexible_type& f)->flexible_type {
      return flexible_type(resize_image(f, resized_width, resized_height, resized_channels, decode, resample_method));
    };
  auto ret = image_sarray->transform_lambda(fn, flex_type_enum::IMAGE, true, 0);
  return std::static_pointer_cast<unity_sarray>(ret);
};

/**
 * Convert sarray of image data to sarray of vector
 */
std::shared_ptr<unity_sarray> image_sarray_to_vector_sarray(
    std::shared_ptr<unity_sarray> image_sarray,
    bool undefined_on_failure) {
  // decoded_image_sarray
  log_func_entry();

  // transform the array with type casting
  auto fn = [=](const flexible_type& f)->flexible_type {
              flexible_type ret(flex_type_enum::VECTOR);
              flex_image tmp_img = f;
              image_util_detail::decode_image_impl(tmp_img);
              try {
                ret = tmp_img;
              } catch (...) {
                if (undefined_on_failure) {
                  ret = FLEX_UNDEFINED;
                } else {
                  throw;
                }
              }
              return ret;
            };
  auto ret = image_sarray->transform_lambda(fn, flex_type_enum::VECTOR, true, 0);
  return std::static_pointer_cast<unity_sarray>(ret);
};

/**
 * Convert sarray of vector to sarray of image
 */
std::shared_ptr<unity_sarray> vector_sarray_to_image_sarray(
    std::shared_ptr<unity_sarray> image_sarray,
    size_t width, size_t height, size_t channels,
    bool undefined_on_failure) {
  log_func_entry();
  size_t expected_array_size = height * width * channels;
  auto transformfn =
      [height,width,channels,
      expected_array_size,undefined_on_failure](const flex_vec& vec)->flexible_type {
        try {
          if (expected_array_size != vec.size()) {
            logprogress_stream << "Dimensions do not match vec size" << std::endl;
            log_and_throw("Dimensions do not match vec size");
          }
          flexible_type ret;
          flex_image img;
          size_t data_size = vec.size();
          char* data = new char[data_size];
          for (size_t i = 0; i < data_size; ++i){
            data[i] = static_cast<char>(vec[i]);
          }
          img.m_image_data_size = data_size;
          img.m_image_data.reset(data);
          img.m_height = height;
          img.m_width = width;
          img.m_channels = channels;
          img.m_format = Format::RAW_ARRAY;
          img.m_version = 0;
          ret = img;
          return ret;
        } catch (...) {
          if (undefined_on_failure) {
            return FLEX_UNDEFINED;
          } else {
            throw;
          }
        }
      };

  auto ret = image_sarray->transform_lambda(transformfn,
                                            flex_type_enum::IMAGE, true, 0);
  return std::static_pointer_cast<unity_sarray>(ret);
};

}  // namespace image_util
} //namespace turi
