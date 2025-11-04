# BUAA_SysY_Compiler_2025

## 文法

### Exp

#### 分类
BinaryExp:CommaExp, AddExp, MulExp, OrExp, AndExp, RelExp

AssignExp

UnaryExp: PlusExp, MinusExp, NotExp

PostfixExp: SubscriptExp, FunctionCallExp

PrimaryExp: LiteralExp, IdentifierExp

#### 文法

BinaryExp -> BinaryExp BinaryOp UnaryExp | UnaryExp

**AssignExp视作一种特殊的BinaryExp**

UnaryExp -> UnaryOp UnaryExp | PostfixExp

PostfixExp -> SubscriptExp '['Exp']' | FunctionCallExp '('[RealParams]')' | PrimaryExp

RealParams -> Exp {,Exp}

Primary -> LiteralExp | IdentifierExp

IdentifierExp -> Identifier ;

LiteralExp -> number | string

### Stmt

#### 分类

Stmt:BlockStmt, ExpStmt, ForStmt, IfStmt,  ContinueStmt, BreakStmt, ReturnStmt, VariableDecl, FunctionDecl

#### 文法

Stmt -> BlockStmt | ExpStmt | ... | FunctionDecl

1. CompoundStmt

BlockStmt -> '{'{Stmt}'}'

2. ExpressionStmt

ExpStmt -> Exp;

3. IterationExp

ForStmt -> for'('[Exp];[Exp];[Exp]')'Stmt

4. SelectionStmt

IfStmt -> if([Exp])Stmt[else Stmt]

5. JumpStmt

ReturnStmt -> return [Exp];

BreakStmt

ContinueStmt

6. declaration

VariableDecl -> const BasicType VariableDeclarator {,VariableDeclarator} | [static] BasicType VariableDeclarator {,VariableDeclarator} 

VariableDeclarator -> Identifier ['['Exp']'] [= Initial]

Initial -> Exp | '{' Exp {, Exp} '}'

FunctionDecl -> BasicType Identifier '(' [FormatParams] ')' BlockStmt

FormatParams -> FormatParam {,FormatParam}

FormatParam -> BasicType Identifier ['['']']

### Type

```cpp
class Type : public std::enable_shared_from_this<Type> {
public:
    enum TypeID {
        VoidTyID,
        IntegerTyID,
        FunctionTyID,
        CharTyID,
        ArrayTyID,
    };
private:
    TypeID typeIdentifier;
    TypeQualifier typeQualifier;
public:
    ...
}
```

TypeQualifier : NONE, CONST

### Symbol

```cpp
class Symbol {
public:
    enum SymbolKind {
        VARIABLE,
        FUNCTION,
    };

private:
    std::string name;
    StorageClass storageClass;
    TypePtr type;
    SymbolKind kind;
    ValuePtr value; // For IR generation

    bool isGlobal;
public:
    ...
}
```

StorageClass : NONE, STATIC

### visit

```cpp
void visitStmt(Stmt* stmt);

TypePtr visitExp(Exp* exp, int line);
```

**通过visitExp进行error处理，以及返回当前表达式返回的type**
