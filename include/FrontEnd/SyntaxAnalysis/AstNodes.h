#ifndef SYNTAXANALYSIS_ASTNODES_H
#define SYNTAXANALYSIS_ASTNODES_H
#include "AstNode/Declaration/Declarator/Initializer/ExpInitializer.h"
#include "AstNode/Declaration/Declarator/Initializer/ListInitializer.h"
#include "AstNode/Declaration/Declarator/DirectDeclarator.h"
#include "AstNode/Declaration/Declarator/InitialDeclarator.h"

#include "AstNode/Declaration/VariableDecl.h"
#include "AstNode/Declaration/FunctionDecl.h"
#include "AstNode/Declaration/ParameterDecl.h"

#include "AstNode/Expression/BinaryExp.h"
#include "AstNode/Expression/PostfixExp.h"
#include "AstNode/Expression/PrimaryExp.h"
#include "AstNode/Expression/UnaryExp.h"

#include "AstNode/Statement/ExpStmt.h"
#include "AstNode/Statement/JumpStmt.h"
#include "AstNode/Statement/SelectStmt.h"
#include "AstNode/Statement/CompStmt.h"
#include "AstNode/Statement/IteraStmt.h"

#include "AstNode/CompUnit.h"
#endif