#include"../../../include/FrontEnd/SyntaxAnalysis/SyntaxAnalysis.h"
#include"../../../include/Error.h"
#include<fstream>
#include<sstream>
#include<iostream>

Parser::Parser(LexicalAnalysis& lex): lexicalAnalysis(lex) {
    initial();
}

AstNodePtr Parser::parse() {
    return parseCompUnit();
}

// ----- Top-level: compilation unit -----
CompUnitPtr Parser::parseCompUnit() {
    auto node = std::make_unique<CompUnit>(CURRENT_LINE);

    // 交替解析任意顺序的顶层声明 (变量声明与函数声明可交错出现)
    while (match(CONSTTK) || match(VOIDTK) || (match(INTTK) && match(IDENFR, 1))) {
        if (match(CONSTTK) || (match(INTTK) && match(IDENFR, 1) && !match(LPARENT, 2))) {
            node->addVarDecl(parseVarDecl());
        } else {
            node->addFuncDecl(parseFuncDecl());
        }
    }

    node->setMainFuncDecl(parseMainFuncDecl());

    return node;
}

// ----- Initialization & token helpers -----
void Parser::initial() {
    preLine = 0;
    currentToken = lexicalAnalysis.getNextToken();
}

void Parser::advance() {
    preLine = currentToken.getLine();
    currentToken = lexicalAnalysis.getNextToken();
}

Token Parser::peekNext() {
    return lexicalAnalysis.peekNextToken();
}

Token Parser::peekSecond() {
    return lexicalAnalysis.peekSecondToken();
}

const Token& Parser::peekNextRef() {
    return lexicalAnalysis.peekNextTokenRef();
}

const Token& Parser::peekSecondRef() {
    return lexicalAnalysis.peekSecondTokenRef();
}

bool Parser::match(TokenType expected, int mode) {
    switch (mode) {
        case 0 : return CURRENT_TYPE == expected;
        case 1 : return NEXT_TYPE == expected;
        case 2 : return SECOND_TYPE == expected;
        default : return -1;
    }
}

bool Parser::match(TokenType expected) {
    return CURRENT_TYPE == expected;
}

void Parser::expect(TokenType expect) {
    if (!match(expect)) {
        // 错误处理
        
        std::cout << "Error: Expected token " << getTypeCode(expect)
                  << " but found token " << getTypeCode(CURRENT_TYPE)
                  << " at line " << CURRENT_LINE << std::endl;
        
        if (expect == SEMICN) {
            errorLog(ErrorCode::i, preLine);
        } else if (expect == RPARENT) {
            errorLog(ErrorCode::j, preLine);
        } else if (expect == RBRACK) {
            errorLog(ErrorCode::k, preLine);
        }

        // 失败时也推进一个 token (文件末尾除外), 防止语法错误导致无限循环
        if (CURRENT_TYPE != END) {
            advance();
        }
    } else {
        std::cout << "Matched token: " << getTypeCode(CURRENT_TYPE)
                  << " with value \"" << CURRENT_VALUE << "\" at line " << CURRENT_LINE << std::endl;
        advance();
    }
}

// ----- 声明相关 -----
DeclSpec Parser::parseDeclSpec() {
    DeclSpec declSpec = DeclSpec();
    while (match(INTTK) || match(VOIDTK) || match(CHARTK) || match(CONSTTK) || match(STATICTK)) {
        if (match(INTTK)) {
            expect(INTTK);
            declSpec.setTypeSpecifier(TypeSpecifier::INT);
        } else if (match(VOIDTK)) {
            expect(VOIDTK);
            declSpec.setTypeSpecifier(TypeSpecifier::VOID);
        } else if (match(CHARTK)) {
            expect(CHARTK);
            declSpec.setTypeSpecifier(TypeSpecifier::CHAR);
        } else if (match(CONSTTK)) {
            expect(CONSTTK);
            declSpec.setTypeQualifier(TypeQualifier::CONST);
        } else {
            expect(STATICTK);
            declSpec.setStorageClass(StorageClass::STATIC);
        }
    }
    return declSpec;
}

DeclaratorPtr Parser::parseDeclarator() {
    size_t startLine = CURRENT_LINE;

    // 指针修饰: C 声明符中的 * (如 const char *format 中的 * 属于 format 的声明符)
    if (match(MULT)) {
        expect(MULT);
        auto inner = parseDeclarator();
        return std::make_unique<PointerDeclarator>(std::move(inner), startLine);
    }

    DeclaratorPtr declarator;
    if (match(IDENFR)) {
        declarator = std::make_unique<IdentDeclarator>(CURRENT_VALUE, startLine);
        expect(IDENFR);
    }
    
    if (match(LBRACK)) {
        // 数组 (支持多维)
        std::vector<ExpPtr> arraySizeList;
        while (match(LBRACK)) {
            expect(LBRACK);
            arraySizeList.push_back(parseLOrExp());
            expect(RBRACK);
        }
        declarator = std::make_unique<ArrayDeclarator>(std::move(declarator), std::move(arraySizeList), startLine);
    } else if (match(LPARENT)) {
        // 函数
        expect(LPARENT);
        std::vector<ParameterDeclPtr> params;
        if (!match(RPARENT)) {
            bool isVariadic = false;
            if (!match(ELLIPSIS)) {
                params.push_back(parseParameterDecl());
                while (match(COMMA)) {
                    expect(COMMA);
                    if (match(ELLIPSIS)) {
                        isVariadic = true;
                        break;
                    }
                    params.push_back(parseParameterDecl());
                }
            } else {
                isVariadic = true;
            }
            if (isVariadic) {
                expect(ELLIPSIS);
            }
            expect(RPARENT);
            declarator = std::make_unique<FuncDeclarator>(std::move(declarator), std::move(params), startLine, isVariadic);
        } else {
            expect(RPARENT);
            declarator = std::make_unique<FuncDeclarator>(std::move(declarator), std::move(params), startLine, false);
        }
    }

    if (match(ASSIGN)) {
        expect(ASSIGN);
        declarator = std::make_unique<InitialDeclarator>(std::move(declarator), parseInitializer(), startLine);
    }

    // C abstract declarator: 函数原型可省略参数名 (如 void putint(int);)
    // 未匹配到标识符且无数组/函数修饰时, 生成匿名标识符占位, 使 
    // visitParameterDecl 的 getDeclarator()->accept() 不因空指针崩溃
    if (!declarator) {
        declarator = std::make_unique<IdentDeclarator>(
            "__anon_param_" + std::to_string(startLine), startLine);
    }

    return declarator;
}

InitializerPtr Parser::parseInitializer() {
    size_t startLine = CURRENT_LINE;
    if (match(LBRACE)) {
        expect(LBRACE);
        auto listInit = std::make_unique<ListInitializer>(startLine);
        if (!match(RBRACE)) {
            listInit->addInitializer(parseInitializer());
            while (match(COMMA)) {
                expect(COMMA);
                listInit->addInitializer(parseInitializer());
            }
        }
        expect(RBRACE);
        return listInit;
    } else {
        auto expInit = std::make_unique<ExpInitializer>(parseLOrExp(), startLine);
        return expInit;
    }
}

VariableDeclPtr Parser::parseVarDecl() {
    auto variableDecl = std::make_unique<VariableDecl>(CURRENT_LINE);

    variableDecl->setDeclSpec(parseDeclSpec());

    variableDecl->addDeclarator(parseDeclarator());
    while (match(COMMA)) {
        expect(COMMA);
        variableDecl->addDeclarator(parseDeclarator());
    }

    expect(SEMICN);
    return variableDecl;
}

FunctionDeclPtr Parser::parseFuncDecl() {
    auto functionDecl = std::make_unique<FunctionDecl>(CURRENT_LINE);

    functionDecl->setDeclSpec(parseDeclSpec());

    functionDecl->setDeclarator(parseDeclarator());

    if (match(SEMICN)) {
        // 函数声明 (无函数体), 链接时由运行库提供实现
        expect(SEMICN);
    } else {
        functionDecl->setBody(parseBlockStmt());
    }

    return functionDecl;
}

ParameterDeclPtr Parser::parseParameterDecl() {
    auto parameterDecl =  std::make_unique<ParameterDecl>(CURRENT_LINE);

    parameterDecl->setDeclSpec(parseDeclSpec());

    parameterDecl->setDeclarator(parseDeclarator());

    return parameterDecl;
}

FunctionDeclPtr Parser::parseMainFuncDecl() {
    auto mainFuncDecl = std::make_unique<FunctionDecl>(CURRENT_LINE);

    expect(INTTK);
    DeclSpec declSpec;
    declSpec.setTypeSpecifier(TypeSpecifier::INT);
    mainFuncDecl->setDeclSpec(std::move(declSpec));

    auto identDeclarator = std::make_unique<IdentDeclarator>("main", CURRENT_LINE);
    expect(MAINTK);
    expect(LPARENT);
    expect(RPARENT);
    size_t mainDeclLine = identDeclarator->getStartLine();
    auto mainDeclarator = std::make_unique<FuncDeclarator>(std::move(identDeclarator), std::vector<ParameterDeclPtr>{}, mainDeclLine);
    mainFuncDecl->setDeclarator(std::move(mainDeclarator));

    mainFuncDecl->setBody(parseBlockStmt());

    return mainFuncDecl;
}

// ----- 语句与语句块 -----
BlockStmtPtr Parser::parseBlockStmt() {
    auto node = std::make_unique<BlockStmt>(CURRENT_LINE);
    expect(LBRACE);
    
    while (!match(RBRACE)) {
        if (match(END)) {
            // 语法错误: 块未闭合, 直接终止避免死循环
            break;
        }
        if (match(STATICTK) || match(CONSTTK) || (match(INTTK) && match(IDENFR, 1) && !match(LPARENT, 2))) {
            node->addBlockItem(parseVarDecl());
        } else {
            node->addBlockItem(parseStmt());
        }
    }
    
    node->setEndLine(CURRENT_LINE);
    expect(RBRACE);
    
    return node;
}

ExpStmtPtr Parser::parseExpStmt() {
    ExpStmtPtr node = std::make_unique<ExpStmt>(CURRENT_LINE);
    if (!match(SEMICN)) {      
        node->setExp(parseAssignExp());
        if (!node->getExp() && !match(SEMICN)) {
            // 表达式解析失败且未停在分号: 强制推进一个 token, 防止死循环
            advance();
        }
    }
    expect(SEMICN);
    return node;
}

IfStmtPtr Parser::parseIfStmt() {
    auto node = std::make_unique<IfStmt>(CURRENT_LINE);

    expect(IFTK);
    expect(LPARENT);
    node->setCondition(parseLOrExp());
    expect(RPARENT);
    node->setTrueStmt(parseStmt());
    if (match(ELSETK)) {
        expect(ELSETK);
        node->setFalseStmt(parseStmt());
    }

    return node;
}

ForStmtPtr Parser::parseForStmt() {
    auto node = std::make_unique<ForStmt>(CURRENT_LINE);

    expect(FORTK);
    expect(LPARENT);
    bool declInit = false;
    if (!match(SEMICN)) {
        // for(int i = 0; cond; step) 形式的初始化声明 (SysY 文法 InitStmt 可为声明)
        // C 语法: for 头声明只能是普通/[const] 变量声明, 不允许 static
        if (match(INTTK) || (match(CONSTTK) && match(INTTK, 1))) {
            node->setInitial(parseVarDecl()); // parseVarDecl 内部消费 ';'
            declInit = true;
        } else {
            node->setInitial(parseCommaExp());
        }
    }
    if (!declInit) {
        expect(SEMICN);
    }
    if (!match(SEMICN)) {
        node->setCondition(parseLOrExp());
    } 
    expect(SEMICN);
    if (!match(SEMICN)) {
        node->setIteration(parseCommaExp());
    }
    expect(RPARENT);
    node->setBody(parseStmt());

    return node;
}

WhileStmtPtr Parser::parseWhileStmt() {
    auto node = std::make_unique<WhileStmt>(CURRENT_LINE);

    expect(WHILETK);
    expect(LPARENT);
    node->setCondition(parseLOrExp());
    expect(RPARENT);
    node->setBody(parseStmt());

    return node;
}

DoWhileStmtPtr Parser::parseDoWhileStmt() {
    auto node = std::make_unique<DoWhileStmt>(CURRENT_LINE);

    expect(DOTK);
    node->setBody(parseStmt());
    expect(WHILETK);
    expect(LPARENT);
    node->setCondition(parseLOrExp());
    expect(RPARENT);
    expect(SEMICN);

    return node;
}

BreakStmtPtr Parser::parseBreakStmt() {
    auto node = std::make_unique<BreakStmt>(CURRENT_LINE);

    expect(BREAKTK);
    expect(SEMICN);

    return node;
}

ContinueStmtPtr Parser::parseContinueStmt() {
    auto node = std::make_unique<ContinueStmt>(CURRENT_LINE);

    expect(CONTINUETK);
    expect(SEMICN);

    return node;
}

ReturnStmtPtr Parser::parseReturnStmt() {
    auto node = std::make_unique<ReturnStmt>(CURRENT_LINE);

    expect(RETURNTK);
    if (!match(SEMICN)) {
        node->setReturnExp(parseLOrExp());
    }
    expect(SEMICN);
    
    return node;
}

StmtPtr Parser::parseStmt() {
    if (match(LBRACE)) {
        return parseBlockStmt();
    } else if (match(IFTK)) {
        return parseIfStmt();
    } else if (match(FORTK)) {
        return parseForStmt();
    } else if (match(WHILETK)) {
        return parseWhileStmt();
    } else if (match(DOTK)) {
        return parseDoWhileStmt();
    } else if (match(BREAKTK) || match(CONTINUETK) || match(RETURNTK)) {
        if (match(BREAKTK) ) {
            return parseBreakStmt();
        } else if (match(CONTINUETK)) {
            return parseContinueStmt();
        } else  {  
            return parseReturnStmt();
        }
    } else {
        return parseExpStmt();
    }
}

// ----- 表达式相关 -----
ExpPtr Parser::parseCommaExp() {
    auto node = parseAssignExp();
    if (!node) {
        return nullptr;
    }
    if (match(COMMA)) {
        auto commaExp = std::make_unique<CommaExp>(node->getStartLine());
        commaExp->addExp(std::move(node));
        while (match(COMMA)) {
            expect(COMMA);
            auto exp = parseAssignExp();
            commaExp->addExp(std::move(exp));
        }
        return commaExp;
    } else {
        return node;
    }
}

ExpPtr Parser::parseAssignExp() {
    auto node = parseLOrExp();
    if (!node) {
        return nullptr;
    }
    if (match(ASSIGN)) {
        size_t sl = node->getStartLine();
        expect(ASSIGN);
        auto rhs = parseAssignExp();
        return std::make_unique<AssignExp>(std::move(node), AssignExp::AssignOp::ASSIGN, std::move(rhs), sl);
    }
    return node;
}

ExpPtr Parser::parseLOrExp() {
    auto node = parseLAndExp();
    while (node && match(OR)) {
        size_t sl = node->getStartLine();
        expect(OR);
        auto rhs = parseLAndExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::OR, std::move(rhs), sl);
    }
    return node;
}

ExpPtr Parser::parseLAndExp() {
    auto node = parseEqExp();
    while (node && match(AND)) {
        size_t sl = node->getStartLine();
        expect(AND);
        auto rhs = parseEqExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::AND, std::move(rhs), sl);
    }
    return node;
}

ExpPtr Parser::parseEqExp() {
    auto node = parseRelExp();
    while (node && (match(EQL) || match(NEQ))) {
        size_t sl = node->getStartLine();
        if (match(EQL)) {
            expect(EQL);
            auto rhs = parseRelExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::EQL, std::move(rhs), sl);
        } else if (match(NEQ)) {
            expect(NEQ);
            auto rhs = parseRelExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::NEQ, std::move(rhs), sl);
        }
    }
    return node;
}

ExpPtr Parser::parseRelExp() {
    auto node = parseAddExp();
    while (node && (match(GRE) || match(GEQ) || match(LSS) || match(LEQ))) {
        size_t sl = node->getStartLine();
        if (match(GRE)) {
            expect(GRE);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::GRE, std::move(rhs), sl);
        } else if (match(GEQ)) {
            expect(GEQ);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::GEQ, std::move(rhs), sl);
        } else if (match(LSS)) {
            expect(LSS);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::LSS, std::move(rhs), sl);
        } else if (match(LEQ)) {
            expect(LEQ);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::LEQ, std::move(rhs), sl);
        }
    }
    return node;
}

ExpPtr Parser::parseAddExp() {
    auto node = parseMulExp();
    while (node && (match(PLUS) || match(MINU))) {
        size_t sl = node->getStartLine();
        if (match(PLUS)) {
            expect(PLUS);
            auto rhs = parseMulExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::PLUS, std::move(rhs), sl);
        } else if (match(MINU)) {
            expect(MINU);
            auto rhs = parseMulExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MINU, std::move(rhs), sl);
        }
    }
    return node;
}

ExpPtr Parser::parseMulExp() {
    auto node = parseUnaryExp();
    while (node && (match(MULT) || match(DIV) || match(MOD))) {
        size_t sl = node->getStartLine();
        if (match(MULT)) {
            expect(MULT);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MULT, std::move(rhs), sl);
        } else if (match(DIV)) {
            expect(DIV);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::DIV, std::move(rhs), sl);
        } else if (match(MOD)) {
            expect(MOD);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MOD, std::move(rhs), sl);
        }
    }                                                       
    return node;
}

ExpPtr Parser::parseUnaryExp() {
    if (match(PLUS)) {
        size_t sl = CURRENT_LINE;
        expect(PLUS);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::PLUS, parseUnaryExp(), sl);
    } else if (match(MINU)) {
        size_t sl = CURRENT_LINE;
        expect(MINU);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::MINU, parseUnaryExp(), sl);
    } else if (match(NOT)) {
        size_t sl = CURRENT_LINE;
        expect(NOT);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::NOT, parseUnaryExp(), sl);
    } else {
        return parsePostfixExp();
    }
}

ExpPtr Parser::parsePostfixExp() {
    auto node = parsePrimaryExp();
    if (!node) {
        return nullptr;
    }

    // 数组下标(支持多维) 或 函数调用
    if (match(LBRACK)) {
        std::vector<ExpPtr> subscripts;
        while (match(LBRACK)) {
            expect(LBRACK);
            subscripts.push_back(parseLOrExp());
            expect(RBRACK);
        }
        size_t sl = node->getStartLine();
        node = std::make_unique<SubscriptExp>(std::move(node), std::move(subscripts), sl);
    } else if (match(LPARENT)) {
        expect(LPARENT);
        std::vector<ExpPtr> realParams;
        if (!match(RPARENT)) {
            realParams.push_back(parseLOrExp());
            while (match(COMMA)) {
                expect(COMMA);
                realParams.push_back(parseLOrExp());
            }
        }
        expect(RPARENT);
        size_t sl = node->getStartLine();
        node = std::make_unique<FunctionCallExp>(std::move(node), std::move(realParams), sl);
    } 
    return node;
}

ExpPtr Parser::parsePrimaryExp() {
    if (match(LPARENT)) {
        expect(LPARENT);
        // 括号内是完整表达式(含赋值/逗号), 符合 C/SysY 文法 Exp -> '(' Exp ')'
        auto node = parseCommaExp();
        expect(RPARENT);
        return node;
    } else if (match(INTCON) || match(STRCON)) {
        return parseLiteralExp();
    } else if (match(IDENFR)) {
        return parseIdentifierExp();
    } else {
        return nullptr;
    }
}

ExpPtr Parser::parseIdentifierExp() {
    auto idExp = std::make_unique<IdentifierExp>(CURRENT_VALUE, CURRENT_LINE);
    expect(IDENFR);
    return idExp;
}

ExpPtr Parser::parseLiteralExp() {
    if (match(INTCON)) {
        int val = std::stoi(CURRENT_VALUE);
        expect(INTCON);
        return std::make_unique<LiteralExp>(val, CURRENT_LINE);
    } else if (match(STRCON)) {
        std::string val = std::move(CURRENT_VALUE);
        expect(STRCON);
        return std::make_unique<LiteralExp>(val, CURRENT_LINE);
    }
    return nullptr;
}

// ----- AST printing for debugging -----
void Parser::parserLog(const AstNode* root) const {
    std::ofstream ofs("parser.txt");
    if (!ofs.is_open()) return;

    auto indent = [](int lvl) {
        return std::string(lvl * 2, ' ');
    };

    struct Printer {
        std::ofstream& ofs;
        decltype(indent)& indentFn;

        std::string declSpec(const DeclSpec& ds) const {
            std::string res;
            if (ds.isStatic()) {
                res += "static ";
            }
            if (ds.isConst()) {
                res += "const ";
            }
            if (ds.getTypeSpecifier() == TypeSpecifier::INT) {
                res += "int";
            } else if (ds.getTypeSpecifier() == TypeSpecifier::VOID) {
                res += "void";
            } else {
                res += "unknown_type";
            }
            return res;
        }

        const char* binOpName(BinaryExp::BinaryOp op) const {
            switch (op) {
                case BinaryExp::BinaryOp::PLUS:   return "+";
                case BinaryExp::BinaryOp::MINU:   return "-";
                case BinaryExp::BinaryOp::MULT:   return "*";
                case BinaryExp::BinaryOp::DIV:    return "/";
                case BinaryExp::BinaryOp::MOD:    return "%";
                case BinaryExp::BinaryOp::OR:     return "||";
                case BinaryExp::BinaryOp::AND:    return "&&";
                case BinaryExp::BinaryOp::EQL:    return "==";
                case BinaryExp::BinaryOp::NEQ:    return "!=";
                case BinaryExp::BinaryOp::GRE:    return ">";
                case BinaryExp::BinaryOp::GEQ:    return ">=";
                case BinaryExp::BinaryOp::LSS:    return "<";
                case BinaryExp::BinaryOp::LEQ:    return "<=";
                default:                           return "?";
            }
        }

        const char* assignOpName(AssignExp::AssignOp op) const {
            switch (op) {
                case AssignExp::AssignOp::ASSIGN:     return "=";
                default:                            return "?=";
            }
        }

        const char* unaryOpName(UnaryExp::UnaryOp op) const {
            switch (op) {
                case UnaryExp::UnaryOp::PLUS:    return "+";
                case UnaryExp::UnaryOp::MINU:    return "-";
                case UnaryExp::UnaryOp::NOT:     return "!";
                default:                         return "?";
            }
        }

        void printNode(const AstNode* node, int lvl) {
            if (!node) return;

            // ===== Expressions =====
            if (auto n = dynamic_cast<const LiteralExp*>(node)) {
                if (n->getLiteralType() == LiteralExp::LiteralType::STRING) {
                    ofs << indentFn(lvl) << "String: " << n->getStringValue() << " (line " << n->getStartLine() << ")\n";
                } else {
                    ofs << indentFn(lvl) << "Integer: " << n->getNumberValue() << " (line " << n->getStartLine() << ")\n";
                }
                return;
            }
            if (auto id = dynamic_cast<const IdentifierExp*>(node)) {
                ofs << indentFn(lvl) << "Identifier: " << id->getIdentifier() << " (line " << id->getStartLine() << ")\n";
                return;
            }
            if (auto fc = dynamic_cast<const FunctionCallExp*>(node)) {
                ofs << indentFn(lvl) << "FunctionCall (line " << fc->getStartLine() << "):\n";
                ofs << indentFn(lvl+1) << "Function:\n";
                printNode(fc->getFunction(), lvl+2);
                ofs << indentFn(lvl+1) << "Params:\n";
                for (size_t i = 0; i < fc->getArgumentsCount(); ++i) {
                    printNode(fc->getArgument(i), lvl+2);
                }
                return;
            }
            if (auto sub = dynamic_cast<const SubscriptExp*>(node)) {
                ofs << indentFn(lvl) << "Subscript: (line " << sub->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Array:\n";
                printNode(sub->getArray(), lvl+2);
                ofs << indentFn(lvl+1) << "Indexs:\n";
                for (size_t i = 0; i < sub->getSubscriptsCount(); ++i) {
                    printNode(sub->getSubscript(i), lvl+2);
                }
                return;
            }
            if (auto u = dynamic_cast<const UnaryExp*>(node)) {
                ofs << indentFn(lvl) << "UnaryExp: " << unaryOpName(u->getUnaryOp()) << " (line " << u->getStartLine() << ")\n";
                printNode(u->getExp(), lvl+1);
                return;
            }
            if (auto a = dynamic_cast<const AssignExp*>(node)) {
                ofs << indentFn(lvl) << "AssignExp: " << assignOpName(a->getAssignOp()) << " (line " << a->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "LHS:\n";
                printNode(a->getLhs(), lvl+2);
                ofs << indentFn(lvl+1) << "RHS:\n";
                printNode(a->getRhs(), lvl+2);
                return;
            }
            if (auto c = dynamic_cast<const CommaExp*>(node)) {
                ofs << indentFn(lvl) << "CommaExp: (line " << c->getStartLine() << ")\n";
                for (size_t i = 0; i < c->getExpsCount(); ++i) {
                    printNode(c->getExp(i), lvl+1);
                }
                return;
            }
            if (auto b = dynamic_cast<const BinaryExp*>(node)) {
                ofs << indentFn(lvl) << "BinaryExp: " << binOpName(b->getBinaryOp()) << " (line " << b->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "LHS:\n";
                printNode(b->getLhs(), lvl+2);
                ofs << indentFn(lvl+1) << "RHS:\n";
                printNode(b->getRhs(), lvl+2);
                return;
            }

            // ===== Declarations =====
            if (auto cu = dynamic_cast<const CompUnit*>(node)) {
                ofs << indentFn(lvl) << "CompUnit\n";
                ofs << indentFn(lvl+1) << "VarDecls:\n";
                for (size_t i = 0; i < cu->getVarDeclsCount(); ++i) {
                    printNode(cu->getVarDecl(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "FuncDecls:\n";
                for (size_t i = 0; i < cu->getFuncDeclsCount(); ++i) {
                    printNode(cu->getFuncDecl(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Main:\n";
                printNode(cu->getMainFuncDecl(), lvl+2);
                return;
            }
            if (auto vds = dynamic_cast<const VariableDecl*>(node)) {
                ofs << indentFn(lvl) << "VariableDecl: (line " << vds->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "DeclSpec:\n";
                ofs << indentFn(lvl+2) << declSpec(vds->getDeclSpec()) << "\n";
                ofs << indentFn(lvl+1) << "Declarators:\n";
                for (size_t i = 0; i < vds->getDeclaratorsCount(); ++i) {
                    printNode(vds->getDeclarator(i), lvl+2);
                }
                return;
            }
            if (auto fd = dynamic_cast<const FunctionDecl*>(node)) {
                ofs << indentFn(lvl) << "FunctionDecl: (line " << fd->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "DeclSpec:\n";
                ofs << indentFn(lvl+2) << declSpec(fd->getDeclSpec()) << "\n";
                ofs << indentFn(lvl+1) << "Declarator:\n";
                printNode(fd->getDeclarator(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n";
                printNode(fd->getBody(), lvl+2);
                return;
            }
            if (auto p = dynamic_cast<const ParameterDecl*>(node)) {
                ofs << indentFn(lvl) << "ParameterDecl: (line " << p->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "DeclSpec:\n";
                ofs << indentFn(lvl+2) << declSpec(p->getDeclSpec()) << "\n";
                ofs << indentFn(lvl+1) << "Declarator:\n";
                printNode(p->getDeclarator(), lvl+2);
                return;
            }

            if (auto d = dynamic_cast<const IdentDeclarator*>(node)) {
                ofs << indentFn(lvl) << "IdentDeclarator: " << d->getIdentifier() << " (line " << d->getStartLine() << ")\n";
                return;
            }
            if (auto a = dynamic_cast<const ArrayDeclarator*>(node)) {
                ofs << indentFn(lvl) << "ArrayDeclarator: (line " << a->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Array:\n";
                printNode(a->getArray(), lvl+2);
                ofs << indentFn(lvl+1) << "Sizes:\n";
                for (size_t i = 0; i < a->getArraySizesCount(); ++i) {
                    printNode(a->getArraySize(i), lvl+2);
                }
                return;
            }
            if (auto f = dynamic_cast<const FuncDeclarator*>(node)) {
                ofs << indentFn(lvl) << "FuncDeclarator: (line " << f->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Function:\n";
                printNode(f->getFunction(), lvl+2);
                ofs << indentFn(lvl+1) << "Params:\n";
                for (size_t i = 0; i < f->getParametersCount(); ++i) {
                    printNode(f->getParameter(i), lvl+2);
                }
                return;
            }
            if (auto i = dynamic_cast<const InitialDeclarator*>(node)) {
                ofs << indentFn(lvl) << "InitialDeclarator: (line " << i->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Declarator:\n";
                printNode(i->getDeclarator(), lvl+2);
                ofs << indentFn(lvl+1) << "Initializer:\n";
                printNode(i->getInitializer(), lvl+2);
                return;
            }
            if (auto li = dynamic_cast<const ListInitializer*>(node)) {
                ofs << indentFn(lvl) << "ListInitializer: (line " << li->getStartLine() << ")\n";
                for (size_t i = 0; i < li->getInitializersCount(); ++i) {
                    printNode(li->getInitializer(i), lvl+1);
                }
                return;
            }
            if (auto ei = dynamic_cast<const ExpInitializer*>(node)) {
                ofs << indentFn(lvl) << "ExpInitializer: (line " << ei->getStartLine() << ")\n";
                printNode(ei->getExp(), lvl+1);
                return;
            }

            // ===== Statements =====
            if (auto bs = dynamic_cast<const BlockStmt*>(node)) {
                ofs << indentFn(lvl) << "BlockStmt (line " << bs->getStartLine()
                    << " - " << bs->getEndLine() << ") " << "size: " << bs->getBlockItemsCount() << "\n";
                for (size_t i = 0; i < bs->getBlockItemsCount(); ++i) {
                    printNode(bs->getBlockItem(i), lvl+1);
                }
                return;
            }
            if (auto ifs = dynamic_cast<const IfStmt*>(node)) {
                ofs << indentFn(lvl) << "IfStmt (line " << ifs->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Cond:\n"; printNode(ifs->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Then:\n"; printNode(ifs->getTrueStmt(), lvl+2);
                ofs << indentFn(lvl+1) << "Else:\n"; printNode(ifs->getFalseStmt(), lvl+2);
                return;
            }
            if (auto fs = dynamic_cast<const ForStmt*>(node)) {
                ofs << indentFn(lvl) << "ForStmt (line " << fs->getStartLine() << ")\n";
                if (fs->getInitial()) {
                    if (dynamic_cast<const VariableDecl*>(fs->getInitial())) {
                        ofs << indentFn(lvl+1) << "InitialDecl:\n";
                    } else {
                        ofs << indentFn(lvl+1) << "Initial:\n";
                    }
                    printNode(fs->getInitial(), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Condition:\n"; printNode(fs->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Iteration:\n"; printNode(fs->getIteration(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n";      printNode(fs->getBody(),      lvl+2);
                return;
            }
            if (auto ws = dynamic_cast<const WhileStmt*>(node)) {
                ofs << indentFn(lvl) << "WhileStmt (line " << ws->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Condition:\n"; printNode(ws->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n";      printNode(ws->getBody(),      lvl+2);
                return;
            }
            if (auto dws = dynamic_cast<const DoWhileStmt*>(node)) {
                ofs << indentFn(lvl) << "DoWhileStmt (line " << dws->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Condition:\n"; printNode(dws->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n";      printNode(dws->getBody(),      lvl+2);
                return;
            }
            if (auto rs = dynamic_cast<const ReturnStmt*>(node)) {
                ofs << indentFn(lvl) << "ReturnStmt (line " << rs->getStartLine() << ")\n";
                if (rs->getReturnExp()) {
                    ofs << indentFn(lvl+1) << "ReturnExp:\n";
                    printNode(rs->getReturnExp(), lvl+2);
                }
                return;
            }
            if (auto br = dynamic_cast<const BreakStmt*>(node)) {
                ofs << indentFn(lvl) << "BreakStmt (line " << br->getStartLine() << ")\n";
                return;
            }
            if (auto ct = dynamic_cast<const ContinueStmt*>(node)) {
                ofs << indentFn(lvl) << "ContinueStmt (line " << ct->getStartLine() << ")\n";
                return;
            }
            if (auto es = dynamic_cast<const ExpStmt*>(node)) {
                ofs << indentFn(lvl) << "ExpStmt (line " << es->getStartLine() << ")\n";
                ofs << indentFn(lvl+1) << "Exp:\n";
                printNode(es->getExp(), lvl+2);
                return;
            }

            // fallback
            ofs << indentFn(lvl) << "UnknownNode\n";
        }
    };

    Printer p{ofs, const_cast<decltype(indent)&>(indent)}; // indent 无状态，安全使用
    p.printNode(root, 0);

    ofs.close();
}