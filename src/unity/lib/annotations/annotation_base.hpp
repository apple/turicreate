#ifndef TURI_ANNOTATIONS_ANNOTATION_BASE_HPP
#define TURI_ANNOTATIONS_ANNOTATION_BASE_HPP

#include <unity/lib/gl_sframe.hpp>
#include <string>
#include <map>
#include "utils.hpp"

namespace turi {
namespace annotate{

class AnnotationBase {
	public:
		AnnotationBase(){};
		AnnotationBase(const gl_sframe& data,
					   const std::string& data_column,
					   const std::string& annotation_column);
		void start();
		
		// virtual method that must be implemented by the inhereting classes
		
		//// virtual gl_sframe get_items();

		//// virtual void add_annotation(std::map<>);
		
		// Return Annotation SFrame from SFrame builder	
		// Add Annotation method
		// modify annotation
	private:
		gl_sframe m_data;
		std::string m_data_column;
		std::string m_string_column;
		
		
		// Base class should store the reference to the data being annotated
		// SFrame builder to store all the annotated data
		// sorted heuristic
};

}
}

#endif
