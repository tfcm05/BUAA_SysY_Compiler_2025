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

Exp -> CommaExp

BinaryExp -> BinaryExp BinaryOp UnaryExp | UnaryExp

**AssignExp, CommaExp视作一种特殊的BinaryExp**

UnaryExp -> UnaryOp UnaryExp | PostfixExp

PostfixExp -> PostfixExp '[' Exp ']' | PostfixExp '('[ Arguments ]')' | PrimaryExp

Arguments -> Exp { ',' Exp}

PrimaryExp -> LiteralExp | IdentifierExp | '(' Exp ')'

IdentifierExp -> Identifier

LiteralExp -> Number | String

### Stmt

#### 分类

Stmt: BlockStmt, ExpStmt, ForStmt, IfStmt,  ContinueStmt, BreakStmt, ReturnStmt

#### 文法

Stmt -> BlockStmt | ExpStmt | ... | ReturnStmt

1. CompoundStmt

BlockStmt -> '{' { BlockItem } '}'
BlockItem -> Stmt | Decl

2. ExpressionStmt

ExpStmt -> Exp;

3. IterationExp

ForStmt -> 'for' '(' [ Exp ] ';' [ Exp ] ';' [ Exp ] ')' Stmt

4. SelectionStmt

IfStmt -> 'if' '(' [ Exp ] ')' Stmt [ 'else' Stmt ]

5. JumpStmt

ReturnStmt -> return [ Exp ];

BreakStmt

ContinueStmt

### Decl

#### 分类

Decl: VariableDecl, FunctionDecl, ParameterDecl

#### 文法

Decl -> VariableDecl | FunctionDecl

VariableDecl -> DeclSpec Declarator { ',' Declarator } ';'

FunctionDecl -> DeclSpec Declarator BlockStmt

ParameterDecl -> DeclSpec [ Declarator ]

**MainFunctionDecl视作一种特殊的FunctionDecl**

#### Declarator

##### 分类

Declarator: DirectDeclarator, InitialDeclaraor

DirectDecalrator: IdentifierDeclarator, FunctionDeclarator, ArrayDeclarator

##### 文法

Declarator -> DirectDeclarator | InitialDeclaraor

DirectDeclarator -> IdentifierDeclarator | FunctionDeclarator | ArrayDeclarator

IdentifierDeclarator -> Identifier

FunctionDeclarator -> Declarator '(' [ ParameterDecls ] ')'

ParamterDecls -> ParameterDecl { ',' ParameterDecl }

ArrayDeclarator -> Declarator '[' [ Exp ] ']' {, '[' Exp ']'}

InitialDeclarator -> Declarator = Initializer

**ArrayDeclarator的Exp为ConstExp**

#### Initializer

##### 分类

Initializer: ExpInitializer, ListInitializer

##### 文法

Initializer -> ExpInitializer | ListInitializer

ExpInitializer -> Exp

ListInitializer -> '{' [ Initializers ] '}'

Initializers -> Initializer { ',' Initializer }

**若为全局变量，ExpInitializer中的Exp为ConstExp**

### Factory

通过工厂模式，实现类型的唯一化

*以Clang的Type为例*

```cpp
class Type {
public:
    enum TypeID {
        VoidTyID,
        CharTyID, // i8
        IntegerTyID, // i32

        ArrayTyID,
        PointerTyID,
        FunctionTyID,
    };
private:
    TypeID typeIdentifier;
    unsigned int qualifier;
    ClangContext& context;
protected:
    // 明确通过构造函数创建实例
    explicit Type(ClangContext& context, TypeID id);
public:
    // 禁用拷贝构造和赋值操作
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    TypeID getTypeID() const;
    ClangContext& getContext() const;

    virtual ~Type() = default;

    // 基本类型
    bool isVoidTy() const;
    bool isCharTy() const;
    bool isIntegerTy() const;

    // 派生类型
    bool isArrayTy() const;
    bool isPointerTy() const;
    bool isFunctionTy() const;

    // 限定类型
    unsigned int getQualifier() const;
    void setQualifier(unsigned int tq);
    void addQualifier(unsigned int tq);
    bool isConst() const;
};
```

### Visitor

通过访问者模式，实现语义分析与中间代码生成两次不同的语法树访问

```cpp
class Info;
using InfoPtr = std::shared_ptr<Info>;

class Info {
public:
    virtual ~Info() = default;
};

class Context;
using ContextPtr = std::shared_ptr<Context>;

class Context {
public:
    virtual ~Context() = default;
    // ...
};

class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    // Expressions
    virtual InfoPtr visitLiteralExp(LiteralExp* node, ContextPtr context) = 0;
    virtual InfoPtr visitIdentifierExp(IdentifierExp* node, ContextPtr context) = 0;
    
    //...
}
```
通过虚拟类Info和Context，统一两次访问所需不同上下文的传递

```cpp
class AstNode {
private:
    // ...
public:
    // ...

    virtual InfoPtr accept(AstVisitor& visitor, ContextPtr context) = 0;
};
```
语法树节点通过accept函数接受不同访问者的访问

#### 语义分析

```cpp
class SemanticInfo : public Info {
public:
    enum class InfoKind {
        DEFAULT,            // 默认
        CONST_EXP_INFO,   // 常量表达式信息
    };
private:
    InfoKind kind;

    // 声明的declType和表达式的expType
    Clang::Type* type;;

    // 表达式
    bool lValueFlag;
        // 常量表达式的常量值
        int64_t constValue;

    // 声明
    std::string name;
public:
    SemanticInfo(InfoKind k = InfoKind::DEFAULT);

    InfoKind getInfoKind() const;
    void setInfoKind(InfoKind k);

    Clang::Type* getType() const;
    void setType(Clang::Type* t);

    bool isLValue() const;
    void setLValueFlag(bool lvf);
    int64_t getConstValue() const;
    void setConstValue(int64_t cv);

    std::string getName() const;
    void setName(const std::string& n);
};

class SemanticContext : public Context {
public:
    enum class VisitPurpose {
        DEFAULT,           // 默认

        // 声明相关
        GENERATE_SYMBOL,   // 生成符号

        // 表达式相关
        CHECK_LVALUE,      // 检查表达式是否为可赋值的左值
        EVALUATE_CONST,    // 检查表达式是否为编译期常量，并求其值

        // 语句相关
        // 块语句
        RESOLVE_FUNCTION_BODY_ONLY // 只解析函数体，不创建新作用域
    };
private:
    VisitPurpose purpose;

    // 函数的returnType和变量的varType
    Clang::Type* type;

    bool loopFlag;
public:
    SemanticContext(VisitPurpose vp = VisitPurpose::DEFAULT);
    ContextPtr clone() override;

    VisitPurpose getVisitPurpose() const;
    void setVisitPurpose(VisitPurpose vp);

    Clang::Type* getType() const;
    void setType(Clang::Type* t);

    bool inLoop() const;
    void setLoopFlag(bool lf);
};
```
