/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/extensions/option_info.hpp>

#include <boost/lexical_cast.hpp>

namespace turi { namespace option_handling {

flexible_type option_info::to_dictionary() const {

  flex_dict n;
  n.push_back({"description", description});
  n.push_back({"default_value", default_value});
  switch(parameter_type) {
    case REAL: {
      n.push_back({"parameter_type", "REAL"});
      n.push_back({"lower_bound", lower_bound});
      n.push_back({"upper_bound", upper_bound});
      break;
    }
    case INTEGER: {
      n.push_back({"parameter_type", "INTEGER"});
      n.push_back({"lower_bound", lower_bound});
      n.push_back({"upper_bound", upper_bound});
      break;
    }
    case BOOL: {
      n.push_back({"parameter_type", "BOOL"});
      break;
    }
    case CATEGORICAL: {
      n.push_back({"parameter_type", "CATEGORICAL"});
      flex_list a;
      for (const flexible_type& v : allowed_values) {
        a.push_back(v);
      }
      n.push_back({"possible_values", a});
      break;
    }
    case STRING: {
      n.push_back({"parameter_type", "STRING"});
      break;
    }
    case FLEXIBLE_TYPE: {
      n.push_back({"parameter_type", "DYNAMIC"});
      break;
    }
  }
  return n;
}

flexible_type option_info::interpret_value(const flexible_type& value) const {

  std::string sep_char = (value.get_type() == flex_type_enum::STRING) ? "'" : "";

  flexible_type ret_v;

  switch(parameter_type) {
    case option_info::REAL:
      {
        bool value_type_okay = false;

        switch(value.get_type()) {
          case flex_type_enum::INTEGER:
            ret_v = double(value);
            value_type_okay = true;
            break;
          case flex_type_enum::FLOAT:
            ret_v = value;
            value_type_okay = true;
            break;
          case flex_type_enum::STRING:
            {
              try {
                ret_v = boost::lexical_cast<double>(value.get<flex_string>());
                value_type_okay = true;
              } catch(const boost::bad_lexical_cast&) {
                value_type_okay = false;
              }

              break;
            }
          case flex_type_enum::UNDEFINED:
            ret_v = default_value;
            value_type_okay = true;
            break;
          default:
            value_type_okay = false;
            break;
        }

        if(!value_type_okay) {
          std::ostringstream msg;
          msg << "Expected numeric value for option '" << name
              << "'. Cannot cast " << sep_char << value << sep_char
              << " to a numeric value.";
          log_and_throw(msg.str());
        }

        if( (!(double(ret_v) >= lower_bound)) || (!(double(ret_v) <= upper_bound))) {
          std::ostringstream msg;
          msg << "Option '" << name << "' must be in the range ["
              << lower_bound << ", " << upper_bound << "].";
          log_and_throw(msg.str());
        }

        break;
      }
    case option_info::INTEGER:
      {
        bool value_type_okay = false;

        switch(value.get_type()) {
          case flex_type_enum::INTEGER:
            ret_v = value;
            value_type_okay = true;
            break;
          case flex_type_enum::FLOAT:
            ret_v = int64_t(value.get<double>());
            if(double(ret_v) != value.get<double>())
              value_type_okay = false;
            else
              value_type_okay = true;
            break;
          case flex_type_enum::STRING:
            {
              try {
                ret_v = boost::lexical_cast<int64_t>(value.get<flex_string>());
                value_type_okay = true;
              } catch(const boost::bad_lexical_cast&) {
                value_type_okay = false;
              }

              break;
            }
          case flex_type_enum::UNDEFINED:
            ret_v = default_value;
            value_type_okay = true;
            break;
          default:
            value_type_okay = false;
            break;
        }

        if(!value_type_okay) {
          std::ostringstream msg;
          msg << "Expected integer value for option '" << name
              << "'. Cannot cast " << sep_char << value << sep_char
              << " to an integer value.";
          log_and_throw(msg.str());
        }

        if( (!(flex_int(ret_v) >= lower_bound)) || (!(flex_int(ret_v) <= upper_bound))) {
          std::ostringstream msg;
          msg << "Option '" << name << "' must be in the range ["
              << lower_bound << ", " << upper_bound << "].";
          log_and_throw(msg.str());
        }

        break;
      }
    case option_info::BOOL:
      {
        bool value_type_okay = false;

        switch(value.get_type()) {
          case flex_type_enum::INTEGER:
            if(value.get<flex_int>() == 0) {
              ret_v = false;
              value_type_okay = true;
            } else if (value.get<flex_int>() == 1) {
              ret_v = true;
              value_type_okay = true;
            } else {
              value_type_okay = false;
            }
            break;
          case flex_type_enum::FLOAT:
            if(value.get<flex_float>() == 0.0) {
              ret_v = false;
              value_type_okay = true;
            } else if (value.get<flex_float>() == 1.0) {
              ret_v = true;
              value_type_okay = true;
            } else {
              value_type_okay = false;
            }
            break;

          case flex_type_enum::STRING:
            {
              static const std::map<std::string, bool> okay_values =
                  { {"1",      true},
                    {"True",   true},
                    {"T",      true},
                    {"true",   true},
                    {"Y",      true},
                    {"y",      true},
                    {"yes",    true},
                    {"0",      false},
                    {"False",  false},
                    {"F",      false},
                    {"false",  false},
                    {"N",      false},
                    {"n",      false},
                    {"no",     false} };

              auto it = okay_values.find(value.get<flex_string>());

              if(it != okay_values.end()) {
                ret_v = it->second;
                value_type_okay = true;
              } else {
                value_type_okay = false;
              }
            }

          case flex_type_enum::UNDEFINED:
            ret_v = default_value;
            value_type_okay = true;
            break;

          default:
            value_type_okay = false;
            break;
        }


        if(!value_type_okay) {

          std::ostringstream msg;
          msg << "Expected boolean value for option '" << name << sep_char
              << "'. Cannot interpret " << sep_char << value << sep_char
              << " as True or False.";

          log_and_throw(msg.str());
        }

        break;
      }
    case option_info::CATEGORICAL:
      {
        DASSERT_EQ(std::set<flexible_type>(allowed_values.begin(),
                                           allowed_values.end()).count(default_value), 1);

        bool is_okay = false;
        for(const auto& okay_v : allowed_values) {
          if(value == okay_v) {
            is_okay = true;
            break;
          }
        }
        if(!is_okay){
          std::ostringstream msg;
          msg << "Option '" << name << "' must be one of (";
          for(size_t i = 0; i < allowed_values.size()-1; i++){
            msg << sep_char << allowed_values[i] << sep_char << ", ";
          }
          msg << "or " << sep_char << allowed_values[allowed_values.size()-1]
              << sep_char << ").";
          log_and_throw(msg.str());
        }
        ret_v = value;
        break;
      }
    case option_info::STRING:
      ret_v = value;

      // Maybe more on this later?
      break;

    case option_info::FLEXIBLE_TYPE:
      ret_v = value;
      break;

    default:
      log_and_throw("Internal error. Option type un-implemented");
      break;

  }

  return ret_v;
}


/**
 * Check to make sure that the options satisfy the requirements.
*/
void option_info::check_value(const flexible_type& value) const {
  interpret_value(value);
}


/**
 * Save
*/
void option_info::save(turi::oarchive& oarc) const {
  oarc << name << description << default_value << parameter_type
       << lower_bound << upper_bound << allowed_values;
}

/**
 * Load
*/
void option_info::load(turi::iarchive& iarc) {
  iarc >> name >> description >> default_value >> parameter_type
       >> lower_bound >> upper_bound >> allowed_values;
}

}}
