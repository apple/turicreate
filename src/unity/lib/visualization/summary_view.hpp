#ifndef _TC_SUMMARY_VIEW
#define _TC_SUMMARY_VIEW

#include <flexible_type/flexible_type.hpp>
#include <parallel/lambda_omp.hpp>

#include "transformation.hpp"

namespace turi {
namespace visualization {

class summary_view_transformation_output : public transformation_output {
  private:
    std::vector<std::shared_ptr<transformation_output>> m_outputs;
  public:
    std::vector<std::string> m_column_names;
    std::vector<flex_type_enum> m_column_types;
    size_t m_size;
    size_t m_index;

    summary_view_transformation_output(const std::vector<std::shared_ptr<transformation_output>> outputs, std::vector<std::string> column_names, std::vector<flex_type_enum> column_types, size_t size, size_t index);
    virtual std::string vega_column_data(bool sframe = false) const override;
};

class summary_view_transformation : public transformation_base {
  private:
    std::vector<std::shared_ptr<transformation_base>> m_transformers;
    size_t m_index = 0;

  public:
    std::vector<std::string> m_column_names;
    std::vector<flex_type_enum> m_column_types;
    size_t m_size;

    summary_view_transformation(const std::vector<std::shared_ptr<transformation_base>> transformers, std::vector<std::string> column_names, std::vector<flex_type_enum> column_types, size_t size);

    virtual std::shared_ptr<transformation_output> get() override;
    virtual bool eof() const override;
    virtual flex_int get_rows_processed() const override;
    virtual size_t get_batch_size() const override;
};

}
}


#endif
