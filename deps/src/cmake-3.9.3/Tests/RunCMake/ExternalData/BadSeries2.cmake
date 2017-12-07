include(ExternalData)
set(ExternalData_SERIES_PARSE "(x)(y)$")
ExternalData_Expand_Arguments(Data args DATA{Data.txt,:})
