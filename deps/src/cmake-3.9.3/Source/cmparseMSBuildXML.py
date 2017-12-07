# This python script parses the spec files from MSBuild to create
# mappings from compiler options to IDE XML specifications.  For
# more information see here:

#  http://blogs.msdn.com/vcblog/archive/2008/12/16/msbuild-task.aspx
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/1033/cl.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/1033/lib.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/1033/link.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V110/1033/cl.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V110/1033/lib.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V110/1033/link.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/v120/1033/cl.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/v120/1033/lib.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/v120/1033/link.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V140/1033/cl.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V140/1033/lib.xml"
#  "${PROGRAMFILES}/MSBuild/Microsoft.Cpp/v4.0/V140/1033/link.xml"
#  "${PROGRAMFILES}/Microsoft Visual Studio/VS15Preview/Common7/IDE/VC/VCTargets/1033/cl.xml"
#  "${PROGRAMFILES}/Microsoft Visual Studio/VS15Preview/Common7/IDE/VC/VCTargets/1033/lib.xml"
#  "${PROGRAMFILES}/Microsoft Visual Studio/VS15Preview/Common7/IDE/VC/VCTargets/1033/link.xml"
#
#  BoolProperty  <Name>true|false</Name>
#   simple example:
#     <BoolProperty ReverseSwitch="Oy-" Name="OmitFramePointers"
#      Category="Optimization" Switch="Oy">
#   <BoolProperty.DisplayName>  <BoolProperty.Description>
# <CLCompile>
#     <OmitFramePointers>true</OmitFramePointers>
#  </ClCompile>
#
#  argument means it might be this: /MP3
#   example with argument:
#   <BoolProperty Name="MultiProcessorCompilation" Category="General" Switch="MP">
#      <BoolProperty.DisplayName>
#        <sys:String>Multi-processor Compilation</sys:String>
#      </BoolProperty.DisplayName>
#      <BoolProperty.Description>
#        <sys:String>Multi-processor Compilation</sys:String>
#      </BoolProperty.Description>
#      <Argument Property="ProcessorNumber" IsRequired="false" />
#    </BoolProperty>
# <CLCompile>
#   <MultiProcessorCompilation>true</MultiProcessorCompilation>
#   <ProcessorNumber>4</ProcessorNumber>
#  </ClCompile>
#  IntProperty
#     not used AFIT
#  <IntProperty Name="ProcessorNumber" Category="General" Visible="false">


#  per config options example
#    <EnableFiberSafeOptimizations Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">true</EnableFiberSafeOptimizations>
#
#  EnumProperty
#   <EnumProperty Name="Optimization" Category="Optimization">
#      <EnumProperty.DisplayName>
#       <sys:String>Optimization</sys:String>
#     </EnumProperty.DisplayName>
#     <EnumProperty.Description>
#       <sys:String>Select option for code optimization; choose Custom to use specific optimization options.     (/Od, /O1, /O2, /Ox)</sys:String>
#     </EnumProperty.Description>
#      <EnumValue Name="MaxSpeed" Switch="O2">
#       <EnumValue.DisplayName>
#         <sys:String>Maximize Speed</sys:String>
#       </EnumValue.DisplayName>
#       <EnumValue.Description>
#         <sys:String>Equivalent to /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy</sys:String>
#       </EnumValue.Description>
#     </EnumValue>
#     <EnumValue Name="MinSpace" Switch="O1">
#       <EnumValue.DisplayName>
#         <sys:String>Minimize Size</sys:String>
#       </EnumValue.DisplayName>
#       <EnumValue.Description>
#         <sys:String>Equivalent to /Og /Os /Oy /Ob2 /Gs /GF /Gy</sys:String>
#       </EnumValue.Description>
#     </EnumValue>
#     example for O2 would be this:
#     <Optimization>MaxSpeed</Optimization>
#     example for O1 would be this:
#     <Optimization>MinSpace</Optimization>
#
#  StringListProperty
#   <StringListProperty Name="PreprocessorDefinitions" Category="Preprocessor" Switch="D ">
#     <StringListProperty.DisplayName>
#       <sys:String>Preprocessor Definitions</sys:String>
#     </StringListProperty.DisplayName>
#     <StringListProperty.Description>
#       <sys:String>Defines a preprocessing symbols for your source file.</sys:String>
#     </StringListProperty.Description>
#   </StringListProperty>

#   <StringListProperty Subtype="folder" Name="AdditionalIncludeDirectories" Category="General" Switch="I">
#     <StringListProperty.DisplayName>
#       <sys:String>Additional Include Directories</sys:String>
#     </StringListProperty.DisplayName>
#     <StringListProperty.Description>
#       <sys:String>Specifies one or more directories to add to the include path; separate with semi-colons if more than one.     (/I[path])</sys:String>
#     </StringListProperty.Description>
#   </StringListProperty>
#  StringProperty

# Example add bill include:

#   <AdditionalIncludeDirectories>..\..\..\..\..\..\bill;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>


import sys
from xml.dom.minidom import parse, parseString

def getText(node):
    nodelist = node.childNodes
    rc = ""
    for child in nodelist:
        if child.nodeType == child.TEXT_NODE:
            rc = rc + child.data
    return rc

def print_tree(document, spaces=""):
  for i in range(len(document.childNodes)):
    if document.childNodes[i].nodeType == document.childNodes[i].ELEMENT_NODE:
      print spaces+str(document.childNodes[i].nodeName )
    print_tree(document.childNodes[i],spaces+"----")
  pass

###########################################################################################
#Data structure that stores a property of MSBuild
class Property:
  #type = type of MSBuild property (ex. if the property is EnumProperty type should be "Enum")
  #attributeNames = a list of any attributes that this property could have (ex. if this was a EnumProperty it should be ["Name","Category"])
  #document = the dom file that's root node is the Property node (ex. if you were parsing a BoolProperty the root node should be something like <BoolProperty Name="RegisterOutput" Category="General" IncludeInCommandLine="false">
  def __init__(self,type,attributeNames,document=None):
    self.suffix_type = "Property"
    self.prefix_type = type
    self.attributeNames = attributeNames
    self.attributes = {}
    self.DisplayName = ""
    self.Description = ""
    self.argumentProperty = ""
    self.argumentIsRequired = ""
    self.values = []
    if document is not None:
      self.populate(document)
    pass

  #document = the dom file that's root node is the Property node (ex. if you were parsing a BoolProperty the root node should be something like <BoolProperty Name="RegisterOutput" Category="General" IncludeInCommandLine="false">
  #spaces = do not use
  def populate(self,document, spaces = ""):
    if document.nodeName == self.prefix_type+self.suffix_type:
      for i in self.attributeNames:
        self.attributes[i] = document.getAttribute(i)
    for i in range(len(document.childNodes)):
      child = document.childNodes[i]
      if child.nodeType == child.ELEMENT_NODE:
        if child.nodeName == self.prefix_type+self.suffix_type+".DisplayName":
          self.DisplayName = getText(child.childNodes[1])
        if child.nodeName == self.prefix_type+self.suffix_type+".Description":
          self.Description = getText(child.childNodes[1])
        if child.nodeName == "Argument":
          self.argumentProperty = child.getAttribute("Property")
          self.argumentIsRequired = child.getAttribute("IsRequired")
        if child.nodeName == self.prefix_type+"Value":
          va = Property(self.prefix_type,["Name","DisplayName","Switch"])
          va.suffix_type = "Value"
          va.populate(child)
          self.values.append(va)
      self.populate(child,spaces+"----")
      pass

  #toString function
  def __str__(self):
    toReturn = self.prefix_type+self.suffix_type+":"
    for i in self.attributeNames:
      toReturn += "\n    "+i+": "+self.attributes[i]
    if self.argumentProperty != "":
      toReturn += "\n    Argument:\n        Property: "+self.argumentProperty+"\n        IsRequired: "+self.argumentIsRequired
    for i in self.values:
        toReturn+="\n    "+str(i).replace("\n","\n    ")
    return toReturn
###########################################################################################

###########################################################################################
#Class that populates itself from an MSBuild file and outputs it in CMake
#format

class MSBuildToCMake:
  #document = the entire MSBuild xml file
  def __init__(self,document=None):
    self.enumProperties = []
    self.stringProperties = []
    self.stringListProperties = []
    self.boolProperties = []
    self.intProperties = []
    if document!=None :
      self.populate(document)
    pass

  #document = the entire MSBuild xml file
  #spaces = don't use
  #To add a new property (if they exist) copy and paste this code and fill in appropriate places
  #
  #if child.nodeName == "<Name>Property":
  #        self.<Name>Properties.append(Property("<Name>",[<List of attributes>],child))
  #
  #Replace <Name> with the name of the new property (ex. if property is StringProperty replace <Name> with String)
  #Replace <List of attributes> with a list of attributes in your property's root node
  #in the __init__ function add the line self.<Name>Properties = []
  #
  #That is all that is required to add new properties
  #
  def populate(self,document, spaces=""):
    for i in range(len(document.childNodes)):
      child = document.childNodes[i]
      if child.nodeType == child.ELEMENT_NODE:
        if child.nodeName == "EnumProperty":
          self.enumProperties.append(Property("Enum",["Name","Category"],child))
        if child.nodeName == "StringProperty":
          self.stringProperties.append(Property("String",["Name","Subtype","Separator","Category","Visible","IncludeInCommandLine","Switch","DisplayName","ReadOnly"],child))
        if child.nodeName == "StringListProperty":
           self.stringListProperties.append(Property("StringList",["Name","Category","Switch","DisplayName","Subtype"],child))
        if child.nodeName == "BoolProperty":
           self.boolProperties.append(Property("Bool",["ReverseSwitch","Name","Category","Switch","DisplayName","SwitchPrefix","IncludeInCommandLine"],child))
        if child.nodeName == "IntProperty":
           self.intProperties.append(Property("Int",["Name","Category","Visible"],child))
      self.populate(child,spaces+"----")
    pass

  #outputs information that CMake needs to know about MSBuild xml files
  def toCMake(self):
    toReturn = "static cmVS7FlagTable cmVS10CxxTable[] =\n{\n"
    toReturn += "\n  //Enum Properties\n"
    lastProp = {}
    for i in self.enumProperties:
      if i.attributes["Name"] == "CompileAsManaged":
        #write these out after the rest of the enumProperties
        lastProp = i
        continue
      for j in i.values:
        #hardcore Brad King's manual fixes for cmVS10CLFlagTable.h
        if i.attributes["Name"] == "PrecompiledHeader" and j.attributes["Switch"] != "":
          toReturn+="  {\""+i.attributes["Name"]+"\", \""+j.attributes["Switch"]+"\",\n   \""+j.attributes["DisplayName"]+"\", \""+j.attributes["Name"]+"\",\n   cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue},\n"
        else:
          #default (normal, non-hardcoded) case
          toReturn+="  {\""+i.attributes["Name"]+"\", \""+j.attributes["Switch"]+"\",\n   \""+j.attributes["DisplayName"]+"\", \""+j.attributes["Name"]+"\", 0},\n"
      toReturn += "\n"

    if lastProp != {}:
      for j in lastProp.values:
          toReturn+="  {\""+lastProp.attributes["Name"]+"\", \""+j.attributes["Switch"]+"\",\n   \""+j.attributes["DisplayName"]+"\", \""+j.attributes["Name"]+"\", 0},\n"
      toReturn += "\n"

    toReturn += "\n  //Bool Properties\n"
    for i in self.boolProperties:
      if i.argumentProperty == "":
        if i.attributes["ReverseSwitch"] != "":
          toReturn += "  {\""+i.attributes["Name"]+"\", \""+i.attributes["ReverseSwitch"]+"\", \"\", \"false\", 0},\n"
        if i.attributes["Switch"] != "":
          toReturn += "  {\""+i.attributes["Name"]+"\", \""+i.attributes["Switch"]+"\", \"\", \"true\", 0},\n"

    toReturn += "\n  //Bool Properties With Argument\n"
    for i in self.boolProperties:
      if i.argumentProperty != "":
        if i.attributes["ReverseSwitch"] != "":
          toReturn += "  {\""+i.attributes["Name"]+"\", \""+i.attributes["ReverseSwitch"]+"\", \"\", \"false\",\n   cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue},\n"
          toReturn += "  {\""+i.attributes["Name"]+"\", \""+i.attributes["ReverseSwitch"]+"\", \""+i.attributes["DisplayName"]+"\", \"\",\n   cmVS7FlagTable::UserValueRequired},\n"
        if i.attributes["Switch"] != "":
          toReturn += "  {\""+i.attributes["Name"]+"\", \""+i.attributes["Switch"]+"\", \"\", \"true\",\n   cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue},\n"
          toReturn += "  {\""+i.argumentProperty+"\", \""+i.attributes["Switch"]+"\", \""+i.attributes["DisplayName"]+"\", \"\",\n   cmVS7FlagTable::UserValueRequired},\n"

    toReturn += "\n  //String List Properties\n"
    for i in self.stringListProperties:
      if i.attributes["Switch"] == "":
        toReturn += "  // Skip [" + i.attributes["Name"] + "] - no command line Switch.\n";
      else:
        toReturn +="  {\""+i.attributes["Name"]+"\", \""+i.attributes["Switch"]+"\",\n   \""+i.attributes["DisplayName"]+"\",\n   \"\", cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable},\n"

    toReturn += "\n  //String Properties\n"
    for i in self.stringProperties:
      if i.attributes["Switch"] == "":
        if i.attributes["Name"] == "PrecompiledHeaderFile":
          #more hardcoding
          toReturn += "  {\"PrecompiledHeaderFile\", \"Yc\",\n"
          toReturn += "   \"Precompiled Header Name\",\n"
          toReturn += "   \"\", cmVS7FlagTable::UserValueRequired},\n"
          toReturn += "  {\"PrecompiledHeaderFile\", \"Yu\",\n"
          toReturn += "   \"Precompiled Header Name\",\n"
          toReturn += "   \"\", cmVS7FlagTable::UserValueRequired},\n"
        else:
          toReturn += "  // Skip [" + i.attributes["Name"] + "] - no command line Switch.\n";
      else:
        toReturn +="  {\""+i.attributes["Name"]+"\", \""+i.attributes["Switch"]+i.attributes["Separator"]+"\",\n   \""+i.attributes["DisplayName"]+"\",\n   \"\", cmVS7FlagTable::UserValue},\n"

    toReturn += "  {0,0,0,0,0}\n};"
    return toReturn
    pass

  #toString function
  def __str__(self):
    toReturn = ""
    allList = [self.enumProperties,self.stringProperties,self.stringListProperties,self.boolProperties,self.intProperties]
    for p in allList:
      for i in p:
        toReturn += "==================================================\n"+str(i).replace("\n","\n    ")+"\n==================================================\n"

    return toReturn
###########################################################################################

###########################################################################################
# main function
def main(argv):
  xml_file = None
  help = """
  Please specify an input xml file with -x

  Exiting...
  Have a nice day :)"""
  for i in range(0,len(argv)):
    if argv[i] == "-x":
      xml_file = argv[i+1]
    if argv[i] == "-h":
      print help
      sys.exit(0)
    pass
  if xml_file == None:
    print help
    sys.exit(1)

  f = open(xml_file,"r")
  xml_str = f.read()
  xml_dom = parseString(xml_str)

  convertor = MSBuildToCMake(xml_dom)
  print convertor.toCMake()

  xml_dom.unlink()
###########################################################################################
# main entry point
if __name__ == "__main__":
  main(sys.argv)

sys.exit(0)
