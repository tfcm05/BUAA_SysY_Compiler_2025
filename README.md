# BUAA_SysY_Compiler_2025

## 文法

### Exp
BinaryExp:CommaExp, AddExp, MulExp, OrExp, AndExp, RelExp
AssignExp: 
UnaryExp: PlusExp, MinusExp, NotExp
PostfixExp: SubscriptExp, FunctionCallExp
PrimaryExp: LiteralExp, IdentifierExp

(BinaryExp | AssignExp) -> (BinaryExp | AssignExp) BinaryOp UnaryExp | UnaryExp
UnaryExp -> UnaryOp UnaryExp | PostfixExp
PostfixExp -> SubscriptExp [Exp] | FunctionCallExp ([RealParams]) | PrimaryExp
RealParams -> Exp {,Exp}
Primary -> LiteralExp | IdentifierExp

### Stmt
Stmt:BlockStmt, ExpStmt, ForStmt, IfStmt,  ContinueStmt, BreakStmt, ReturnStmt, VariableDecl, FunctionDecl

Stmt -> BlockStmt | ExpStmt | ... | FunctionDecl
BlockStmt -> '{'Stmt'}'
ExpStmt -> Exp;

// IterationExp
ForStmt -> for([Exp];[Exp];[Exp])Stmt

// SelectionStmt
IfStmt -> if([Exp])Stmt[else Stmt]

// JumpStmt
ReturnStmt -> return [Exp];

VariableDecl ->
FunctionDecl ->