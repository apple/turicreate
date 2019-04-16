/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <regex>
#include <vector>
#include <map>
#include <set>
#include <parallel/mutex.hpp>
#include <boost/algorithm/string.hpp>
#include <logger/logger.hpp>
#include <timer/timer.hpp>
#include <parallel/thread_pool.hpp>
#include <parallel/atomic.hpp>
#include <flexible_type/flexible_type.hpp>
#include <sframe/sframe.hpp>
#include <sframe/parallel_csv_parser.hpp>
#include <sframe/csv_line_tokenizer.hpp>
#include <fileio/general_fstream.hpp>
#include <fileio/sanitize_url.hpp>
#include <fileio/fs_utils.hpp>
#include <cppipc/server/cancel_ops.hpp>
#include <sframe/sframe_constants.hpp>
#include <util/dense_bitset.hpp>

namespace turi {

using fileio::file_status;

/**
 * Escape BOMs
 */
void skip_BOM(general_ifstream& fin) {
  char fChar, sChar, tChar;
  size_t count = 0;
  fChar = fin.get();
  count = fin.gcount();
  if (!fin.good()) {
    if (count == 1) {
      fin.putback(fChar);
      fin.clear(std::ios_base::goodbit);
    } else {
      DASSERT_EQ(count, 0);
    }
    return;
  }
  DASSERT_EQ(count, 1);
  sChar = fin.get();
  count = fin.gcount();
  if (!fin.good()) {
    fin.clear(std::ios_base::goodbit);
    if (count == 1) {
      fin.putback(sChar);
    } else {
      DASSERT_EQ(count, 0);
    }
    fin.putback(fChar);
    return;
  }
  DASSERT_EQ(count, 1);
  tChar = fin.get();
  count = fin.gcount();
  if (fin.bad() || count == 0) {
    fin.clear(std::ios_base::goodbit);
    if (count == 1) {
      fin.putback(tChar);
    } else {
      DASSERT_EQ(count, 0);
    }
    fin.putback(sChar);
    fin.putback(fChar);
    return;
  }
  DASSERT_EQ(count, 1);
  bool isBOM = ((fChar == (char)0xEF) && (sChar == (char)0xBB) && (tChar == (char)0xBF));
  if (!isBOM) {
    fin.putback(tChar);
    fin.putback(sChar);
    fin.putback(fChar);
  }
}

/**
 * Code from
 * http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
 *
 * A getline implementation which supports '\n', '\r' and '\r\n'
 */
std::istream& eol_safe_getline(std::istream& is, std::string& t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for(;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\n':
        return is;
      case '\r':
        if(sb->sgetc() == '\n')
          sb->sbumpc();
        return is;
      case EOF:
        // Also handle the case when the last line has no line ending
        if(t.empty())
          is.setstate(std::ios::eofbit);
        return is;
      default:
        t += (char)c;
    }
  }
}


/**
 * Reads until the eol string is encountered
 */
std::istream& custom_eol_getline(std::istream& is,
                                 std::string& t,
                                 const std::string& eol) {
  t.clear();
  if (eol.empty()) {
    // read the entire stream
    t = std::string(std::istreambuf_iterator<char>(is),
                    std::istreambuf_iterator<char>());
    return is;
  } else {
    // keep reading one character at a time
    // advancing the match position
    size_t eolmatchpos = 0;
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();
    for(;;) {
      int c = sb->sbumpc();
      if (c == eol[eolmatchpos]) {
        ++eolmatchpos;
        t += (char)c;
        if (eolmatchpos == eol.length()) {
          // eol matched. remove the eol from the string
          // and return
          t.resize(t.length() - eol.length());
          return is;
        }
      } else if (c == EOF) {
         if(t.empty()) is.setstate(std::ios::eofbit);
         return is;
      } else {
        t += (char)c;
        eolmatchpos = 0;
      }
    }
  }
}

/**
 * if eol == "\n", this will get read a line until the next
 * "\n", "\r" or "\r\n" sequence.
 * Otherwise, it will read until the eol string appears
 */
std::istream& eol_getline(std::istream& is,
                          std::string& t,
                          const std::string& eol) {
  if (eol == "\n") {
    return eol_safe_getline(is, t);
  } else {
    return custom_eol_getline(is, t, eol);
  }
}

namespace {

struct csv_info {
  size_t ncols = 0;
  std::vector<std::string> column_names;
  std::vector<flex_type_enum> column_types;
};

class parallel_csv_parser {
 public:
  /**
   * Constructs a parallel CSV parser which parses a CSV into SFrames
   * \param column_types This must match the number of columns of output and
   *                     the type of each column. Generally this should the same
   *                     number of columns as in the CSV. If only a subset if
   *                     (say 3 out of 4) columns are to be stored, this should
   *                     contain the types of the output columns, and
   *                     column_output_order is used to map the CSV columns to
   *                     the output columns.
   * \param tokenizer The tokenizer rules to use
   * \param continue_on_failure Whether to keep going even when an error is
   *                            encountered.
   * \param store_errors Whether to store bad lines in a separate SArray
   * \param row_limit Maximum number of rows to read
   * \param column_output_order An array of the same length as the output.
   *                     Essentially column 'i' will be written to output_order[i].
   *                     if output_order[i] == -1, the column is ignored.
   *                     If output_order is empty (default), this is equivalent
   *                     to the having output_order[i] == i.
   * \param num_threads Amount of parallelism to use.
   */
  parallel_csv_parser(std::vector<flex_type_enum> column_types,
                      csv_line_tokenizer tokenizer,
                      bool continue_on_failure,
                      bool store_errors,
                      size_t row_limit,
                      std::vector<size_t> column_output_order = std::vector<size_t>(),
                      size_t num_threads = thread_pool::get_instance().size()):
      nthreads(std::max<size_t>(num_threads, 2) - 1),
      parsed_buffer(nthreads), parsed_buffer_last_elem(nthreads),
      writing_buffer(nthreads), writing_buffer_last_elem(nthreads),
      error_buffer(nthreads), writing_error_buffer(nthreads),
      thread_local_tokenizer(nthreads, tokenizer),
    read_group(thread_pool::get_instance()),
    write_group(thread_pool::get_instance()), column_types(column_types),
    column_output_order(column_output_order),
    row_limit(row_limit),
    continue_on_failure(continue_on_failure),
    store_errors(store_errors),
    line_terminator(tokenizer.line_terminator),
    is_regular_line_terminator(line_terminator == "\n") {
    };

  /**
   * Sets the total size of all inputs. Required if multiple output segments
   * are desired. Otherwise all outputs will go to segment 0.
   */
  void set_total_input_size(size_t input_size) {
    total_input_file_sizes = input_size;
  }
  /**
   * Parses an input file into an output frame
   */
  void parse(general_ifstream& fin,
             sframe& output_frame,
             sarray<flexible_type>& errors) {
    size_t num_output_segments = output_frame.num_segments();
    size_t current_input_file_size = fin.file_size();
    try {
      timer ti;
      bool fill_buffer_is_good = true;
      while(fin.good() && fill_buffer_is_good &&
            (row_limit == 0 || lines_read.value < row_limit)) {
        fill_buffer_is_good = fill_buffer(fin);
        if (buffer.size() == 0) break;

        // if there is no line terminator, we should pick up the entire file.
        if (line_terminator.empty()) while(fill_buffer(fin));
        parallel_parse(!fin.good());
        /**************************************************************************/
        /*                                                                        */
        /*      This handles the end of the buffer which needs to be joined       */
        /*                   with the start of the next buffer.                   */
        /*                                                                        */
        /**************************************************************************/
        write_group.join();
        //join_background_write();

        if(cppipc::must_cancel()) {
          log_and_throw(std::string("CSV parsing cancelled"));
        }

        // we need to truncate the current writing buffer to ensure that we don't output more than
        // row_limit rows
        bool incomplete_write = false;
        if (row_limit > 0) {
          size_t remainder = 0;
          if (row_limit > lines_read.value) remainder = row_limit - lines_read.value;
          else remainder = 0;
          for (size_t i = 0; i < parsed_buffer_last_elem.size(); ++i) {
            if (parsed_buffer_last_elem[i] > remainder) {
              parsed_buffer_last_elem[i] = remainder;
              incomplete_write = true;
            }
            remainder -= parsed_buffer_last_elem[i];
          }
        }

        // if we are doing multiple segments
        if (total_input_file_sizes > 0) {
          // compute the current output segment
          // It really is simply.
          // current_output_segment =
          //         (fin.get_bytes_read() + cumulative_file_read_sizes)
          //          * num_output_segments / total_input_file_sizes;
          // But a lot of sanity checking is required because
          //  - fin.get_bytes_read() may fail.
          //  - files on disk may change after I last computed the file sizes, so
          //    there is no guarantee that cumulatively, they will all add up.
          //  - So, a lot of bounds checking is required.
          //  - Also, Once I advance to a next segment, I must not go back.
          size_t next_output_segment = current_output_segment;
          size_t read_pos = fin.get_bytes_read();
          if (read_pos == (size_t)(-1)) {
            // I don't know where I am in the file. Use what I last know
            read_pos = cumulative_file_read_sizes;
          } else {
            read_pos += cumulative_file_read_sizes;
          }
          next_output_segment = read_pos * num_output_segments / total_input_file_sizes;
          // sanity boundary check
          if (next_output_segment >= num_output_segments) next_output_segment = num_output_segments - 1;
          // we never go back
          current_output_segment = std::max(current_output_segment, next_output_segment);
        }

        start_background_write(output_frame, errors, current_output_segment);
        if (lines_read.value > 0) {
          logprogress_stream_ontick(5) << "Read " << lines_read.value
                                       << " lines. Lines per second: "
                                       << lines_read.value / get_time_elapsed()
                                       << "\t"
                                       << std::endl;
        }
        if (incomplete_write) {
          write_group.join();
        }
      }
      // file read complete. join the writer
      write_group.join();

      cumulative_file_read_sizes += current_input_file_size;
    } catch (...) {
      try { read_group.join(); } catch (...) { }
      try { write_group.join(); } catch (...) { }
      // even on a failure, we still increment the cumulative read count
      cumulative_file_read_sizes += current_input_file_size;
      throw;
    }
  }

  /**
   * Returns the number of lines which failed to parse
   */
  size_t num_lines_failed() const {
    return num_failures;
  }

  /**
   * Returns the number of CSV lines read
   */
  size_t num_lines_read() const {
    return lines_read.value;
  }

  /**
   * Returns the number of input columns in the CSV file
   */
  size_t num_input_columns() const {
    if (column_output_order.empty()) return column_types.size();
    else return column_output_order.size();
  }

  /**
   * Returns the number of output columns in the CSV file
   */
  size_t num_output_columns() const {
    return column_types.size();
  }

  /**
   * Start timer
   */
  void start_timer() {
    ti.start();
  }

  /**
   * Current time elapsed
   */
  double get_time_elapsed() {
    return ti.current_time();
  }

 private:
  /// number of threads
  size_t nthreads;
  /// thread local parse output buffer
  std::vector<std::vector<std::vector<flexible_type> > > parsed_buffer;
  /// Number of elements in each thread local parse output buffer
  std::vector<size_t> parsed_buffer_last_elem;
  /// thread local write buffer
  std::vector<std::vector<std::vector<flexible_type> > > writing_buffer;
  /// Number of elements in each thread local write buffer
  std::vector<size_t> writing_buffer_last_elem;
  /// Thread local error buffer
  std::vector<std::vector<flexible_type>> error_buffer;
  /// Error buffer used for when writing
  std::vector<std::vector<flexible_type>> writing_error_buffer;
  /// Thread local tokenizer
  std::vector<csv_line_tokenizer> thread_local_tokenizer;
  /// shared buffer
  std::string buffer;
  /// Same length as buffer. marks if a part of the buffer is in a quoted string
  dense_bitset quote_parity;
  /// Reading thread pool
  parallel_task_queue read_group;
  /// writing thread pool
  parallel_task_queue write_group;

  std::vector<flex_type_enum> column_types;
  std::vector<size_t> column_output_order;

  size_t current_output_segment = 0;

  atomic<size_t> lines_read = 0;
  timer ti;
  size_t row_limit = 0;
  size_t cumulative_file_read_sizes = 0;
  size_t total_input_file_sizes = 0;

  volatile bool background_thread_running = false;
  atomic<size_t> num_failures = 0;
  bool continue_on_failure = false;
  bool store_errors = false;

  std::string line_terminator = "\n";
  // true if the line_terminator is "\n"
  bool is_regular_line_terminator = true;

  inline bool is_end_line_str(char* c, char* cend) const {
    if (is_regular_line_terminator) return (*c) == '\n' || (*c) == '\r';
    else if (line_terminator.empty() == false &&
             cend - c >= (int)(line_terminator.length())) {
      for (char nl : line_terminator) {
        if (nl != (*c)) return false;
        ++c;
      }
      return true;
    }
    return false;
  }


  /**
   * For a string which begins at c and ends at cend (exclusive), this advances
   * c until it exceeds the encounters a newline and returns the pointer to the
   * character immediately *after* the newline.
   *
   * line_was_matched returns true if a newline was matched, and false otherwise.
   * (since the returned value could be cend, but a newline was matched.
   */
  char* advance_past_newline(char* c, char* cend, bool& newline_was_matched) const {
    if (is_regular_line_terminator) {
      while(c < cend) {
         if ((*c) == '\n') {
           // its just a \n. advance past the \n and return
           newline_was_matched = true;
           return c + 1;
         } else if ((*c) == '\r') {
           // its a \r. It could be just a \r, or a \r\n.
           // check for \r\n
           if (c + 1 < cend && (*(c+1)) == '\n') {
             // its a \r\n, advance past and return
             newline_was_matched = true;
             return c + 2;
           } else {
             // its a mac line ending. Its just \r
             newline_was_matched = true;
             return c + 1;
           }
         }
         ++c;
       }
    } else if (line_terminator.empty() == false) {
      while(c + line_terminator.length() <= cend) {
        bool match = true;
        for (size_t i = 0;i < line_terminator.length(); ++i) {
          if (c[i] != line_terminator[i]) {
            match = false;
            break;
          }
        }
        if (match) {
          newline_was_matched = true;
          return c + line_terminator.length();
        }
        ++c;
      }
    }
    newline_was_matched = false;
    return cend;
  }
  // same as advance_past_newline, but checks the quote_parity
  // to make sure we are only handling true new lines
  char* advance_past_newline_with_quote_parity(char* bufstart, char* c, char* cend, 
                                               bool& newline_was_matched) const {
    newline_was_matched = false;
    while(c != cend) {
      c = advance_past_newline(c, cend, newline_was_matched);
      bool b = quote_parity.get(c - bufstart - 1);
      // it is not actually a newline if the quote parity is set
      // continue to continue searching for the next newline
      if (b && newline_was_matched) {
        newline_was_matched = false;
        continue;
      } else {
        break;
      }
    }
    return c;
  }

  /// parses the line between pstart to pnext, using threadid's buffer
  void parse_line(char* pstart, char* pnext, size_t threadid) {
    // this is the current character I am scanning
    const char comment_char = thread_local_tokenizer[threadid].comment_char;
    // clear local tokens
    size_t nextelem = parsed_buffer_last_elem[threadid];
    if (nextelem >= parsed_buffer[threadid].size()) parsed_buffer[threadid].resize(nextelem + 1);
    std::vector<flexible_type>& local_tokens = parsed_buffer[threadid][nextelem];
    local_tokens.resize(column_types.size());
    for (size_t i = 0;i < column_types.size(); ++i) {
      if (local_tokens[i].get_type() != column_types[i]) {
        local_tokens[i].reset(column_types[i]);
      }
    }
    const std::vector<size_t>* ptr_to_output_order =
        column_output_order.empty() ? nullptr : &column_output_order;

    size_t num_tokens_parsed =
        thread_local_tokenizer[threadid].
        tokenize_line(pstart, pnext - pstart,
                      local_tokens,
                      true /*permit undefined*/,
                      ptr_to_output_order);

    if (num_tokens_parsed == num_input_columns()) {
      ++parsed_buffer_last_elem[threadid];
    } else {

      // incomplete parse
      std::string badline(pstart, pnext - pstart);
      boost::algorithm::trim(badline);

      if (!badline.empty() && badline[0] != comment_char) {
        // keep track of line for error reporting
        if (store_errors) error_buffer[threadid].push_back(badline);
        if (continue_on_failure) {
          if (num_failures.value < 10) {
            if (!thread_local_tokenizer[threadid].get_last_parse_error_diagnosis().empty()) {
              logprogress_stream << thread_local_tokenizer[threadid].get_last_parse_error_diagnosis() 
                                 << std::endl;
            } else {
              std::string badline = std::string(pstart, pnext - pstart);
              if (badline.length() > 256) badline=badline.substr(0, 256) + "...";
              logprogress_stream << std::string("Unable to parse line \"") +
                                 badline + "\"" << std::endl;
            }
          }
          ++num_failures;
        } else {
          if (!thread_local_tokenizer[threadid].get_last_parse_error_diagnosis().empty()) {
            logprogress_stream << thread_local_tokenizer[threadid].get_last_parse_error_diagnosis() 
                               << std::endl;
          } 
          std::string badline = std::string(pstart, pnext - pstart);
          if (badline.length() > 256) badline=badline.substr(0, 256) + "...";
          log_and_throw(std::string("Unable to parse line \"") +
                        badline + "\"\n");
        }
      }
    }
  }

  /**
   * Performs the parse on a section of the buffer (threadid in nthreads)
   * adding new rows into the parsed_buffer.
   * Returns a pointer to the last unparsed character.
   */
  char* parse_thread(size_t threadid) {
    size_t step = buffer.size() / nthreads;
    // cut the buffer into "nthread uniform pieces"
    char* bufstart = &buffer[0];
    char* pstart = &(buffer[0]) + threadid * step;
    char* pend = &(buffer[0]) + (threadid + 1) * step;
    char* bufend = &(buffer[0]) + buffer.size();
    if (threadid == nthreads - 1) pend = bufend;

    // ok, this is important. Pay attention.
    // We are sweeping from
    //  - the first line which begins AFTER pstart, but before pend
    //  - And we are finishing on the last line which ends AFTER pend.
    // (if we are the last thread, something special happens and
    //  the last unparsed line gets pushed into the next buffer)
    //  This correctly takes care of all cases; even where a line spans
    //  multiple pieces.
    //
    //  Another way to think about it is that whatever range includes the very
    //  last character in the line terminator, is responsible for handling the
    //  next line. i.e. if the line terminator is "abcd"
    //
    //  hello, world abcd
    //  1, 2 abcd
    //  3, 4 abcd
    //
    //  Then whichever range includes a "d" handles the line after that.
    //
    //  This is a little subtle when the line_terminator may be multiple
    //  characters.
    //

    /**************************************************************************/
    /*                                                                        */
    /*                        Find the Start Position                         */
    /*                                                                        */
    /**************************************************************************/
    // threadid 0 begins at start
    bool start_position_found = (threadid == 0);
    if (threadid > 0) {
      // find the first line beginning after pstart but before pend

      // if we have a multicharacter line terminator, we have to be a bit
      // intelligent. to match the "last character" of the terminator,
      // we need to shift the newline search backwards by
      // line_terminator.length() - 1 characters
      if (!is_regular_line_terminator &&
          line_terminator.length() > 1 &&
          // make sure there is enough room to shift backwards
          pstart - bufstart >= int(line_terminator.length() - 1)) {
        pstart -= line_terminator.length() - 1;
      }
      bool newline_was_matched;
      pstart = advance_past_newline_with_quote_parity(bufstart, pstart, pend, newline_was_matched);
      if (newline_was_matched) {
        start_position_found = true;
      }
    }
    if (start_position_found) {
      /**************************************************************************/
      /*                                                                        */
      /*                         Find the End Position                          */
      /*                                                                        */
      /**************************************************************************/
      // find the end position
      if (!is_regular_line_terminator &&
          line_terminator.length() > 1 &&
          // make sure there is enough room to shift backwards
          pend - bufstart >= int(line_terminator.length() - 0)) {
        pend -= line_terminator.length() - 1;
      }
      bool newline_was_matched_unused = false;
      pend = advance_past_newline_with_quote_parity(bufstart, pend, bufend, newline_was_matched_unused);
      // make a thread local parsed tokens set
      /**************************************************************************/
      /*                                                                        */
      /*                          start parsing lines                           */
      /*                                                                        */
      /**************************************************************************/
      char* pnext = pstart;

      // the rule that every line must end with a terminator is wrong when
      // the line terminator is empty. some special handling is needed for this
      // case.
      if (line_terminator.empty()) {
        parse_line(pstart, pend, threadid);
      } else {
        while(pnext < pend) {
          // search for a new line
          // This can be massively optimized in the general case of interesting
          // end line characters using Robin Karp or KMP.
          if (is_end_line_str(pnext, pend) && quote_parity.get(pnext - bufstart) == false) {
            // parse pstart until pnext
            parse_line(pstart, pnext, threadid);
            pnext = advance_past_newline_with_quote_parity(bufstart, pnext, pend, newline_was_matched_unused);
            pstart = pnext;
          } else {
            ++pnext;
          }
        }
      }
    }
    return pstart;
  }

  /**
   * Adds a line terminator to the buffer if it does not already
   * end with a line terminator. Used by the buffer reading routines on
   * EOF so that the parser is always guaranteed that every line
   * ends with a line terminator, even the last line.
   */
  void add_line_terminator_to_buffer() {
    if (is_regular_line_terminator &&
        buffer[buffer.length() - 1] != '\n' &&
        buffer[buffer.length() - 1] != '\r') {
      buffer.push_back('\n');
    } else if (!is_regular_line_terminator &&
               buffer.length() >= line_terminator.length() &&
               buffer.substr(buffer.length() - line_terminator.length()) != line_terminator) {
      buffer += line_terminator;
    }
  }

  /**
   * Adds a large chunk of bytes to the buffer. Returns true if all requested
   * lines were read. False otherwise: indicating this is the last block.
   */
  bool fill_buffer(general_ifstream& fin) {
    if (fin.good()) {
      size_t oldsize = buffer.size();
      size_t amount_to_read = SFRAME_CSV_PARSER_READ_SIZE;
      buffer.resize(buffer.size() + amount_to_read);
      fin.read(&(buffer[0]) + oldsize, buffer.size() - oldsize);
      if ((size_t)fin.gcount() < amount_to_read) {
        // if we did not read till the entire buffer , this is an EOF
        buffer.resize(oldsize + fin.gcount());
        // EOF. Put a line_terminator to catch the last line
        add_line_terminator_to_buffer();
        return false;
      } else {
        return true;
      }
    } else {
      // EOF. Put a line_terminator to catch the last line
      add_line_terminator_to_buffer();
      return false;
    }
  }

  /**
   * to handle CSV lines which span multiple lines, this happens in particular
   * when we have quoted line terminators. For instance a CSV might be of the
   * form
   *  
   * user_id, review
   * 123, "good food"
   * 456, "hello
   * world"
   * 678, "moof"
   *  
   * In this case, "hello\nworld" spans a line.
   * 
   * However, this gets complicated with the buffer splitting algorithm which
   * takes a large contiguous buffer and tries to cut it up into #thread chunks.
   * Each chunk must span a complete set of lines. See \ref parse_thread
   * for details.
   * 
   * To fix this behavior, we need to be able to mark which line_terminator 
   * positions are *really* line terminators and which are sitting within quotes.
   * Note that there are a variety of quoting rules we need to be able to handle.
   * 
   * To do this, we pay for one pass through the buffer counting quote characters
   * and marking all the "valid" newline positions.
   *
   * This is essentially a "parity" problem. we need to know at each
   * character position in the buffer, if we are in a quoted string or not.
   *
   * This uses the information in \ref tokenizer to get the quoting rules.
   * Of the csv rules, we can ignore double_quote, (adjacent quote chars 
   * just flips the parity back and forth). Rules which affect are  
   * quote_char, escape_char, comment_char, has_comment_char.
   *
   * comment_char is quite hard...
   *
   * This function reads the variable \ref buffer and fills \ref quote_parity.
   */
  void find_true_new_line_positions() {
    quote_parity.resize(buffer.size());
    quote_parity.clear();

    // if the prev char is not an escape char.
    // this seems like not very natural instead of is_esc. but this
    // saves a negation every loop iteration.
    bool not_esc = true; 
    csv_line_tokenizer& tokenizer = thread_local_tokenizer[0];
    const char escape_char = tokenizer.escape_char;
    const char quote_char = tokenizer.quote_char;
    const char comment_char = tokenizer.comment_char;
    bool cur_in_quote = false;

    int idx = 0;
    const char* __restrict__ buf = &(buffer[0]);
    const char* __restrict__ bufend = &(buffer[0]) + buffer.size();
    if (tokenizer.has_comment_char) {
      // with comment char we need to do a bit more work.
      while(buf != bufend) {
        // fast path. quickly scan through the buffer while I don't see special
        // characters
        if (not_esc) {
          while(buf != bufend && 
                (*(buf) != comment_char) && 
                (*(buf) != quote_char) && 
                (*(buf) != escape_char)) {
            ++buf;
          }
          int endidx = buf - &(buffer[0]);
          if (cur_in_quote) {
            // if in_quote we need to flag all the quote_parity bits
            while(idx < endidx) {
              quote_parity.set_bit_unsync(idx);
              ++idx;
            }
          } else {
            idx = endidx;
          }
          if (buf == bufend) break;
          // continue for slow path handling
        }
        const char c = (*buf);
        if (c == comment_char && not_esc && cur_in_quote == false) {
          // skip to next newline 
          bool newline_was_matched = false;
          const char* initial_buf = buf;
          buf = advance_past_newline((char*)buf, (char*)bufend, newline_was_matched);
          idx += buf - initial_buf;
          if (newline_was_matched == false) break;
          else continue;
        }
        bool is_quote_char = (c == quote_char) && not_esc;
        cur_in_quote ^= is_quote_char;
        quote_parity.set_unsync(idx, cur_in_quote);
        // invert of the following
        // if (!esc) esc = c == escape_char
        // else esc = false
        not_esc = !not_esc || c != escape_char;
        ++idx; ++buf;
      }
    } else {
      // without comment char this is simpler.
      while(buf != bufend) {
        // fast path. quickly scan through the buffer while I don't see special
        // characters
        if (not_esc) {
          while(buf != bufend && 
                (*(buf) != quote_char) && 
                (*(buf) != escape_char)) {
            ++buf;
          }
          int endidx = buf - &(buffer[0]);
          if (cur_in_quote) {
            // if in_quote we need to flag all the quote_parity bits
            while(idx < endidx) {
              quote_parity.set_bit_unsync(idx);
              ++idx;
            }
          } else {
            idx = endidx;
          }
          if (buf == bufend) break;
          // continue for slow path handling
        }
        const char c = (*buf);
        bool is_quote_char = (c == quote_char) && not_esc;
        cur_in_quote ^= is_quote_char;
        quote_parity.set_unsync(idx, cur_in_quote);
        // invert of the following
        // if (!esc) esc = c == escape_char
        // else esc = false
        not_esc = !not_esc || c != escape_char;
        ++idx; ++buf;
      }
    }
  }

  /**
   * Performs a parallel parse of the contents of the buffer, adding to
   * parsed_buffer.
   *
   * \param eof Flags if this is the last buffer. If it is, we get a little
   *            more aggressive at making sure we consume everything and
   *            not leave unparsed stuff.
   */
  void parallel_parse(bool eof) {
    // take a pointer to the last token that has been succesfully parsed.
    // this will tell me how much buffer I need to shift to the next read.
    // parse buffer in parallel
    mutex last_parsed_token_lock;
    char* last_parsed_token = &(buffer[0]);

    find_true_new_line_positions();
    if (eof && buffer.size() > 0) {
      // the last newline is *always* of the right parity
      quote_parity.clear_bit_unsync(buffer.size() - 1);
    }

    for (size_t threadid = 0; threadid < nthreads; ++threadid) {
      read_group.launch(
          [=,&last_parsed_token_lock,&last_parsed_token](void) {
            char* ret = parse_thread(threadid);
            std::lock_guard<mutex> guard(last_parsed_token_lock);
            if (last_parsed_token < ret) last_parsed_token = ret;
          } );
    }
    read_group.join();
    // ok shift the buffer left
    if ((size_t)(last_parsed_token - buffer.data()) < buffer.size()) {
      buffer = buffer.substr(last_parsed_token - buffer.data());
    } else {
      buffer.clear();
    }
  }

  /**
   * Spins up a background thread to write parse results from parallel_parse
   * to the output frame. First the parsed_buffer is swapped into the
   * writing_buffer, thus permitting the parsed_buffer to be used again in
   * a different thread.
   */
  void start_background_write(sframe& output_frame,
                              sarray<flexible_type>& errors_array,
                              size_t output_segment) {
    // switch the parse buffer with the write buffer
    writing_buffer.swap(parsed_buffer);
    writing_buffer_last_elem.swap(parsed_buffer_last_elem);
    background_thread_running = true;
    // reset the parsed buffer
    parsed_buffer_last_elem.clear();
    parsed_buffer_last_elem.resize(nthreads, 0);
    writing_error_buffer.swap(error_buffer);

    write_group.launch([&, output_segment] {
        auto iter = output_frame.get_output_iterator(output_segment);
        for (size_t i = 0; i < writing_buffer.size(); ++i) {
          std::copy(writing_buffer[i].begin(),
                    writing_buffer[i].begin() + writing_buffer_last_elem[i], iter);
          lines_read.inc(writing_buffer_last_elem[i]);
        }
        if (store_errors) {
          auto errors_iter = errors_array.get_output_iterator(0);
          for (auto& chunk_errors : writing_error_buffer) {
            std::copy(chunk_errors.begin(), chunk_errors.end(), errors_iter);
            chunk_errors.clear();
          }
        }
        background_thread_running = false;
      });
  }

};

/**
 * Makes unique column names R-style.
 *
 * if we have duplicated column names, the duplicated names get a .1, .2, .3
 * etc suffix.
 *
 * e.g.
 * {"A", "A", "A"} --> {"A", "A.1", "A.2"}
 *
 * If the name with the suffix already exists, the suffix is skipped:
 *
 * e.g.
 * {"A", "A", "A.1"} --> {"A", "A.2", "A.1"}
 *
 * \param column_names The set of column names to be renamed. The vector
 *                     will be modified in place.
 */
void make_unique_column_names(std::vector<std::string>& column_names) {
  // this is the set of column names to the left of the column we
  // are current inspected. i.e. these column names are already validated to
  // be correct.
  log_func_entry();
  std::set<std::string> accepted_column_names;
  for (size_t i = 0;i < column_names.size(); ++i) {
    std::string colname = column_names[i];

    if (accepted_column_names.count(colname)) {
      // if this is a duplicated name
      // we need to rename this column
      // insert the rest of the columns into a new set to test if the suffix
      // already exists.
      std::set<std::string> all_column_names(column_names.begin(),
                                             column_names.end());
      // start incrementing at A.1, A.2, etc.
      size_t number = 1;
      std::string new_column_name;
      while(1) {
        new_column_name = colname + "." + std::to_string(number);
        if (all_column_names.count(new_column_name) == 0) break;
        ++number;
      }
      column_names[i] = new_column_name;
    }
    accepted_column_names.insert(column_names[i]);
  }
}

/**************************************************************************/
/*                                                                        */
/* Parse Header Line                                                      */
/* -----------------                                                      */
/* Read the header line if "use_header" is specified.                     */
/* Use the column names from the header. Performing column name mangling  */
/* R-style if there are duplicated column names                           */
/*                                                                        */
/* If use_header is not specified, we read the first line anyway,         */
/* and try to figure out the number of columns. Use the sequence X1,X2,X3 */
/* as column names.                                                       */
/*                                                                        */
/* At completion of this section,                                         */
/* - ncols is set                                                         */
/* - column_names.size() == ncols                                         */
/* - column_names contain all the column headers.                         */
/* - all column names are different                                       */
/*                                                                        */
/**************************************************************************/
void read_csv_header(csv_info& info,
                     const std::string& path,
                     csv_line_tokenizer& tokenizer,
                     bool use_header,
                     size_t skip_rows) {
  std::string first_line;
  std::vector<std::string> first_line_tokens;
  general_ifstream probe_fin(path);

  if (!probe_fin.good()) {
    log_and_throw("Fail reading " + sanitize_url(path));
  }
  skip_BOM(probe_fin);

  // skip skip_rows lines
  std::string skip_string;
  for (size_t i = 0;i < skip_rows; ++i) {
    eol_getline(probe_fin, skip_string, tokenizer.line_terminator);
  }

  // skip rows with no data
  while (first_line_tokens.size() == 0 && probe_fin.good()) {
    eol_getline(probe_fin, first_line, tokenizer.line_terminator);
    boost::algorithm::trim(first_line);
    tokenizer.tokenize_line(&(first_line[0]),
                            first_line.length(),
                            first_line_tokens);
  }

  info.ncols = first_line_tokens.size();

  if (use_header) {
    info.column_names = first_line_tokens;
    make_unique_column_names(info.column_names);
    first_line.clear();
    first_line_tokens.clear();
  } else {
    info.column_names.resize(info.ncols);
    for (size_t i = 0;i < info.ncols; ++i) {
      info.column_names[i] = "X" + std::to_string(i + 1);
    }
    // do not clear first_line and first_line_tokens. They are actual data.
  }
}

/**************************************************************************/
/*                                                                        */
/* Type Determination                                                     */
/* ------------------                                                     */
/* At this stage we fill column_types. This stage can be expanded in      */
/* the future to perform more comprehensive type inference. But at the    */
/* moment, we simply default all column types to STRING, and use the      */
/* types specified in column_type_hints if there is a column name         */
/* which matches.                                                         */
/*                                                                        */
/* At completion of this section:                                         */
/* - column_types.size() == column_names.size() == ncols                  */
/*                                                                        */
/**************************************************************************/
void get_column_types(csv_info& info,
                      std::map<std::string, flex_type_enum> column_type_hints) {
  info.column_types.resize(info.ncols, flex_type_enum::STRING);

  if (column_type_hints.count("__all_columns__")) {
    info.column_types = std::vector<flex_type_enum>(info.ncols, column_type_hints["__all_columns__"]);
  } else if (column_type_hints.count("__X0__")) {
    if (column_type_hints.size() != info.column_types.size()) {
      std::stringstream warning_msg;
      warning_msg << "column_type_hints has different size from actual number of columns: "
                  << "column_type_hints.size()=" << column_type_hints.size()
                  << ";number of columns=" << info.ncols
                  << std::endl;
      log_and_throw(warning_msg.str());
    }
    for (size_t i = 0; i < info.ncols; ++i) {
      std::stringstream key;
      key << "__X" << i << "__";
      if (column_type_hints.count(key.str())) {
        info.column_types[i] = column_type_hints[key.str()];
      } else {
        log_and_throw("Bad column type hints");
      }
    }
  } else {
    for (size_t i = 0; i < info.column_names.size(); ++i) {
      if (column_type_hints.count(info.column_names[i])) {
        flex_type_enum coltype = column_type_hints.at(info.column_names[i]);
        info.column_types[i] = coltype;
        column_type_hints.erase(info.column_names[i]);
      }
    }
    if (column_type_hints.size() > 0) {
      std::stringstream warning_msg;
      warning_msg << "These column type hints were not used:";
      for(const auto &hint : column_type_hints) {
        warning_msg << " " << hint.first;
      }
      logprogress_stream << warning_msg.str() << std::endl;
    }
  }
}

} // anonymous namespace


/**
 * Parsed a CSV file to an SFrame.
 *
 * \param path The file to open as a csv
 * \param tokenizer The tokenizer configuration to use. This should be
 *                  filled with all the tokenization rules (like what
 *                  separator character to use, what quoting character to use,
 *                  etc.)
 * \param writer The sframe writer to use.
 * \param frame_sidx_file Where to write the frame to
 * \param parallel_csv_parser A parallel_csv_parser
 * \param errors A reference to a map in which to store an sarray of bad lines
 * for each input file.
 */
void parse_csv_to_sframe(
    const std::string& path,
    csv_line_tokenizer& tokenizer,
    csv_file_handling_options options,
    sframe& frame,
    std::string frame_sidx_file,
    parallel_csv_parser& parser,
    std::map<std::string, std::shared_ptr<sarray<flexible_type>>>& errors) {
  auto use_header = options.use_header;
  auto continue_on_failure = options.continue_on_failure;
  auto store_errors = options.store_errors;
  auto skip_rows = options.skip_rows;

  logstream(LOG_INFO) << "Loading sframe from " << sanitize_url(path) << std::endl;

  // load; For each line, insert into the frame
  {
    general_ifstream fin(path);
    if (!fin.good()) log_and_throw("Cannot open " + sanitize_url(path));
    skip_BOM(fin);

    // skip skip_rows lines
    std::string skip_string;
    for (size_t i = 0;i < skip_rows; ++i) {
      eol_getline(fin, skip_string, tokenizer.line_terminator);
    }

    // if use_header, we keep throwing away empty or comment lines until we
    // get one good line
    if (use_header) {
      std::vector<std::string> first_line_tokens;
      // skip rows with no data, and skip the head
      while (first_line_tokens.size() == 0 && fin.good()) {
        std::string line;
        eol_getline(fin, line, tokenizer.line_terminator);
        tokenizer.tokenize_line(&(line[0]), line.length(), first_line_tokens);
      }
      // if we are going to store errors, we don't do early skippng on
      // mismatched files
      if (!store_errors &&
          first_line_tokens.size() != parser.num_input_columns()) {
        logprogress_stream << "Unexpected number of columns found in " << path
                           << ". Skipping this file." << std::endl;
        return;
      }
    }

    // store errors for this particular file in an sarray
    auto file_errors = std::make_shared<sarray<flexible_type>>();
    if (store_errors) {
      file_errors->open_for_write();
      file_errors->set_type(flex_type_enum::STRING);
    }

    try {
      parser.parse(fin, frame, *file_errors);
    } catch(const std::string& s) {
      frame.close();
      if (store_errors) file_errors->close();
      log_and_throw(s);
    }

    if (continue_on_failure && parser.num_lines_failed() > 0) {
      logprogress_stream << parser.num_lines_failed()
                         << " lines failed to parse correctly"
                         << std::endl;
    }

    if (store_errors) {
      file_errors->close();
      if (file_errors->size() > 0) {
        errors.insert(std::make_pair(path, file_errors));
      }
    }

    logprogress_stream << "Finished parsing file " << sanitize_url(path) << std::endl;
  }
}

std::map<std::string, std::shared_ptr<sarray<flexible_type>>> parse_csvs_to_sframe(
    const std::string& url,
    csv_line_tokenizer& tokenizer,
    csv_file_handling_options options,
    sframe& frame,
    std::string frame_sidx_file) {
  // unpack the options
  auto use_header = options.use_header;
  auto continue_on_failure = options.continue_on_failure;
  auto store_errors = options.store_errors;
  auto column_type_hints = options.column_type_hints;
  auto output_columns = options.output_columns;
  auto row_limit = options.row_limit;
  auto skip_rows = options.skip_rows;

  if (store_errors) continue_on_failure = true;
  // otherwise, check that url is valid directory, and get its listing if no
  // pattern present
  std::vector<std::string> files;
  bool found_zero_byte_files = false;
  std::vector<std::pair<std::string, file_status>> file_and_status = fileio::get_glob_files(url);

  for (auto p : file_and_status) {
    if (p.second == file_status::REGULAR_FILE) {
      // throw away empty files
      try {
        general_ifstream fin(p.first);
        if (fin.file_size() == 0) {
          logstream(LOG_INFO) << "Skipping file "
                              << sanitize_url(p.first)
                              << " because it appears to be empty"
                              << std::endl;
          found_zero_byte_files = true;
          continue;
        }
      } catch (...) {
          logstream(LOG_INFO) << "Can't get size of file "
                              << sanitize_url(p.first)
                              << std::endl;
      }

      logstream(LOG_INFO) << "Adding CSV file "
                          << sanitize_url(p.first)
                          << " to list of files to parse"
                          << std::endl;
      files.push_back(p.first);
    }
  }

  file_and_status.clear(); // don't need these anymore

  // ensure that we actually found some valid files
  if (files.empty()) {
    if (found_zero_byte_files) {
      // We only found zero-byte files - return an empty SFrame.
      if (!frame.is_opened_for_write()) {
        frame.open_for_write({},{},frame_sidx_file);
      }
      frame.close();
      return {};
    } else {
      log_and_throw(std::string("No files corresponding to the specified path (") +
                    sanitize_url(url) + std::string(")."));
    }
  }
  // get CSV info from first file
  csv_info info;
  read_csv_header(info, files[0], tokenizer, use_header, skip_rows);
  logstream(LOG_INFO) << "CSV num. columns: " << info.ncols << std::endl;

  if (info.ncols <= 0)
    log_and_throw(std::string("0 columns found"));

  // check output_columns
  std::vector<size_t> output_column_order;
  if (!output_columns.empty()) {
    /*
     * We are only outputing a subset of columns, we need to prepare some data
     * structures.We need to prepare the output_column_order structure which is
     * basically a mapping from input columns to output columns, and is -1
     * where the input column is dropped.
     */
    // reset the output column order to all '-1' i.e. not generating any columns
    output_column_order.resize(info.column_names.size(), (size_t)(-1));

    // loop through each column, find it in the csv column names we know of
    // and if it exists, arrange to output it.
    for (size_t i = 0;i < output_columns.size(); ++i) {
      const auto& outcol = output_columns[i];
      auto iter = std::find(info.column_names.begin(),
                            info.column_names.end(),
                            outcol);
      // Cannot find this column in the talble?
      // is output_columns a positional type? i.e. "X" something
      if (iter == info.column_names.end() &&
          outcol.length() > 1 && outcol[i] == 'X') {
        size_t colnumber = stoull(outcol.substr(1));
        // column number is 1 based
        if (colnumber == 0 || colnumber > info.column_names.size()) {
          log_and_throw(std::string("Cannot find positional column ") + outcol);
        } else {
          iter = info.column_names.begin() + (colnumber - 1);
        }
      }
      if (iter != info.column_names.end()) {
        size_t src_column_index = iter - info.column_names.begin();
        output_column_order[src_column_index] = i;
      } else {
        log_and_throw(std::string("Cannot find column name: ") + outcol);
      }
    }
    info.column_names = output_columns;
    info.ncols = output_columns.size();
  }
  // fill in the type information
  get_column_types(info, column_type_hints);

  parallel_csv_parser parser(info.column_types, tokenizer,
                             continue_on_failure, store_errors, row_limit,
                             output_column_order);
  // get the total input file size so I can stripe it across segments
  size_t total_input_file_sizes = 0;
  for (auto file : files) {
    general_ifstream fin(file);
    total_input_file_sizes += fin.file_size();
  }
  parser.set_total_input_size(total_input_file_sizes);

  if (!frame.is_opened_for_write()) {
    // open as many segments as there are temp directories.
    // But at least one segment
    frame.open_for_write(info.column_names, info.column_types,
                         frame_sidx_file,
                         std::max<size_t>(1, num_temp_directories()));
  }

  // create the errors map
  std::map<std::string, std::shared_ptr<sarray<flexible_type>>> errors;

  // start parser timer for cumulative time consumed (in seconds)
  parser.start_timer();

  for (auto file : files) {
    // check that we've read < row_limit
    if (parser.num_lines_read() < row_limit || row_limit == 0) {
      parse_csv_to_sframe(file, tokenizer, options, frame,
                          frame_sidx_file, parser, errors);
    } else break;
  }

  logprogress_stream << "Parsing completed. Parsed " << parser.num_lines_read()
                     << " lines in " << parser.get_time_elapsed() << " secs."  << std::endl;


  if (frame.is_opened_for_write()) frame.close();

  return errors;
}

} // namespace turi
