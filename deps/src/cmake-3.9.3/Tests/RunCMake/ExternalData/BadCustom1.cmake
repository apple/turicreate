include(ExternalData)
set(ExternalData_URL_TEMPLATES
  "ExternalDataCustomScript://0BadKey/%(algo)/%(hash)"
  )
ExternalData_Add_Target(Data)
