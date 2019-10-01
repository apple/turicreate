include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "ExternalDataCustomScript:///%(algo)/%(hash)"
  )
ExternalData_Add_Target(Data)
