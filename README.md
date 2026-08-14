---

# BUAA_SysY_Compiler_2025

## 总体架构与编译流程

整体采用「前端 - 中端 - IR 生成」三阶段流水线，并通过访问者模式在同一棵语法树上完成语义分析与中间代码生成：

1. 词法分析（LexicalAnalysis）  
    从 `testfile.txt` 读取源程序，拆分为 Token 流，供语法分析使用。
2. 语法分析（Parser）  
    使用递归下降分析（参见 `Parser::parse*` 系列函数）构造 AST，根节点类型为 `CompUnit`，同时将结构化的 AST 文本输出到 `parser.txt` 便于调试。
3. 语义分析（SemanticVisitor）  
    在 AST 上进行一次遍历：
    - 基于 `SymbolTable` 维护多层作用域与内置符号（如 `printf`、`getint`），并在 `symbol.txt` 中输出各作用域内的符号和类型信息；
    - 使用 `ClangContext` 与 `Clang::Type` 完成类型唯一化（Factory 模式），进行类型检查、左值检查、常量表达式求值等；
    - 所有语义错误通过 `errorLog` 记录，最终由 `dumpErrorsToFile` 统一写入 `error.txt`。
4. 中间代码生成（IRVisitor + IRBuilder）  
    第二次遍历 AST：
    - `IRVisitor` 将语义阶段得到的类型信息映射为 `LLVM::Type`，并使用 `IRBuilder` 在 `Module` 上构建函数、基本块以及指令；
    - `LLVMContext` 负责 IR 类型的唯一化与缓存（类似前述 `ClangContext`）。
5. IR 打印（IRPrinter）  
    由 `IRPrinter::printModule` 将 `Module` 中收集的函数、全局变量、字符串常量等打印为近似 LLVM IR 的文本格式，输出到 `llvm_ir.txt`，包括：
    - 运行时库函数声明（`getint`、`putint`、`putstr`、`putchar` 等）；
    - 全局变量和全局常量字符串；
    - 普通函数及 `main` 函数定义。

主程序入口参见 `Compiler.cpp`：

1. 构造 `LexicalAnalysis` 与 `Parser` 完成前端分析；
2. 构造 `ClangContext` 与 `SemanticVisitor` 完成语义检查与符号表生成；
3. 构造 `LLVMContext`、`Module`、`IRBuilder` 与 `IRVisitor` 完成 IR 构建与打印。

### 词法分析（LexicalAnalysis）

词法分析器位于 `LexicalAnalysis`，输入为 `testfile.txt` 的字符流（`std::istream&`），输出为带行号的 `Token` 序列：

- `Token`：由 `TokenType`、词面值 `value` 和出现行号 `line` 组成，`TokenType` 覆盖关键字、运算符、分隔符以及 `IDENFR`、`INTCON`、`STRCON` 等基本记号；
- 关键字与单字符运算符/分隔符通过两个 `std::unordered_map`（`keywords` 与 `singleCharOps`）统一管理，便于扩展；
- 词法状态机以 `currentChar` 为当前字符，配合 `advance()` / `peek()` 前瞻：
    - `skipWhiteSpace()` 负责跳过空白符；
    - `skipComment()` 识别并跳过 `//` 单行注释与 `/* ... */` 多行注释；
    - `number()` 解析十进制整数字面量；
    - `identifier()` 解析标识符/关键字（先扫描标识符，再在 `keywords` 中判定是否为关键字）；
    - `string()` 处理双引号括起的字符串常量。
- 复杂运算符通过“读一个字符 + 查看下一个字符”的方式识别：
    - 关系与相等运算符：`>=`、`<=`、`==`、`!=`；
    - 逻辑与/或：`&&`、`||`，若写成单个 `&` / `|` 会在词法阶段上报错误 `ErrorCode::a`；
- `getNextTokenInternal()` 负责真正的扫描逻辑，`getNextToken()` / `peekNextToken()` / `peekSecondToken()` 则在其基础上提供 1~2 记号的前瞻缓冲；

词法分析保持实现简单直接，保证行号精确与错误位置友好，便于后续语法/语义阶段的报错定位。

### 文法

#### Exp

##### 分类
BinaryExp:CommaExp, AddExp, MulExp, OrExp, AndExp, RelExp

AssignExp

UnaryExp: PlusExp, MinusExp, NotExp

PostfixExp: SubscriptExp, FunctionCallExp

PrimaryExp: LiteralExp, IdentifierExp

##### 文法

Exp -> CommaExp

BinaryExp -> BinaryExp BinaryOp UnaryExp | UnaryExp

**AssignExp, CommaExp视作一种特殊的BinaryExp**

UnaryExp -> UnaryOp UnaryExp | PostfixExp

PostfixExp -> PostfixExp '[' Exp ']' | PostfixExp '('[ Arguments ]')' | PrimaryExp

Arguments -> Exp { ',' Exp}

PrimaryExp -> LiteralExp | IdentifierExp | '(' Exp ')'

IdentifierExp -> Identifier

LiteralExp -> Number | String

#### Stmt

##### 分类

Stmt: BlockStmt, ExpStmt, ForStmt, IfStmt,  ContinueStmt, BreakStmt, ReturnStmt

##### 文法

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

#### Decl

##### 分类

Decl: VariableDecl, FunctionDecl, ParameterDecl

##### 文法

Decl -> VariableDecl | FunctionDecl

VariableDecl -> DeclSpec Declarator { ',' Declarator } ';'

FunctionDecl -> DeclSpec Declarator BlockStmt

ParameterDecl -> DeclSpec [ Declarator ]

**MainFunctionDecl视作一种特殊的FunctionDecl**

##### Declarator

###### 分类

Declarator: DirectDeclarator, InitialDeclaraor

DirectDecalrator: IdentifierDeclarator, FunctionDeclarator, ArrayDeclarator

###### 文法

Declarator -> DirectDeclarator | InitialDeclaraor

DirectDeclarator -> IdentifierDeclarator | FunctionDeclarator | ArrayDeclarator

IdentifierDeclarator -> Identifier

FunctionDeclarator -> Declarator '(' [ ParameterDecls ] ')'

ParamterDecls -> ParameterDecl { ',' ParameterDecl }

ArrayDeclarator -> Declarator '[' [ Exp ] ']' {, '[' Exp ']'}

InitialDeclarator -> Declarator = Initializer

**ArrayDeclarator的Exp为ConstExp**

##### Initializer

###### 分类

Initializer: ExpInitializer, ListInitializer

###### 文法

Initializer -> ExpInitializer | ListInitializer

ExpInitializer -> Exp

ListInitializer -> '{' [ Initializers ] '}'

Initializers -> Initializer { ',' Initializer }

**若为全局变量、static变量，ExpInitializer中的Exp为ConstExp**

### 语法分析（Parser）

语法分析使用手写递归下降的 LL(1) 分析器 `Parser`，直接在 `Token` 流上构造 AST：

- 记号管理：
    - 当前记号通过 `currentToken` 保存；
    - `advance()` 从词法分析器中读取下一个记号，并维护上一记号所在行号 `preLine`；
    - `peekNextRef()` / `peekSecondRef()` 直接复用 `LexicalAnalysis` 的前瞻接口；
    - 通过宏 `CURRENT_TYPE` / `NEXT_TYPE` / `SECOND_TYPE` 简化判断。
- 匹配与错误恢复：
    - `match(TokenType)` / `match(TokenType,int mode)` 实现 0/1/2 记号前瞻判断，用于区分声明与表达式等二义性情况；
    - `expect(TokenType)` 在匹配失败时打印期望/实际记号信息，并根据缺失的 `;`、`)`、`]` 分别记录 `ErrorCode::i/j/k`，为后续阶段提供尽量完整的 AST。
- 顶层结构：
    - `parseCompUnit()` 对应编译单元：先解析所有全局变量声明，再解析普通函数声明，最后解析 `main` 函数；
    - 全局与局部变量均复用 `VariableDecl` 结点，只是后续语义阶段根据作用域做区分。
- 声明与类型：
    - `parseDeclSpec()` 负责解析 `static` / `const` / `int` / `void` 等前缀，落到 `DeclSpec` 上的 `StorageClass`、`TypeQualifier` 与 `TypeSpecifier`；
    - `parseDeclarator()` 根据后继的 `[` / `(` 区分标识符、数组（一维）与函数声明，并在遇到 `=` 时转为 `InitialDeclarator`；
    - `parseInitializer()` 解析表达式初始化（`ExpInitializer`）与花括号列表初始化（`ListInitializer`）。
- 语句与表达式：
    - 语句：`parseBlockStmt()`、`parseIfStmt()`、`parseForStmt()`、`parseReturnStmt()`、`parseBreakStmt()`、`parseContinueStmt()` 与 `parseExpStmt()` 覆盖文法中所有语句形式；
    - 表达式：从 `parseCommaExp()` 底向上，经 `AssignExp`、逻辑或/与、相等、关系、加减、乘除取模，一直到一元表达式和后缀/基本表达式，分别落到 `BinaryExp`、`AssignExp`、`UnaryExp`、`SubscriptExp`、`FunctionCallExp` 等 AST 结点；
    - `parsePrimaryExp()` 负责括号包裹的子表达式、字面量以及简单标识符。
- 调试支持：
    - `parserLog(const AstNode* root)` 以文本形式深度优先遍历整棵 AST，带缩进地打印声明、语句与表达式层次结构到 `parser.txt`，方便对照源程序进行调试和验证语法树结构。

目前语法分析严格遵循前文“文法”一节的设计，尽量保持节点类型和产生式一一对应，方便后续语义分析和 IR 生成阶段在同一棵 AST 上复用结构信息。

### 核心模式

#### Visitor模式

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

#### Factory模式

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
    ...

    // 基本类型
    ...

    // 派生类型
    ...

    // 限定类型
    ...
};
```
只允许ClangContextImpl创建实例，使用智能指针存储对象，以保证对象在需要时仍然活跃，传递指针也相对简单

### 语义分析

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
        // 常量表达式/常量初始值
        Clang::ConstValue* constValue;

    // 声明
    std::string name;
public:
    ...
};
```

```cpp
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
    ...
};
```

**context传递上文，子节点一般可以共用**

**info传回下文，一般无法共用**

除此之外，还有：

- `SymbolTable`：基于父子指针维护静态作用域链，提供 `pushScope` / `popScope`、`lookup` 等接口，支持在当前及外层作用域查找符号；
- `Symbol`：记录标识符的存储类（`StorageClass`）、`Clang::Type*` 以及常量初始值等信息；
- `SemanticVisitor`：继承自 `AstVisitor`，实现所有 `visit*` 接口，在不同 `SemanticContext::VisitPurpose` 下完成符号生成、常量表达式求值、左值检查等，并将信息缓存到 `SemanticInfo` 中。

#### 类型系统（Clang::Type 与 ClangContext）

语义阶段的类型系统抽象为一小套不可变的 `Clang::Type` 层次结构，由 `ClangContext` 统一创建和管理，用工厂模式保证**同构类型唯一**：

- 基本类型：
    - `VoidType`：`getVoidTyImpl()`；
    - `IntegerType`（i32）：`getIntegerTyImpl()`；
    - `CharType`（i8）：`getCharTyImpl()`。
- 派生类型：
    - `PointerType`：`getPointerTyImpl(Clang::Type* pointed)`，用于表示 `T*`；
    - `ArrayType`：`getArrayTyImpl(Clang::Type* elem, size_t size)`，用于表示编译期已知长度的一维/多维数组；
    - `FunctionType`：`getFunctionTyImpl(Clang::Type* ret, const std::vector<Clang::Type*>& params, bool isVarArg)`，可表示可变参数函数（如 `printf`）。
- 限定符：
    - 通过 `getQualifiedTyImpl(baseType, qualifier)` 将 `const` 信息叠加在基础类型之上；
    - `Type::isConst()`、`getQualifier()`/`addQualifier()` 用于在语义阶段区分可变与只读对象（比如阻止对 `const` 全局的赋值）。

常量值统一抽象为 `Clang::ConstValue`：

- `ConstInteger`：保存编译期整数常量；
- `ConstArray`：保存编译期数组常量（元素是 `ConstValue*`），支持嵌套数组；
- 由 `ClangContext` 提供 `getConstIntegerImpl` / `getConstArrayImpl` 创建，便于在常量折叠和全局初始化期间复用。

`SemanticVisitor` 在 `EVALUATE_CONST` 模式下，会递归计算表达式的 `ConstValue`，并存入 `SemanticInfo::constValue`，其中：

- 二元算术/比较/逻辑运算统一在 `visitBinaryExp` 中根据 `BinaryExp::BinaryOp` 进行常量折叠；
- 结果使用 `ClangContext::getConstIntegerImpl` 重新封装为 `ConstInteger`，保证常量池唯一性。

### 中间代码生成

中间代码生成同样基于访问者框架，核心类为 `IRInfo`、`IRContext` 与 `IRVisitor`：

```cpp
class IRInfo : public Info {
private:
    Value* value; // 对应表达式或左值在 IR 中的 Value
public:
    IRInfo(Value* val);

    Value* getValue() const;
    void setValue(Value* val);
};
```

```cpp
class IRContext : public Context {
public:
    enum visitPurpose {
        DEFAULT,
        CREATE_GLOBAL_VARIABLE, // 全局变量初始化
        USE_LOCATION_VALUE,     // 使用左值地址而非右值
    };
private:
    visitPurpose purpose;
    Value* values[2];              // 用于传递临时 SSA 值/地址
    LLVM::Type* type;              // 初始化或转换时需要的目标类型
    std::vector<size_t> indices;   // GEP 用到的下标序列
public:
    ...
};
```

`IRVisitor` 负责：

- 将 `Clang::Type*` 通过内部的 `typeCache` 映射为 `LLVM::Type*`；
- 利用 `IRBuilder` 统一构造 `Function`、`BasicBlock` 及各种 `Instruction`（包括 `alloca`/`load`/`store`、算术逻辑指令、`icmp`、`br`、`ret`、`phi`、`call`、`getelementptr` 等）；
- 在 `Module` 中登记所有函数、全局变量与字符串常量，并记录 `main` 函数指针，供 `IRPrinter` 使用。

#### 数组与函数调用

AST 层面：

- 声明：
    - `ArrayDeclarator` 表示 `T a[N][M]` 一类的数组声明，其下挂若干维度的 `ConstExp`；
    - `FuncDeclarator` 表示函数声明，记录参数列表（`ParameterDecl`）和返回类型。
- 表达式：
    - `SubscriptExp` 表示数组访问 `a[i][j]`，持有一个“数组表达式”和一组下标表达式；
    - `FunctionCallExp` 持有“被调函数表达式”和实参列表 `arguments`。

语义阶段：

- 数组：
    - 声明时，`visitArrayDeclarator` 逐维检查维度是否为 `ConstExp`，并在 `ClangContext` 中构造嵌套 `ArrayType`；
    - 访问时，`visitSubscriptExp` 校验被下标对象是否为数组或指针，并根据下标个数推导访问后的元素类型；
    - 对于数组元素赋值，会同时检查左值属性与下标范围是否为整数表达式。
- 函数调用：
    - `visitFunctionCallExp` 首先通过 `IdentifierExp` 在 `SymbolTable` 中查到对应 `Symbol`，其类型为 `FunctionType`；
    - 校验**实参与形参个数匹配**、**类型可赋值**，若为可变参数函数（例如 `printf`），则仅对固定参数部分做严格检查；
    - 在返回的 `SemanticInfo` 中记录返回值类型，用于后续表达式上下文类型检查（例如赋值、条件判断等）。

IR 生成阶段：

- 数组：
    - 全局数组：在 `CREATE_GLOBAL_VARIABLE` 模式下，`visitVariableDecl` / `visitListInitializer` 会构造 `LLVM::ArrayType` 和对应的 `ConstArray`，通过 `IRBuilder::createConstArray` 与 `createGlobalVariable` 写入 `Module`；
    - 局部数组：通过 `IRBuilder::createAlloca` 分配数组对象，再用 `getelementptr`（`createGEP`）根据下标计算元素地址；
    - 多维数组：在 `IRContext::indices` 中累积每一维的下标，统一构造带多个 index 的 GEP。
- 函数调用：
    - 函数声明与定义：
        - 依赖 `convertType(Clang::Type*)` 将 `FunctionType` 映射为 `LLVM::FunctionType`；
        - 使用 `IRBuilder::createFunction` 创建 `Function`，并在 `Module::addFunction` 中登记；
    - 调用点：在 `visitFunctionCallExp` 中：
        - 从符号表/全局 Map 中取到对应 `Function*`；
        - 将每个实参表达式生成 `Value*` 后，调用 `IRBuilder::createCall` 生成 `CallInstr`；
        - 若返回类型为 `void`，忽略返回值；否则把 `CallInstr` 包装到 `IRInfo`，供上层表达式继续使用。

#### 短路求值（逻辑与/或）

**IR 生成**阶段：

- IR 生成阶段：
    - 在 `IRVisitor::visitBinaryExp` 中，对 `AND` / `OR` 特殊处理：
        - 先把左操作数通过一个本地 `ensureBoolean` lambda 统一转为 i1：
            - 若本来就是 i1，直接使用；
            - 若是更宽的整数，插入一条 `icmp ne` 与 0 的比较得到 i1。
        - `AND`：
            1. 记录当前基本块 `currentBlock`，新建 `rhsBlock` 与 `mergeBlock`；
            2. 在 `currentBlock` 根据 `lhsBool` 生成 `br i1 lhsBool, label %rhsBlock, label %mergeBlock`；
            3. 在 `rhsBlock` 计算右操作数 `rhsBool`，然后无条件跳转到 `mergeBlock`；
            4. 在 `mergeBlock` 构造 `phi i1 [ false, %currentBlock ], [ rhsBool, %rhsEndBlock ]`，作为整个 AND 表达式的值。
        - `OR`：
            1. 同样记录 `currentBlock`，新建 `rhsBlock` 与 `mergeBlock`；
            2. 在 `currentBlock` 根据 `lhsBool` 生成 `br i1 lhsBool, label %mergeBlock, label %rhsBlock`；
            3. 在 `rhsBlock` 计算 `rhsBool` 后跳转 `mergeBlock`；
            4. 在 `mergeBlock` 构造 `phi i1 [ true, %currentBlock ], [ rhsBool, %rhsEndBlock ]`。
    - 这种编码方式保证：
        - 当 `lhs` 已经确定结果时，不再对 `rhs` 求值，实现真正的短路；
        - 整个表达式产生一个 i1 值，可直接用于条件跳转（如 `if` 和 `for`）。


---

```
准备生成目录
mkdir build
cd build
生成构建文件（用 MinGW）
cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="D:/c_byq/mingw64/bin/g++.exe" -DCMAKE_BUILD_TYPE=Release ..
编译
cmake --build . --config Release
运行
可执行文件会在 build/Compiler.exe（或生成目录下），直接运行：
./Compiler.exe
如果想切换 Debug，把步骤 2 的 -DCMAKE_BUILD_TYPE=Release 改成 Debug。
```

### 一些想法

本来是直接把这个编译器当C语言的编译器来设计的，因为本学期上了一门课程，叫做“C语言系统级编程”。`
这门课程扫清了我对C语言的很多误区，我干脆就照着课上讲的内容，参考了一些现有编译器的设计，设计了编译器的语法树节点、符号和符号表等内容，再逐步扩充与实现代码。

问题也随之而来，实现逻辑变得非常复杂：原有文法只支持一维数组及一维初始化，而我根据C语言文法，设计的是多维数组及嵌套初始化，递归访问的逻辑已经很复杂了，更何况还包含了左值右值、标识符嵌套等内容，使得访问者模式的上下文传递，即属性文法的分析变得极其困难。
而且，由于我将语义分析阶段和中间代码生成阶段分开，虽然解耦使得两者的代码功能相对清晰，但是也使得传递的信息需要更多的设计，比如常量值的传递，比如functionCall预先声明的函数（因为Symbol存储在声明语句中）。
（*printf函数调用的存在也破坏了代码的协调性*）

除此之外，原有文法虽然文法上是C语言的子集，但语义上却有所区别，一些概念也与C语言的不同，比如const变量被视作常量表达式，必须由常量表达式初始化。
因此，const、global、static其实是一样的，都是在编译期确定初始值，不过const有更多的语义限制。实际上，C语言中const属于typeQualifier，static属于storageClass。可惜我到最后才发现这个问题，之前一直把原有语言当作C语言的子集...


还有表达式常量折叠、llvm常量唯一化等想要实现的功能没有功能，也算是一个遗憾了，心有余而力不足呀，没想到最后连优化的屁股都没摸到，看来还是不能勉强自己做一些自己做不到的事情，量力而行吧！

**迭代过程**
https://github.com/tfcm05/BUAA_SysY_Compiler_2025
