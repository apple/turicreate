include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "ExternalDataCustomScript://RelPathKey/%(algo)/%(hash)"
  )
set(ExternalData_CUSTOM_SCRIPT_RelPathKey "RelPathScript.cmake")
ExternalData_Add_Target(Data)
