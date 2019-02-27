include(RunCMake)

run_cmake(MATH)
run_cmake(MATH-WrongArgument)
run_cmake(MATH-DoubleOption)
run_cmake(MATH-TooManyArguments)
run_cmake(MATH-InvalidExpression)
run_cmake(MATH-ToleratedExpression)
run_cmake(MATH-DivideByZero)
