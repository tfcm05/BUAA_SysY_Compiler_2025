# BUAA_SysY_Compiler_2025

一个面向 C 语言子集的编译器，将源代码编译为 LLVM IR 文本（可链接运行）和 MIPS 汇编。当前主开发分支按 **mini-C 前端** 演进：保留 Declarator 形 AST、双类型系统（Clang:: / LLVM::）、`printf` 等作为普通库函数调用。

## 编译与运行

```bash
# 推荐 CMake（Debug=-O0 快增量；Release=-O2）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
# Release: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)
# 若已安装 ccache，CMake 会自动设为 CMAKE_CXX_COMPILER_LAUNCHER

# 备用（单命令链接）
mkdir -p build && g++ -std=c++17 -O0 -Iinclude -o build/Compiler \
  Compiler.cpp $(find src -name "*.cpp")

# 编译源程序（argv[1]；省略则默认 testfile.txt）
./build/Compiler [源文件]
# → parser.txt / symbol.txt / error.txt / llvm_ir.txt / mips.s

# 链接运行（LLVM IR 路线）
clang -w -x ir llvm_ir.txt -x none tests/runtime.c -o program && ./program

# 批量测试
./tests/run.sh
```

语义有错时写入 `error.txt` 并 **跳过** IR 优化与 MIPS；成功路径上 `llvm_ir.txt` 在 **全部 Pass 之后** 由驱动显式 `printModule`（不再依赖 `IRVisitor` 析构副作用）。

## 总体架构

```
源文件 (argv / testfile.txt)
    │
    ▼
┌──────────────┐
│  词法分析     │  LexicalAnalysis → Token 流
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  语法分析     │  Parser (递归下降) → AST（Declarator 形，声明按源码顺序）
└──────┬───────┘
       │ AST（纯语法，不挂 Symbol*）
       ▼
┌──────────────┐
│  语义分析     │  SemanticVisitor → AnalysisResult
│               │  符号 / 类型 / 常量注解外置；AST 保持语法形状
└──────┬───────┘
       │ AST + AnalysisResult
       ▼
┌──────────────┐
│  IR 生成      │  IRVisitor + IRBuilder + IRBindings
│               │  Symbol* → Value* 仅会话映射；Symbol 不持 Value*
└──────┬───────┘
       │ Module
       ▼
┌──────────────┐
│  IR 优化      │  PassManager（只读/改 Module，不再 traverse AST）
│               │  Mem2Reg → AlgebraicSimplification → IRConstantFolding
│               │  → ConstantBranchFolding → DeadBlockElimination
│               │  → DeadInstructionElimination
└──────┬───────┘
       │
       ├──────────────────────────────┐
       ▼                              ▼
┌──────────────┐            ┌──────────────────┐
│  IRPrinter   │            │  MIPS 后端        │
│  → llvm_ir.txt│            │  MIPSCodeGenerator│
└──────────────┘            │  → Peephole → mips.s
                            └──────────────────┘
```

阶段边界原则：**语法树纯语法 → 语义结果进 `AnalysisResult` → IR 绑定进 `IRBindings` → 优化/后端只碰 `Module`。** 只有 `Compiler.cpp` 贯穿各层。

## 目录结构

```
src/
├── FrontEnd/
│   ├── LexicalAnalysis/
│   └── SyntaxAnalysis/          # Parser + AstNode accept 实现
├── MiddleEnd/
│   ├── SemanticAnalysis/        # SemanticVisitor 拆为 Decl/Exp/Stmt + ConstEval
│   │   ├── SemanticVisitor.cpp  # 核心 Info/Context/符号辅助
│   │   ├── SemanticDecl/Exp/Stmt.cpp
│   │   ├── ConstEval.cpp        # 编译期常量折叠
│   │   ├── ClangContext.cpp / Type.cpp / Symbol.cpp / SymbolTable.cpp
│   │   └── ScopeGuard / DeclaratorTypeBuilder
│   ├── IntermediateCodeGenerate/
│   │   ├── IRVisitor.cpp + IRDecl/IRExp/IRStmt.cpp
│   │   ├── IRBuilder / IRPrinter / Module / LLVMContext / Type
│   │   └── Value/               # Constant / Instruction 层次
│   └── Optimization/
│       ├── Pass.h / PassManager.h
│       ├── IRReplaceUses.cpp    # 扫描式操作数替换（不依赖未维护 users）
│       ├── Mem2Reg.cpp          # 标量 alloca→SSA（单块 + 跨块 IDF/Phi）
│       ├── AlgebraicSimplification / IRConstantFolding
│       ├── ConstantBranchFolding / DeadBlockElimination
│       └── DeadInstructionElimination
├── BackEnd/MIPS/                # 只读 Module → mips.s
└── Error.cpp
include/
├── DeclEnums.h                  # TypeSpecifier / StorageClass / TypeQualifier（中性枚举）
├── FrontEnd/
│   └── SyntaxAnalysis/
│       ├── AstNodes.h           # AST 全集伞头（仅 Parser / 实现 .cpp 使用）
│       ├── AstVisitor.h         # 默认空 visit*；Visitor 公开头只依赖此前向接口
│       └── AstNode/             # 各结点头（不挂 Symbol*）
├── MiddleEnd/
│   ├── SemanticAnalysis/
│   │   ├── AnalysisResult.h     # 语义注解唯一来源
│   │   ├── SemanticVisitor.h
│   │   └── Symbol.h             # name/type/kind/constValue；无 Value*
│   ├── IntermediateCodeGenerate/
│   │   ├── IRBindings.h         # 会话 Symbol* → Value*
│   │   └── LLVM/ …
│   └── Optimization/
└── BackEnd/MIPS/
tests/
├── run.sh
├── runtime.c                    # getint / putint
└── cases/                       # *.sy + *.out / *.err / *.in
Compiler.cpp
CMakeLists.txt
```

### Include 约定

- 编译用 `-Iinclude`；公共头优先 `#include "FrontEnd/..."` / `"MiddleEnd/..."` / `"DeclEnums.h"`。
- **`AstNodes.h`**：整棵 AST 伞头，只给 Parser 与实现 `.cpp`；`SemanticVisitor` / `IRVisitor` 公开头不吃伞头。
- **`DeclEnums.h`**：声明相关枚举，避免 `Clang::Type.h` 依赖 AST 的 `DeclSpec`。
- **允许**：`IRBindings` 挂 `Symbol*` / `Value*`；Opt / BackEnd 只依赖 `Module`。
- **禁止**：语义头依赖 `LLVM::Value`；AST 结点挂 `Symbol*`；优化 Pass 再 traverse 源语言 AST。

## 设计要点

### 注解外置（相对「黑板 Symbol」）

| 数据 | 放哪 | 不放哪 |
|------|------|--------|
| 语法形状 | AST | — |
| Symbol / 类型 / 常量 | `AnalysisResult` | AST 字段、`Symbol::Value*` |
| IR 映射 | `IRBindings`（会话） | `Symbol` 成员 |

IR 生成时结果仍「皆是 Value」；去掉的是让语义层 `Symbol` 长住 `Value*`，以免阶段耦合与所有权不清。

### Visitor

语义与 IR 仍是双遍 Visitor（`Decl` / `Exp` / `Stmt` 实现拆文件）。`SemanticContext` / `IRContext` 用显式 bool（如 `wantConst` / `useLocation`）+ `*ContextGuard` 就地改写再恢复，**不再**用 `VisitPurpose` 枚举当控制流 DSL，热路径避免反复 `clone()` Context。`AstVisitor` 提供默认空 `visit*`，新增结点不必改所有 Visitor。

### Factory / 类型唯一化

- `ClangContext` → `Clang::Type*`（含 **FunctionType 按签名 uniquify**）
- `LLVMContext` → `LLVM::Type*`（双侧 FunctionType 同样按 `(ret, params…, variadic)` get-or-create）
- `canAssign` 等可依赖类型指针相等，前提是同签名只对应一个 `FunctionType` 实例

## 语言特性

### 支持的 C 子集

| 特性 | 说明 |
|------|------|
| 基本类型 | `int`、`char`、`void` |
| 存储类 | `static`；默认自动存储期 |
| 类型限定符 | `const`（只读限定；局部 const 允许非常量初始化） |
| 变量 / 数组 | 多声明符、多维数组、嵌套/平铺初始化、缺省补零 |
| 指针 | 声明符 `*`；一元 **`&`（取地址）** / **`*`（解引用）**；形参会先 alloca 再 store，便于 `&param` |
| 函数 | 原型 / 定义、先声明后定义；变参如 `printf(const char *, ...)` 作 **普通库调用**（无前端格式串特判） |
| 控制流 | `if/else`、`for`、`while`、`do-while`、`break`、`continue`、`return` |
| 表达式 | 算术/关系/逻辑、赋值、逗号、下标、调用、一元 `&`/`*` |
| 字符串 / 注释 | 字面量与转义；`//`、`/* */` |

### 语义规则（摘要）

- 单遍、先声明后使用；函数允许重复原型，重复定义交链接器
- `static` / 全局初始化须为常量表达式；短路 `&&`/`||` 在 IR 中用分支 + phi
- 数组传参退化为指针；下标对指针槽会先 load 再 GEP

### 已知缺口（相对完整 mini-C）

括号声明符 `int (*fp)(int)`、指针算术、`.` / `->`、`sizeof`、`++`/`--`、`struct` / `typedef` / `enum` / `switch` 尚未实现。下一优先建议：括号 Declarator → 再聚合类型。

### 错误码

按阶段分类的有意义枚举（`IllegalChar`、`MissingSemicolon`、`UndefinedName`、`ParamCountMismatch`、…），输出到 `error.txt`：`行号 中文原因 '符号名'`。完整列表见 `include/Error.h`。

## IR 与优化

### IR 类型对应

| IR | C |
|----|---|
| `i32` / `i8` | `int` / `char` |
| `[N x i32]` | `int[N]` |
| `i8*` 等 | 指针 |
| 函数类型 | 含可变参数 |

### Pass 流水线（已全部接入 `Compiler.cpp`）

| 顺序 | Pass | 作用 |
|------|------|------|
| 1 | **Mem2Reg** | 可提升标量 alloca → SSA（单块线性；跨块迭代支配边界插 Phi + rename）。不提升取过地址 / 类型不匹配者 |
| 2 | AlgebraicSimplification | `x+0`、`x*1`、`x*0` 等 |
| 3 | IRConstantFolding | 常量运算 / icmp 折叠 |
| 4 | ConstantBranchFolding | `br i1 true/false` → 直跳 |
| 5 | DeadBlockElimination | 不可达块；后继以 Phi 开头时跳过危险合并 |
| 6 | DeadInstructionElimination | 无副作用且结果无引用的安全白名单删除 |

操作数替换经 `IRReplaceUses` 扫描式完成（不依赖未维护的 `users` 列表）。打印发生在全部 Pass 之后。尚未做：完整 GVN、循环优化、函数内联；下一性价比方向仍是加强 Mem2Reg / CSE 等。

## MIPS 后端

只读 `Module` → `MIPSCodeGenerator`（Phi 消除、寄存器缓存、GEP 偏移）→ `MIPSPeepholeOptimizer` → `mips.s`（O32：`$a0`–`$a3` + 栈上传参）。中端优化与后端窥孔分工，避免在 `IRVisitor` 里塞优化。

## 测试与运行时

`tests/run.sh`：对 `tests/cases/` 下用例编译 → 匹配 `.err` 或链接 `runtime.c` 跑 IR → 匹配 `.out`。

`tests/runtime.c` 提供 `getint` / `putint`；用户程序也可外部声明变参 `printf`，按普通 `call` 处理。

用例覆盖基础、控制流、数组、常量/`static`、函数、`printf`、若干错误路径等。指针取地址/解引用已在语言与 IR 中支持；本地可用含 `&`/`*` 的源文件自行验证。

## 工程编译注意

- **Debug**（默认）：`-O0 -g`，便于改编译器本身时的增量速度
- **Release**：`-O2`
- **ccache**：检测到则自动作为 compiler launcher
- 拆分 Decl/Exp/Stmt 与瘦 Visitor 头后，改单文件重编面更小；公开头尽量前向声明，重 include 留在 `.cpp`

## 迭代过程

详见 [GitHub](https://github.com/tfcm05/BUAA_SysY_Compiler_2025)
