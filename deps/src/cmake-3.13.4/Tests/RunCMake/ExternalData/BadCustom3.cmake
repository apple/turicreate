include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "ExternalDataCustomScript://MissingKey/%(algo)/%(hash)"
  )
ExternalData_Add_Target(Data)
