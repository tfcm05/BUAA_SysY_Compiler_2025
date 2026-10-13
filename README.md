# BUAA_SysY_Compiler_2025

一个面向 C 语言子集的编译器，将源代码编译为 LLVM IR 文本（可链接运行）和 MIPS 汇编。

## 编译与运行

```bash
# 编译编译器 (推荐 CMake; Debug=-O0 快增量, Release=-O2)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
# Release: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
# 若已安装 ccache, CMake 会自动设为 CMAKE_CXX_COMPILER_LAUNCHER
# 或 (单文件链接, 开发备用): mkdir -p build && g++ -std=c++17 -O0 -Iinclude -o build/Compiler Compiler.cpp $(find src -name "*.cpp")

# 编译源程序
./build/Compiler [源文件]   # 默认 testfile.txt
# → parser.txt / symbol.txt / error.txt / llvm_ir.txt / mips.s

# 链接运行 (LLVM IR 路线)
clang -w -x ir llvm_ir.txt -x none tests/runtime.c -o program && ./program

# 批量测试
./tests/run.sh
```

## 总体架构

```
testfile.txt
    │
    ▼
┌──────────────┐
│  词法分析     │  LexicalAnalysis
│  Lexer        │  Token 流 (关键字/运算符/字面量, 含行号)
└──────┬───────┘
       │ Token
       ▼
┌──────────────┐
│  语法分析     │  Parser (递归下降 LL(1))
│  Parser       │  → AST (CompUnit 根节点, 声明按源码顺序存放)
└──────┬───────┘
       │ AST
       ▼
┌──────────────┐
│  语义分析     │  SemanticVisitor → AnalysisResult
│  Semantic     │  单遍扫描; Symbol/类型/常量写入注解表
└──────┬───────┘
       │ AST + AnalysisResult
       ▼
┌──────────────┐
│  IR 生成      │  IRVisitor + IRBuilder + IRBindings
│  IR Gen       │  → Module (Symbol→Value 会话映射)
└──────┬───────┘
       │ Module
       ▼
┌──────────────┐
│  IR 优化      │  PassManager: AS → ConstFold → ConstBr →
│  Optimization│  DeadBlock → DeadInstr → Module (优化后)
└──────┬───────┘
       │
       ├──────────────────────────────┐
       ▼                              ▼
┌──────────────┐            ┌──────────────────┐
│  IR 打印      │            │  MIPS 后端        │
│  IRPrinter   │            │  MIPSCodeGenerator│
│  → llvm_ir.txt│            │  (Phi消除/寄存器  │
└──────────────┘            │   缓存/GEP偏移)   │
                            │  → MIPSPeepholeOptimizer
                            │  → MIPSPrinter → mips.s
                            └──────────────────┘
```

`Compiler.cpp` 是入口，依次调用上述各阶段。语义错误时 `error.txt` 输出到文件并跳过 IR/MIPS 生成。

## 目录结构

```
src/
├── FrontEnd/
│   ├── LexicalAnalysis/      # 词法分析器 (LexicalAnalysis.cpp, Token.cpp)
│   └── SyntaxAnalysis/       # 语法分析器 (递归下降)
│       ├── SyntaxAnalysis.cpp # Parser 实现 (parseCompUnit/parseVarDecl/parseFuncDecl/...)
│       └── AstNode/          # AST 节点定义与 accept 实现
│           ├── AstNode.cpp   # 基类 + CompUnit (声明按源码顺序存于 AstNodePtr 数组)
│           ├── Declaration.cpp
│           ├── Expression.cpp
│           └── Statement.cpp
├── MiddleEnd/
│   ├── SemanticAnalysis/     # 语义分析 (单遍扫描, 先声明后使用)
│   │   ├── SemanticVisitor.cpp  # 核心: Info/Context/符号辅助
│   │   ├── SemanticDecl.cpp     # 声明/Declarator/Initializer
│   │   ├── SemanticExp.cpp      # 表达式
│   │   ├── SemanticStmt.cpp     # 语句
│   │   ├── ConstEval.cpp        # 编译期常量折叠
│   │   ├── ClangContext.cpp   # 类型工厂
│   │   ├── Type.cpp
│   │   ├── Symbol.cpp         # 符号 (name/type/kind/constValue; 无 Value*)
│   │   ├── AnalysisResult.h   # 语义注解表 (Symbol/type/const)
│   │   ├── ConstValue.cpp
│   │   ├── ScopeGuard.h / DeclaratorTypeBuilder.h
│   │   └── SymbolTable.cpp    # 作用域与符号表
│   ├── IntermediateCodeGenerate/ # IR 生成
│   │   ├── IRVisitor.cpp      # 核心: Info/Context/convertType
│   │   ├── IRDecl.cpp / IRExp.cpp / IRStmt.cpp
│   │   ├── IRBindings.h       # 会话 Symbol*→Value*
│   │   ├── EnsureI1.h / ArrayInitIR.h  # IR 辅助
│   │   ├── IRBuilder.cpp      # IR 指令构造器
│   │   ├── IRPrinter.cpp      # IR 文本输出 (声明/定义区分)
│   │   ├── Module.cpp
│   │   ├── Type.cpp
│   │   ├── LLVMContext.cpp
│   │   └── Value/             # LLVM Value 体系
│   │       ├── Value.cpp
│   │       └── User/
│   │           ├── User.cpp
│   │           ├── Constant/  # ConstInt/ConstArray/ConstString/GlobalValue
│   │           └── Instruction/ # 算术逻辑/控制流/内存访问/函数调用
│   └── Optimization/         # IR 优化 Pass
│       ├── Pass.h / PassManager.h          # Module 级 Pass 接口与编排
│       ├── IRReplaceUses.cpp               # 安全操作数替换 (扫描式 use-def)
│       ├── AlgebraicSimplification.cpp     # 代数化简 (已接入)
│       ├── IRConstantFolding.cpp           # 常量折叠 (已接入)
│       ├── ConstantBranchFolding.cpp       # 常量分支化简 (已接入)
│       ├── DeadBlockElimination.cpp        # 死基本块消除 (已接入; Phi 块合并已门控)
│       └── DeadInstructionElimination.cpp  # 死指令消除 (已接入)
├── BackEnd/
│   └── MIPS/                 # MIPS 后端
│       ├── MIPSCodeGenerator.cpp   # 代码生成 (Phi消除/寄存器缓存/栈帧)
│       ├── MIPSPeepholeOptimizer.cpp # 窥孔优化 (立即数/冗余load/跳转/强度归约)
│       └── MIPS.h / MIPSPrinter   # 汇编输出
├── Error.cpp                 # 错误码与错误输出
└── ...
include/
├── FrontEnd/                  # 与 src 同构的头文件
├── MiddleEnd/
│   ├── SemanticAnalysis/
│   ├── IntermediateCodeGenerate/
│   │   ├── IRBuilder.h
│   │   ├── IRPrinter.h
│   │   ├── Module.h
│   │   ├── LLVMContext.h
│   │   └── LLVM/              # IR 类型与 Value 层次 (Type.h, Value/, BasicBlock.h, Argument.h)
│   └── Optimization/          # 5 个优化 Pass 头文件
├── BackEnd/
│   └── MIPS/                  # MIPSCodeGenerator.h / MIPSPeepholeOptimizer.h / MIPS.h
├── Error.h                   # ErrorCode 枚举 (按编译阶段分类)
└── ...
tests/
├── run.sh                    # 测试脚本
├── runtime.c                 # C 运行时库 (getint/putint)
└── cases/                    # 28 个测试用例 (*.sy + *.out/*.err)
Compiler.cpp                  # 编译器入口
```

## 设计模式

### Visitor 模式

语义分析和 IR 生成两次遍历 AST，通过 `AstVisitor` 虚接口实现：

```cpp
// 每个节点实现 accept, 转发到 visitor 的对应 visit 方法
InfoPtr AstNode::accept(AstVisitor& visitor, ContextPtr context);

// visitor 为每种节点提供 visit 重载
class AstVisitor {
    virtual InfoPtr visitBinaryExp(BinaryExp*, ContextPtr) = 0;
    virtual InfoPtr visitIfStmt(IfStmt*, ContextPtr) = 0;
    // ...
};

// Context 传递上文 (函数返回类型/变量类型/求值模式等), 子节点可共用
// Info 传回下文 (类型/常量值/IR Value 等), 由父节点消费
```

两种具体实现：
- `SemanticVisitor` + `SemanticContext`/`SemanticInfo`：类型检查、作用域管理、常量折叠；结果写入 `AnalysisResult`
- `IRVisitor` + `IRContext`/`IRInfo`：读 `AnalysisResult`，经 `IRBindings` 映射到 `Value*`

`SemanticContext` / `IRContext` 使用显式 bool 字段（`wantConst` / `useLocation` 等），配合 `*ContextGuard` 就地改写再恢复，避免热路径上反复 `clone()`。`AstVisitor` 提供默认空 `visit*`，新增结点不必改所有 Visitor。

### Factory 模式

类型对象通过工厂唯一化，避免重复创建等价类型：

- `ClangContext` → `Clang::Type*`（语义阶段类型，如 `IntegerType`、`ArrayType`、`PointerType`、`FunctionType`）
- `LLVMContext` → `LLVM::Type*`（IR 阶段类型，如 `i32`、`[N x i32]`、`i8*`）
- 常量值也通过 `ClangContext` 唯一化：`getConstIntegerImpl(val)`、`getConstArrayImpl(elements)`

## 语言特性

### 支持的 C 子集

| 特性 | 说明 |
|------|------|
| 基本类型 | `int`、`char`、`void` |
| 存储类 | `static`（静态存储期）、默认（自动存储期） |
| 类型限定符 | `const`（只读限定，局部 const 允许非常量初始化） |
| 变量声明 | 多声明符 `int a, b, c;`、初始化器 `int x = 1;` |
| 数组 | 多维 `int a[2][3];`、嵌套初始化 `int m[2][2] = {{1,2},{3,4}};`、部分初始化补零、平铺 `{1,2,3,4}` |
| 指针 | `char *p;`、指针声明符 `*` |
| 函数 | 声明 `int f(int, int);`、定义 `int f(int a, int b) { ... }`、先声明后定义 |
| 变参函数 | `int printf(const char *, ...);` — 支持外部库函数调用 |
| 控制流 | `if/else`、`for`、`while`、`do-while`、`break`、`continue`、`return` |
| 表达式 | 算术/关系/逻辑运算、赋值、逗号、数组下标、函数调用 |
| 字符串 | 字符串字面量 `"hello\n"`（lexer 统一转义处理） |
| 注释 | `//` 单行、`/* */` 多行 |

### 语义规则（与 C 一致）

- **单遍扫描**：按源码顺序处理，先声明后使用（前向引用需先声明原型）
- **函数声明**：允许重复声明/原型+定义复用同一符号；重复定义由链接器报错
- **static 存储期**：`static` 变量全局存储，初始化器必须为常量表达式
- **const 限定符**：`const` 是只读限定符而非编译期常量；局部 `const` 可用非常量表达式初始化；全局/static `const` 初始化器必须常量
- **数组初始化**：嵌套 `{{1,2},{3,4}}` 与平铺 `{1,2,3,4}` 均可；缺省元素补零（全局/局部一致）
- **return 分析**：`alwaysReturns` 递归判定 if/else 双分支均返回、块内任意位置 return 等情况
- **常量折叠**：`wantConst` 模式下编译期计算常量表达式（算术/比较/逻辑）
- **短路求值**：`&&`/`||` 在 IR 生成时生成分支结构，左操作数确定结果时不求值右操作数
- **数组退化**：数组传参退化为指针，GEP 计算元素地址

### 错误码（按编译阶段分类）

```cpp
// 词法错误
IllegalChar                // 非法字符

// 语法错误
MissingSemicolon           // 缺少分号
MissingRightParen         // 缺少右小括号
MissingRightBracket       // 缺少右中括号

// 语义错误: 声明与作用域
RedefinedName             // 名字重定义
UndefinedName             // 未定义的名字

// 语义错误: 函数调用
ParamCountMismatch        // 函数参数个数不匹配
ParamTypeMismatch         // 函数参数类型不匹配
CallNonFunction           // 对非函数类型进行调用

// 语义错误: 返回语句
ReturnTypeMismatch        // return语句与函数返回类型不匹配
MissingReturn             // 有返回值的函数缺少return语句

// 语义错误: 常量与初始化
ConstAssignment           // 不能改变常量的值
GlobalStaticNonConstInit  // 全局或静态变量初始化必须为常量表达式
ConstDivByZero            // 常量表达式除以零

// 语义错误: 数组
ArraySizeNotConst         // 数组大小不是常量
ArraySizeNotPositive      // 数组大小必须为正
SubscriptNonArray         // 对非数组或非指针类型使用下标

// 语义错误: 赋值
AssignmentToNonLValue     // 赋值左侧不是左值

// 语义错误: 控制流
BreakContinueOutsideLoop  // 非循环块中使用break/continue
```

错误输出到 `error.txt`，格式：`行号 中文原因 '符号名'`

## IR 类型系统

LLVM IR 的类型层次：

| IR 类型 | 对应 C 类型 | 说明 |
|---------|-----------|------|
| `i32` | `int` | 32 位整数 |
| `i8` | `char` | 8 位字符 |
| `[N x i32]` | `int[N]` | 数组 |
| `i8*` | `char*` | 指针 |
| `i32 (i32, ...)*` | 函数指针 | 含可变参数 |

## 优化 Pass

IR 生成后、打印前执行优化，采用独立 Pass 设计：

| Pass | 文件 | 状态 | 作用 |
|------|------|------|------|
| 代数化简 | `AlgebraicSimplification.cpp` | ✅ 已接入 | `x + 0 → x`、`x * 1 → x`、`x * 0 → 0` 等 |
| IR 常量折叠 | `IRConstantFolding.cpp` | 📦 已实现未接入 | `add i32 1, 2` → `i32 3`、`icmp eq 1, 1` → `i1 1` |
| 死指令消除 | `DeadInstructionElimination.cpp` | 📦 已实现未接入 | 删除结果无引用且无副作用的指令 |
| 常量分支化简 | `ConstantBranchFolding.cpp` | 📦 已实现未接入 | `br i1 true, %A, %B` → `br label %A` |
| 死基本块消除 | `DeadBlockElimination.cpp` | 📦 已实现未接入 | 删除不可达基本块、合并无条件跳转链 |

设计执行顺序：代数化简 → 常量折叠 → 死指令消除 → 常量分支化简 → 死基本块消除（当前仅代数化简接入 Compiler.cpp）

## MIPS 后端

IR 生成后，经 MIPS 后端输出 `mips.s`（32 位 O32 调用约定）：

| 阶段 | 文件 | 说明 |
|------|------|------|
| 代码生成 | `MIPSCodeGenerator.cpp` | Phi 消除（前驱 terminator 前插入 copy 到 phi 栈槽）、基本块级寄存器缓存（T0-T7，耗尽自动溢出）、GEP 精确偏移、caller-saved 保护、立即数优化 |
| 窥孔优化 | `MIPSPeepholeOptimizer.cpp` | P1 立即数折叠、P2 冗余 lw/sw 消除、P3 跳转优化、P4 强度归约（mul/div 2^k）、P5 死指令消除、P6 move 消除，多 pass 迭代到 fixpoint |
| 输出 | `MIPS.h` (MIPSPrinter) | 生成 MIPS 汇编文本 |

函数调用约定：前 4 个参数经 `$a0`–`$a3` 传递，更多参数预留栈空间（`16 + 4*(n-4)` 字节）。

## 测试

28 个测试用例覆盖：

- 基础：`hello`、`scope`、`voidfunc`
- 控制：`for`、`while`、`dowhile`、`break`、`continue`、`logic`
- 数组：`arr2d`（嵌套 `{{1,2,3},{4,5,6}}`）、`arrparam`、`constarr`
- 常量：`constexpr`、`constlocal`、`staticzero`、`globalinit`
- 函数：`fib`、`getint`
- for 改进：`forinit`（变量共享）、`forshadow`（遮蔽）
- return 分析：`retif`（if/else 返回）、`retif2`（无 else）、`retmid`（中间返回）、`retnest`（嵌套）、`retvoid`（void 错误）
- 外部函数：`printf`（外部 printf 变参调用）
- 错误用例：`undefvar`（未定义名字）、`retvoid`（类型不匹配）

测试脚本 `tests/run.sh` 对每个用例：编译 → `error.txt` 匹配 `.err` 或 IR 链接运行 → 输出匹配 `.out`。

## 运行时库

`tests/runtime.c` 提供基本 IO：
```c
int getint();    // scanf("%d")
void putint(int); // printf("%d")
```

用户程序通过外部声明使用：
```c
void putint(int a);
int getint();
int printf(const char *, ...);
```

## 迭代过程

详见 [GitHub](https://github.com/tfcm05/BUAA_SysY_Compiler_2025)
