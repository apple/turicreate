%{
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run bison like this:

  bison --yacc --name-prefix=cmDependsJava_yy --defines=cmDependsJavaParserTokens.h -ocmDependsJavaParser.cxx cmDependsJavaParser.y

Modify cmDependsJavaParser.cxx:
  - "#if 0" out yyerrorlab block in range ["goto yyerrlab1", "yyerrlab1:"]

*/

#include "cmConfigure.h" // IWYU pragma: keep

#include <stdlib.h>
#include <string.h>
#include <string>

#define yyGetParser (cmDependsJava_yyget_extra(yyscanner))

/*-------------------------------------------------------------------------*/
#include "cmDependsJavaParserHelper.h" /* Interface to parser object.  */
#include "cmDependsJavaLexer.h"  /* Interface to lexer object.  */
#include "cmDependsJavaParserTokens.h" /* Need YYSTYPE for YY_DECL.  */

/* Forward declare the lexer entry point.  */
YY_DECL;

/* Helper function to forward error callback from parser.  */
static void cmDependsJava_yyerror(yyscan_t yyscanner, const char* message);

#define YYMAXDEPTH 1000000


#define jpCheckEmpty(cnt) yyGetParser->CheckEmpty(__LINE__, cnt, yyvsp);
#define jpElementStart(cnt) yyGetParser->PrepareElement(&yyval)
#define jpStoreClass(str) yyGetParser->AddClassFound(str); yyGetParser->DeallocateParserType(&(str))
/* Disable some warnings in the generated code.  */
#ifdef _MSC_VER
# pragma warning (disable: 4102) /* Unused goto label.  */
# pragma warning (disable: 4065) /* Switch statement contains default but no case. */
#endif
#if defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic ignored "-Wconversion"
#endif
%}

/* Generate a reentrant parser object.  */
/* %define api.pure */

/* Configure the parser to use a lexer object.  */
%lex-param   {yyscan_t yyscanner}
%parse-param {yyscan_t yyscanner}

/* %define parse.error verbose */

/*
%union {
  char* string;
}
*/

/*-------------------------------------------------------------------------*/
/* Tokens */
%token jp_ABSTRACT
%token jp_ASSERT
%token jp_BOOLEAN_TYPE
%token jp_BREAK
%token jp_BYTE_TYPE
%token jp_CASE
%token jp_CATCH
%token jp_CHAR_TYPE
%token jp_CLASS
%token jp_CONTINUE
%token jp_DEFAULT
%token jp_DO
%token jp_DOUBLE_TYPE
%token jp_ELSE
%token jp_EXTENDS
%token jp_FINAL
%token jp_FINALLY
%token jp_FLOAT_TYPE
%token jp_FOR
%token jp_IF
%token jp_IMPLEMENTS
%token jp_IMPORT
%token jp_INSTANCEOF
%token jp_INT_TYPE
%token jp_INTERFACE
%token jp_LONG_TYPE
%token jp_NATIVE
%token jp_NEW
%token jp_PACKAGE
%token jp_PRIVATE
%token jp_PROTECTED
%token jp_PUBLIC
%token jp_RETURN
%token jp_SHORT_TYPE
%token jp_STATIC
%token jp_STRICTFP
%token jp_SUPER
%token jp_SWITCH
%token jp_SYNCHRONIZED
%token jp_THIS
%token jp_THROW
%token jp_THROWS
%token jp_TRANSIENT
%token jp_TRY
%token jp_VOID
%token jp_VOLATILE
%token jp_WHILE

%token jp_BOOLEANLITERAL
%token jp_CHARACTERLITERAL
%token jp_DECIMALINTEGERLITERAL
%token jp_FLOATINGPOINTLITERAL
%token jp_HEXINTEGERLITERAL
%token jp_NULLLITERAL
%token jp_STRINGLITERAL

%token jp_NAME

%token jp_AND
%token jp_ANDAND
%token jp_ANDEQUALS
%token jp_BRACKETEND
%token jp_BRACKETSTART
%token jp_CARROT
%token jp_CARROTEQUALS
%token jp_COLON
%token jp_COMMA
%token jp_CURLYEND
%token jp_CURLYSTART
%token jp_DIVIDE
%token jp_DIVIDEEQUALS
%token jp_DOLLAR
%token jp_DOT
%token jp_EQUALS
%token jp_EQUALSEQUALS
%token jp_EXCLAMATION
%token jp_EXCLAMATIONEQUALS
%token jp_GREATER
%token jp_GTEQUALS
%token jp_GTGT
%token jp_GTGTEQUALS
%token jp_GTGTGT
%token jp_GTGTGTEQUALS
%token jp_LESLESEQUALS
%token jp_LESSTHAN
%token jp_LTEQUALS
%token jp_LTLT
%token jp_MINUS
%token jp_MINUSEQUALS
%token jp_MINUSMINUS
%token jp_PAREEND
%token jp_PARESTART
%token jp_PERCENT
%token jp_PERCENTEQUALS
%token jp_PIPE
%token jp_PIPEEQUALS
%token jp_PIPEPIPE
%token jp_PLUS
%token jp_PLUSEQUALS
%token jp_PLUSPLUS
%token jp_QUESTION
%token jp_SEMICOL
%token jp_TILDE
%token jp_TIMES
%token jp_TIMESEQUALS

%token jp_ERROR

/*-------------------------------------------------------------------------*/
/* grammar */
%%

Goal:
CompilationUnit
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Literal:
IntegerLiteral
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_FLOATINGPOINTLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_BOOLEANLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_CHARACTERLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_STRINGLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_NULLLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

IntegerLiteral:
jp_DECIMALINTEGERLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_HEXINTEGERLITERAL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Type:
PrimitiveType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ReferenceType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

PrimitiveType:
jp_BYTE_TYPE
{
  jpElementStart(0);
}
|
jp_SHORT_TYPE
{
  jpElementStart(0);
}
|
jp_INT_TYPE
{
  jpElementStart(0);
}
|
jp_LONG_TYPE
{
  jpElementStart(0);
}
|
jp_CHAR_TYPE
{
  jpElementStart(0);
}
|
jp_FLOAT_TYPE
{
  jpElementStart(0);
}
|
jp_DOUBLE_TYPE
{
  jpElementStart(0);
}
|
jp_BOOLEAN_TYPE
{
  jpElementStart(0);
}

ReferenceType:
ClassOrInterfaceType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ArrayType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassOrInterfaceType:
Name
{
  jpElementStart(1);
  jpStoreClass($<str>1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassType:
ClassOrInterfaceType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

InterfaceType:
ClassOrInterfaceType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ArrayType:
PrimitiveType Dims
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
Name Dims
{
  jpElementStart(2);
  jpStoreClass($<str>1);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Name:
SimpleName
{
  jpElementStart(1);
  $<str>$ = $<str>1;
}
|
QualifiedName
{
  jpElementStart(1);
  $<str>$ = $<str>1;
}

SimpleName:
Identifier
{
  jpElementStart(1);
  $<str>$ = $<str>1;
}

Identifier:
jp_NAME
{
  jpElementStart(1);
  $<str>$ = $<str>1;
}
|
jp_DOLLAR jp_NAME
{
  jpElementStart(2);
  $<str>$ = $<str>2;
}

QualifiedName:
Name jp_DOT Identifier
{
  jpElementStart(3);
  yyGetParser->AddClassFound($<str>1);
  yyGetParser->UpdateCombine($<str>1, $<str>3);
  yyGetParser->DeallocateParserType(&($<str>1));
  $<str>$ = const_cast<char*>(yyGetParser->GetCurrentCombine());
}
|
Name jp_DOT jp_CLASS
{
  jpElementStart(3);
  jpStoreClass($<str>1);
  jpCheckEmpty(3);
  yyGetParser->SetCurrentCombine("");
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
Name jp_DOT jp_THIS
{
  jpElementStart(3);
  jpStoreClass($<str>1);
  yyGetParser->SetCurrentCombine("");
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
SimpleType jp_DOT jp_CLASS
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

SimpleType:
PrimitiveType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_VOID
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

CompilationUnit:
PackageDeclarationopt ImportDeclarations TypeDeclarations
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

PackageDeclarationopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
PackageDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ImportDeclarations:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ImportDeclarations ImportDeclaration
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

TypeDeclarations:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
TypeDeclarations TypeDeclaration
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

PackageDeclaration:
jp_PACKAGE Name jp_SEMICOL
{
  jpElementStart(3);
  yyGetParser->SetCurrentPackage($<str>2);
  yyGetParser->DeallocateParserType(&($<str>2));
  yyGetParser->SetCurrentCombine("");
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ImportDeclaration:
SingleTypeImportDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
TypeImportOnDemandDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

SingleTypeImportDeclaration:
jp_IMPORT Name jp_SEMICOL
{
  jpElementStart(3);
  yyGetParser->AddPackagesImport($<str>2);
  yyGetParser->DeallocateParserType(&($<str>2));
  yyGetParser->SetCurrentCombine("");
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

TypeImportOnDemandDeclaration:
jp_IMPORT Name jp_DOT jp_TIMES jp_SEMICOL
{
  jpElementStart(5);
  std::string str = $<str>2;
  str += ".*";
  yyGetParser->AddPackagesImport(str.c_str());
  yyGetParser->DeallocateParserType(&($<str>2));
  yyGetParser->SetCurrentCombine("");
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

TypeDeclaration:
ClassDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
InterfaceDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
jp_SEMICOL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Modifiers:
Modifier
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
Modifiers Modifier
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Modifier:
jp_PUBLIC | jp_PROTECTED | jp_PRIVATE |
jp_STATIC |
jp_ABSTRACT | jp_FINAL | jp_NATIVE | jp_SYNCHRONIZED | jp_TRANSIENT | jp_VOLATILE |
jp_STRICTFP

ClassHeader:
Modifiersopt jp_CLASS Identifier
{
  yyGetParser->StartClass($<str>3);
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
}


ClassDeclaration:
ClassHeader ClassBody
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
  yyGetParser->EndClass();
}
|
ClassHeader Interfaces ClassBody
{
  jpElementStart(3);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
  yyGetParser->EndClass();
}
|
ClassHeader Super ClassBody
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
  yyGetParser->EndClass();
}
|
ClassHeader Super Interfaces ClassBody
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
  yyGetParser->EndClass();
}

Modifiersopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
Modifiers
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Super:
jp_EXTENDS ClassType
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

Interfaces:
jp_IMPLEMENTS InterfaceTypeList
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

InterfaceTypeList:
InterfaceType
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
InterfaceTypeList jp_COMMA InterfaceType
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassBody:
jp_CURLYSTART ClassBodyDeclarations jp_CURLYEND
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassBodyDeclarations:
{
  jpElementStart(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ClassBodyDeclarations ClassBodyDeclaration
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassBodyDeclaration:
ClassMemberDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
StaticInitializer
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ConstructorDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
TypeDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

ClassMemberDeclaration:
FieldDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
MethodDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

FieldDeclaration:
Modifiersopt Type VariableDeclarators jp_SEMICOL
{
  jpElementStart(4);
}

VariableDeclarators:
VariableDeclarator
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
VariableDeclarators jp_COMMA VariableDeclarator
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

VariableDeclarator:
VariableDeclaratorId
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
VariableDeclaratorId jp_EQUALS VariableInitializer
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

VariableDeclaratorId:
Identifier
{
  jpElementStart(1);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
VariableDeclaratorId jp_BRACKETSTART jp_BRACKETEND
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

VariableInitializer:
Expression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ArrayInitializer
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

MethodDeclaration:
MethodHeader jp_SEMICOL
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
MethodHeader MethodBody
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
MethodHeader MethodBody jp_SEMICOL
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}

MethodHeader:
Modifiersopt Type MethodDeclarator Throwsopt
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Modifiersopt jp_VOID MethodDeclarator Throwsopt
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Throwsopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Throws
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

MethodDeclarator:
Identifier jp_PARESTART FormalParameterListopt jp_PAREEND
{
  jpElementStart(4);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MethodDeclarator jp_BRACKETSTART jp_BRACKETEND
{
  jpElementStart(3);

}

FormalParameterListopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
FormalParameterList

FormalParameterList:
FormalParameter
{
  jpElementStart(1);

}
|
FormalParameterList jp_COMMA FormalParameter
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

FormalParameter:
Modifiersopt Type VariableDeclaratorId
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Throws:
jp_THROWS ClassTypeList
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ClassTypeList:
ClassType
{
  jpElementStart(1);

}
|
ClassTypeList jp_COMMA ClassType
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

MethodBody:
Block
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

StaticInitializer:
jp_STATIC Block
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConstructorDeclaration:
Modifiersopt ConstructorDeclarator Throwsopt ConstructorBody
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Modifiersopt ConstructorDeclarator Throwsopt ConstructorBody jp_SEMICOL
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConstructorDeclarator:
SimpleName jp_PARESTART FormalParameterListopt jp_PAREEND
{
  jpElementStart(4);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConstructorBody:
jp_CURLYSTART ExplicitConstructorInvocationopt BlockStatementsopt jp_CURLYEND
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ExplicitConstructorInvocationopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ExplicitConstructorInvocationopt ExplicitConstructorInvocation
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ExplicitConstructorInvocation:
jp_THIS jp_PARESTART ArgumentListopt jp_PAREEND jp_SEMICOL
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_SUPER jp_PARESTART ArgumentListopt jp_PAREEND jp_SEMICOL
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

InterfaceHeader:
Modifiersopt jp_INTERFACE Identifier
{
  yyGetParser->StartClass($<str>3);
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
}

InterfaceDeclaration:
InterfaceHeader ExtendsInterfacesopt InterfaceBody
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
  yyGetParser->EndClass();
}

ExtendsInterfacesopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");
}
|
ExtendsInterfaces
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ExtendsInterfaces:
jp_EXTENDS InterfaceType
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ExtendsInterfaces jp_COMMA InterfaceType
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

InterfaceBody:
jp_CURLYSTART InterfaceMemberDeclarations jp_CURLYEND
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

InterfaceMemberDeclarations:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
InterfaceMemberDeclarations InterfaceMemberDeclaration
{
  jpElementStart(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

InterfaceMemberDeclaration:
ConstantDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
AbstractMethodDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassDeclaration jp_SEMICOL
{
  jpElementStart(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
InterfaceDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
InterfaceDeclaration jp_SEMICOL
{
  jpElementStart(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConstantDeclaration:
FieldDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AbstractMethodDeclaration:
MethodHeader Semicols
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Semicols:
jp_SEMICOL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Semicols jp_SEMICOL
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ArrayInitializer:
jp_CURLYSTART VariableInitializersOptional jp_CURLYEND
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

VariableInitializersOptional:
{
  jpElementStart(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
VariableInitializers
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
VariableInitializers jp_COMMA
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

VariableInitializers:
VariableInitializer
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
VariableInitializers jp_COMMA VariableInitializer
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Block:
jp_CURLYSTART BlockStatementsopt jp_CURLYEND
{
  jpElementStart(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

BlockStatementsopt:
{
  jpElementStart(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
BlockStatements
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

BlockStatements:
BlockStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
BlockStatements BlockStatement
{
  jpElementStart(1);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

BlockStatement:
LocalVariableDeclarationStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Statement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

LocalVariableDeclarationStatement:
LocalVariableDeclaration jp_SEMICOL
{
  jpElementStart(1);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

LocalVariableDeclaration:
Modifiers Type VariableDeclarators
{
  jpElementStart(1);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Type VariableDeclarators
{
  jpElementStart(1);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Statement:
StatementWithoutTrailingSubstatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
LabeledStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
IfThenStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
IfThenElseStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
WhileStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ForStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

StatementNoShortIf:
StatementWithoutTrailingSubstatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
LabeledStatementNoShortIf
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
IfThenElseStatementNoShortIf
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
WhileStatementNoShortIf
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ForStatementNoShortIf
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

StatementWithoutTrailingSubstatement:
Block
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
EmptyStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ExpressionStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
SwitchStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
DoStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
BreakStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ContinueStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ReturnStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
SynchronizedStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ThrowStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
TryStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
AssertStatement
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

EmptyStatement:
jp_SEMICOL
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

LabeledStatement:
Identifier jp_COLON Statement
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

LabeledStatementNoShortIf:
Identifier jp_COLON StatementNoShortIf
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ExpressionStatement:
StatementExpression jp_SEMICOL
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

StatementExpression:
Assignment
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PreIncrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PreDecrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PostIncrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PostDecrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MethodInvocation
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassInstanceCreationExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

IfThenStatement:
jp_IF jp_PARESTART Expression jp_PAREEND Statement
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

IfThenElseStatement:
jp_IF jp_PARESTART Expression jp_PAREEND StatementNoShortIf jp_ELSE Statement
{
  jpElementStart(7);
  jpCheckEmpty(7);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

IfThenElseStatementNoShortIf:
jp_IF jp_PARESTART Expression jp_PAREEND StatementNoShortIf jp_ELSE StatementNoShortIf
{
  jpElementStart(7);
  jpCheckEmpty(7);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SwitchStatement:
jp_SWITCH jp_PARESTART Expression jp_PAREEND SwitchBlock
{
  jpElementStart(5);

}

SwitchBlock:
jp_CURLYSTART SwitchBlockStatementGroups SwitchLabelsopt jp_CURLYEND
{
  jpElementStart(4);

}

SwitchLabelsopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
SwitchLabels
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SwitchBlockStatementGroups:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
SwitchBlockStatementGroups SwitchBlockStatementGroup
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SwitchBlockStatementGroup:
SwitchLabels BlockStatements
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SwitchLabels:
SwitchLabel
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
SwitchLabels SwitchLabel
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SwitchLabel:
jp_CASE ConstantExpression jp_COLON
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_DEFAULT jp_COLON
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

WhileStatement:
jp_WHILE jp_PARESTART Expression jp_PAREEND Statement
{
  jpElementStart(5);

}

WhileStatementNoShortIf:
jp_WHILE jp_PARESTART Expression jp_PAREEND StatementNoShortIf
{
  jpElementStart(5);

}

DoStatement:
jp_DO Statement jp_WHILE jp_PARESTART Expression jp_PAREEND jp_SEMICOL
{
  jpElementStart(7);

}

ForStatement:
jp_FOR jp_PARESTART ForInitopt jp_SEMICOL Expressionopt jp_SEMICOL ForUpdateopt jp_PAREEND
Statement
{
  jpElementStart(9);

}

ForUpdateopt:
{
  jpElementStart(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ForUpdate
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ForInitopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ForInit
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ForStatementNoShortIf:
jp_FOR jp_PARESTART ForInitopt jp_SEMICOL Expressionopt jp_SEMICOL ForUpdateopt jp_PAREEND
StatementNoShortIf
{
  jpElementStart(9);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Expressionopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Expression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ForInit:
StatementExpressionList
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
LocalVariableDeclaration
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ForUpdate:
StatementExpressionList
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

StatementExpressionList:
StatementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
StatementExpressionList jp_COMMA StatementExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AssertStatement:
jp_ASSERT Expression jp_SEMICOL
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_ASSERT Expression jp_COLON Expression jp_SEMICOL
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

BreakStatement:
jp_BREAK Identifieropt jp_SEMICOL
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>2));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Identifieropt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Identifier
{
  jpElementStart(1);

}

ContinueStatement:
jp_CONTINUE Identifieropt jp_SEMICOL
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>2));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ReturnStatement:
jp_RETURN Expressionopt jp_SEMICOL
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ThrowStatement:
jp_THROW Expression jp_SEMICOL
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

SynchronizedStatement:
jp_SYNCHRONIZED jp_PARESTART Expression jp_PAREEND Block
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

TryStatement:
jp_TRY Block Catches
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_TRY Block Catchesopt Finally
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Catchesopt:
{
  jpElementStart(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Catches
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Catches:
CatchClause
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Catches CatchClause
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

CatchClause:
jp_CATCH jp_PARESTART FormalParameter jp_PAREEND Block
{
  jpElementStart(5);

}

Finally:
jp_FINALLY Block
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Primary:
PrimaryNoNewArray
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArrayCreationExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PrimaryNoNewArray:
Literal
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_THIS
{
  jpElementStart(1);

}
|
jp_PARESTART Expression jp_PAREEND
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassInstanceCreationExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
FieldAccess
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MethodInvocation
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArrayAccess
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ClassInstanceCreationExpression:
New ClassType jp_PARESTART ArgumentListopt jp_PAREEND ClassBodyOpt
{
  jpElementStart(6);
  jpCheckEmpty(6);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ClassBodyOpt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ClassBody
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ArgumentListopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArgumentList
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ArgumentList:
Expression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArgumentList jp_COMMA Expression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ArrayCreationExpression:
New PrimitiveType DimExprs Dimsopt
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
New ClassOrInterfaceType DimExprs Dimsopt
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
New PrimitiveType Dims ArrayInitializer
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
New ClassOrInterfaceType Dims ArrayInitializer
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Dimsopt:
{
  jpElementStart(0);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Dims
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

DimExprs:
DimExpr
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
DimExprs DimExpr
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

DimExpr:
jp_BRACKETSTART Expression jp_BRACKETEND
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Dims:
jp_BRACKETSTART jp_BRACKETEND
{
  jpElementStart(2);

}
|
Dims jp_BRACKETSTART jp_BRACKETEND
{
  jpElementStart(3);

}

FieldAccess:
Primary jp_DOT Identifier
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_SUPER jp_DOT Identifier
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_THIS jp_DOT Identifier
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Primary jp_DOT jp_THIS
{
  jpElementStart(3);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

MethodInvocation:
Name jp_PARESTART ArgumentListopt jp_PAREEND
{
  jpElementStart(4);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Primary jp_DOT Identifier jp_PARESTART ArgumentListopt jp_PAREEND
{
  jpElementStart(6);
  yyGetParser->DeallocateParserType(&($<str>1));
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(6);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_SUPER jp_DOT Identifier jp_PARESTART ArgumentListopt jp_PAREEND
{
  jpElementStart(6);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(6);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_THIS jp_DOT Identifier jp_PARESTART ArgumentListopt jp_PAREEND
{
  jpElementStart(6);
  yyGetParser->DeallocateParserType(&($<str>3));
  jpCheckEmpty(6);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ArrayAccess:
Name jp_BRACKETSTART Expression jp_BRACKETEND
{
  jpElementStart(4);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PrimaryNoNewArray jp_BRACKETSTART Expression jp_BRACKETEND
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PostfixExpression:
Primary
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Name
{
  jpElementStart(1);
  yyGetParser->DeallocateParserType(&($<str>1));
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArrayType jp_DOT jp_CLASS
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PostIncrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PostDecrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PostIncrementExpression:
PostfixExpression jp_PLUSPLUS
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PostDecrementExpression:
PostfixExpression jp_MINUSMINUS
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

UnaryExpression:
PreIncrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
PreDecrementExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PLUS UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_MINUS UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
UnaryExpressionNotPlusMinus
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PreIncrementExpression:
jp_PLUSPLUS UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

PreDecrementExpression:
jp_MINUSMINUS UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

UnaryExpressionNotPlusMinus:
PostfixExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_TILDE UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_EXCLAMATION UnaryExpression
{
  jpElementStart(2);
  jpCheckEmpty(2);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
CastExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

CastExpression:
jp_PARESTART PrimitiveType Dimsopt jp_PAREEND UnaryExpression
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PARESTART Expression jp_PAREEND UnaryExpressionNotPlusMinus
{
  jpElementStart(4);
  jpCheckEmpty(4);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PARESTART Name Dims jp_PAREEND UnaryExpressionNotPlusMinus
{
  jpElementStart(5);

}

MultiplicativeExpression:
UnaryExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MultiplicativeExpression jp_TIMES UnaryExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MultiplicativeExpression jp_DIVIDE UnaryExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
MultiplicativeExpression jp_PERCENT UnaryExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AdditiveExpression:
MultiplicativeExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
AdditiveExpression jp_PLUS MultiplicativeExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
AdditiveExpression jp_MINUS MultiplicativeExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ShiftExpression:
AdditiveExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ShiftExpression jp_LTLT AdditiveExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ShiftExpression jp_GTGT AdditiveExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ShiftExpression jp_GTGTGT AdditiveExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

RelationalExpression:
ShiftExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
RelationalExpression jp_LESSTHAN ShiftExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
RelationalExpression jp_GREATER ShiftExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
RelationalExpression jp_LTEQUALS ShiftExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
RelationalExpression jp_GTEQUALS ShiftExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
RelationalExpression jp_INSTANCEOF ReferenceType
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

EqualityExpression:
RelationalExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
EqualityExpression jp_EQUALSEQUALS RelationalExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
EqualityExpression jp_EXCLAMATIONEQUALS RelationalExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AndExpression:
EqualityExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
AndExpression jp_AND EqualityExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ExclusiveOrExpression:
AndExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ExclusiveOrExpression jp_CARROT AndExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

InclusiveOrExpression:
ExclusiveOrExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
InclusiveOrExpression jp_PIPE ExclusiveOrExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConditionalAndExpression:
InclusiveOrExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ConditionalAndExpression jp_ANDAND InclusiveOrExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConditionalOrExpression:
ConditionalAndExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ConditionalOrExpression jp_PIPEPIPE ConditionalAndExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConditionalExpression:
ConditionalOrExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ConditionalOrExpression jp_QUESTION Expression jp_COLON ConditionalExpression
{
  jpElementStart(5);
  jpCheckEmpty(5);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AssignmentExpression:
ConditionalExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Assignment
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Assignment:
LeftHandSide AssignmentOperator AssignmentExpression
{
  jpElementStart(3);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

LeftHandSide:
Name
{
  jpElementStart(1);
  yyGetParser->DeallocateParserType(&($<str>1));
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
FieldAccess
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
ArrayAccess
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

AssignmentOperator:
jp_EQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_TIMESEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_DIVIDEEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PERCENTEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PLUSEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_MINUSEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_LESLESEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_GTGTEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_GTGTGTEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_ANDEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_CARROTEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
jp_PIPEEQUALS
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

Expression:
AssignmentExpression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

ConstantExpression:
Expression
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

New:
jp_NEW
{
  jpElementStart(1);
  jpCheckEmpty(1);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}
|
Name jp_DOT jp_NEW
{
  jpElementStart(3);
  jpStoreClass($<str>1);
  jpCheckEmpty(3);
  $<str>$ = 0;
  yyGetParser->SetCurrentCombine("");

}

%%
/* End of grammar */

/*--------------------------------------------------------------------------*/
void cmDependsJava_yyerror(yyscan_t yyscanner, const char* message)
{
  yyGetParser->Error(message);
}
