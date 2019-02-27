include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "file:///path/to/%(algo:)/%(hash)"
  )
ExternalData_Add_Target(Data)
