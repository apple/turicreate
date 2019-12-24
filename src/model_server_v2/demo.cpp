#include <string>
#include <turi_common.h>
#include <model_server_v2/model_base.hpp>
#include <model_server_v2/registration.hpp>

using namespace turi;
using namespace turi::v2;

/** Demo model. 
 *
 */ 
class demo_model : public turi::v2::model_base { 

 public:

   /** The name of the model. 
    *
    */
  const char* name() const { return "demo_model"; } 

  /** Sets up the options and the registration.
   *  
   *  The registration is done in the constructor, without the use of macros. 
   */
  demo_model() { 
    register_method("add", &demo_model::add, "x", "y"); 
    register_method("concat_strings", &demo_model::append_strings, "s1", "s2");

    // Defaults are specified inline
    register_method("increment", &demo_model::increment, "x", Parameter("delta", 1));
  } 

  /** Add two numbers.
   *
   *  Const is fine.
   */
  size_t add(size_t x, size_t y) const { 
    return x + y; 
  }

  /** Append two strings with a +
   */
  std::string append_strings(const std::string& s1, const std::string& s2) const 
  {
    return s1 + "+" + s2;
  }
 
  /** Incerment a value.
   */ 
  size_t increment(size_t x, size_t delta) const { 
    return x + delta; 
  }

};

/** Registration for a model is just a single macro in the header.  
 *
 *  This automatically loads and registers the model when the library is loaded. 
 *  This registration is trivially cheap.
 */
REGISTER_MODEL(demo_model);


void hello_world(const std::string& greeting) {
  std::cout << "Hello, world!!  " << greeting << std::endl;   
}

/** Registration for a function is just a single macro in a source file or header.
 *
 *  This automatically loads and registers the function when the library is loaded. 
 */
REGISTER_FUNCTION(hello_world, "greeting"); 



int main(int argc, char** argv) { 

 
  auto dm = model_server().create_model("demo_model"); 

  std::string name = variant_get_value<std::string>(dm->call_method("name"));

  std::cout << "Demoing model = " << name << std::endl; 

  size_t result = variant_get_value<size_t>(dm->call_method("add", 5, 9));

  std::cout << "5 + 9 = " << result << std::endl; 

  std::string s_res = variant_get_value<std::string>(dm->call_method("concat_strings", "A", "B"));

  std::cout << "Concat A, +, B: " << s_res << std::endl; 
  
  // Delta default is 1 
  size_t inc_value = variant_get_value<size_t>(dm->call_method("increment", 5)); 

  std::cout << "Incremented 5: " << inc_value << std::endl; 


  // Call the registered function.
  std::cout << "Calling hello_world." << std::endl;
  model_server().call_function("hello_world", "This works!"); 


  return 0; 

} 
