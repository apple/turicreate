#ifndef MLMODEL_RESULT_HPP
#define MLMODEL_RESULT_HPP

#include <string>
#include <vector>

namespace CoreML {

class FeatureType;
enum class ResultReason;
enum class ResultType;

class Result {

  private:
    ResultType m_type;
    ResultReason m_reason;
    std::string m_message;

  public:
    Result(); 
    bool good() const;
    Result(ResultType type, const std::string& message); 
    Result(ResultType type, ResultReason reason, const std::string& message);
    const std::string& message() const;
    ResultReason reason() const;
    ResultType type() const;

    static Result typeMismatchError(
        const FeatureType& expected,
        const FeatureType& actual,
        const std::string& parameterName);

    static Result featureTypeInvariantError(
        const std::vector<FeatureType>& allowed,
        const FeatureType& actual);

    
    bool operator==(const Result& other) const;
    bool operator!=(const Result& other) const;
};
}

/* 
 * A convenience macro to pass results onto the caller. Useful when a function
 * both returns a Result and calls other functions that return a Result, and
 * the desired behavior is an early exit in the case of a failure. 
*/
#define HANDLE_RESULT_AND_RETURN_ON_ERROR(EXPR)                                \
  {                                                                            \
    Result _result = (EXPR);                                                   \
    if (!_result.good()) {                                                     \
      return _result;                                                          \
    }                                                                          \
  }                                                                            \

#endif
