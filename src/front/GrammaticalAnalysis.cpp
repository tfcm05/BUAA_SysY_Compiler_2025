#include"../../include/front/GrammaticalAnalysis.h"
#include"../../include/Error.h"
#include<fstream>
#include<sstream>
#include<iostream>

Parser::Parser(LexicalAnalysis& lex): lexicalAnalysis(lex) {
    initial();
}

std::unique_ptr<AstNode> Parser::parse() {
    return parseCompUnit();
}

// ----- Top-level: compilation unit -----
std::unique_ptr<CompUnit> Parser::parseCompUnit() {
    auto node = std::make_unique<CompUnit>();

    while (match(CONSTTK) || (match(INTTK) && match(IDENFR, 1) && !match(LPARENT, 2))) {
        node->addGlobalVarDecl(parseVarDecl());
    }

    while (match(VOIDTK) || (match(INTTK) && match(IDENFR, 1))) {
        node->addFuncDecl(parseFuncDecl());
    }

    node->setMainFuncDecl(parseMainFuncDecl());

    return node;
}

// ----- Initialization & token helpers -----
void Parser::initial() {
    preLine = 0;
    currentToken = lexicalAnalysis.getNextToken();
    lexerLog(currentToken);
}

void Parser::advance() {
    preLine = currentToken.getLine();
    currentToken = lexicalAnalysis.getNextToken();
    lexerLog(currentToken);
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
        if (expect == SEMICN) {
            errorLog(ErrorCode::i, preLine);
        } else if (expect == RPARENT) {
            errorLog(ErrorCode::j, preLine);
        } else if (expect == RBRACK) {
            errorLog(ErrorCode::k, preLine);
        }
    } else {
        advance();
    }
}

// ----- 类型相关 -----

BasicType Parser::parseBasicType() {
    if (match(INTTK)) {
        expect(INTTK);
        return BasicType::INT;
    } else {
        expect(VOIDTK);
        return BasicType::VOID;
    }
}

// ----- 声明相关 -----

std::unique_ptr<VariableDecl> Parser::parseVarDecl() {
    auto node = std::make_unique<VariableDecl>(CURRENT_LINE);

    if (match(CONSTTK)) {
        expect(CONSTTK);
        node->setTypeQualifier(TypeQualifier::CONST);
    } else if (match(STATICTK)) {
        expect(STATICTK);
        node->setStorageClass(StorageClass::STATIC);
    }

    node->setDeclType(parseBasicType());

    node->addDeclarator(parseVarDeclarator());
    while (match(COMMA)) {
        expect(COMMA);
        node->addDeclarator(parseVarDeclarator());
    }

    expect(SEMICN);

    return node;
}

std::unique_ptr<VariableDeclarator> Parser::parseVarDeclarator() {
    auto node = std::make_unique<VariableDeclarator>();

    node->setIdentifier(CURRENT_VALUE);
    expect(IDENFR);

    if (match(LBRACK)) {
        expect(LBRACK);
        node->addDimensionSize(parseCommaExp());
        expect(RBRACK);
        while (match(LBRACK)) {
            expect(LBRACK);
            if (match(RBRACK)) {
                std::cout << "Error: Expected dimension size after '['" << std::endl;
            }
            node->addDimensionSize(parseCommaExp());
            expect(RBRACK);
        }
    }

    if (match(ASSIGN)) {
        expect(ASSIGN);
        auto initials = parseInitVal();
        for (auto &it : initials) {
            node->addInitial(std::move(it));
        }
    }

    return node;
}

std::vector<std::unique_ptr<Exp>> Parser::parseInitVal() {
    std::vector<std::unique_ptr<Exp>> vals;
    if (match(LBRACE)) {
        expect(LBRACE);
        if (!match(RBRACE)) {
            vals.push_back(parseAssignExp());
            while (match(COMMA)) {
                expect(COMMA);
                if (match(RBRACE)) break;
                vals.push_back(parseAssignExp());
            }
        }
        expect(RBRACE);
    } else {
        vals.push_back(parseAssignExp());
    }
    return vals;
}


std::unique_ptr<FunctionDecl> Parser::parseFuncDecl() {
    auto node = std::make_unique<FunctionDecl>(CURRENT_LINE);

    node->setFuncType(parseBasicType());

    node->setIdentifier(CURRENT_VALUE);
    expect(IDENFR);

    expect(LPARENT);
    if (!match(RPARENT)) {
        auto FormatParams = parseFuncFParams();
        for (auto& fParam : FormatParams) {
            node->addFormatParam(std::move(fParam));
        }
    }
    expect(RPARENT);

    node->setFunctionBody(parseBlock());
    
    return node;
}

std::unique_ptr<FunctionDecl> Parser::parseMainFuncDecl() {
    auto node = std::make_unique<FunctionDecl>(CURRENT_LINE);

    node->setFuncType(BasicType::INT);
    expect(INTTK);

    node->setIdentifier("main");
    expect(MAINTK);

    expect(LPARENT);
    expect(RPARENT);

    node->setFunctionBody(parseBlock());

    return node;
}

std::vector<std::unique_ptr<FormatParam>> Parser::parseFuncFParams() {
    std::vector<std::unique_ptr<FormatParam>> params;
    params.push_back(parseFuncFParam());
    while (match(COMMA)) {
        expect(COMMA);
        params.push_back(parseFuncFParam());
    }
    
    return params;
}

std::unique_ptr<FormatParam> Parser::parseFuncFParam() {
    auto node = std::make_unique<FormatParam>();

    node->setParamType(parseBasicType());

    node->setIdentifier(CURRENT_VALUE);
    expect(IDENFR);
    if (match(LBRACK)) {
        expect(LBRACK);
        node->addDimensionSize(nullptr);
        parseCommaExp();
        expect(RBRACK);
        while (match(LBRACK)) {
            expect(LBRACK);
            if (match(RBRACK)) {
                std::cout << "Error: Expected dimension size after '['" << std::endl;
            }
            node->addDimensionSize(parseCommaExp());
            expect(RBRACK);
        }
    }

    return node;
}
std::vector<std::unique_ptr<Exp>> Parser::parseFuncRParams() {
    std::vector<std::unique_ptr<Exp>> params;
    params.push_back(parseAssignExp());
    while (match(COMMA)) {
        expect(COMMA);
        params.push_back(parseAssignExp());
    }
    return params;
}

// ----- 语句与语句块 -----
std::unique_ptr<BlockStmt> Parser::parseBlock() {
    auto node = std::make_unique<BlockStmt>(CURRENT_LINE);
    
    expect(LBRACE);
    
    while (!match(RBRACE)) {
        node->addStmt(parseStmt());
    }
    
    node->setEndLine(CURRENT_LINE);
    expect(RBRACE);
    
    return node;
}

std::unique_ptr<ExpStmt> Parser::parseExpStmt() {

    if (!match(SEMICN)) {
        auto node = std::make_unique<ExpStmt>(CURRENT_LINE);
        node->setExp(parseCommaExp());
        expect(SEMICN);
        return node;
    }
    expect(SEMICN);

    return nullptr;
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    auto node = std::make_unique<IfStmt>(CURRENT_LINE);

    expect(IFTK);
    expect(LPARENT);
    node->setCondition(parseCommaExp());
    expect(RPARENT);
    node->setTrueStmt(parseStmt());
    if (match(ELSETK)) {
        expect(ELSETK);
        node->setFalseStmt(parseStmt());
    }

    return node;
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    auto node = std::make_unique<ForStmt>(CURRENT_LINE);

    expect(FORTK);
    expect(LPARENT);
    if (!match(SEMICN)) {
        node->setInitial(parseCommaExp());
    }
    expect(SEMICN);
    if (!match(SEMICN)) {
        node->setCondition(parseCommaExp());
    } 
    expect(SEMICN);
    if (!match(SEMICN)) {
        node->setIteration(parseCommaExp());
    }
    expect(RPARENT);
    node->setBody(parseStmt());

    return node;
}

std::unique_ptr<BreakStmt> Parser::parseBreakStmt() {
    auto node = std::make_unique<BreakStmt>(CURRENT_LINE);

    expect(BREAKTK);
    expect(SEMICN);

    return node;
}

std::unique_ptr<ContinueStmt> Parser::parseContinueStmt() {
    auto node = std::make_unique<ContinueStmt>(CURRENT_LINE);

    expect(CONTINUETK);
    expect(SEMICN);

    return node;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    auto node = std::make_unique<ReturnStmt>(CURRENT_LINE);

    expect(RETURNTK);
    if (!match(SEMICN)) {
        node->setReturnExp(parseCommaExp());
    }
    expect(SEMICN);
    
    return node;
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    if (match(LBRACE)) {
        return parseBlock();
    } else if (match(INTTK) || match(STATICTK) || match(CONSTTK)) {
        return parseVarDecl();
    } else if (match(IFTK)) {
        return parseIfStmt();
    } else if (match(FORTK)) {
        return parseForStmt();
    } else if (match(BREAKTK) || match(CONTINUETK) || match(RETURNTK)) {
        if (match(BREAKTK) ) {
            return parseBreakStmt();
        } else if (match(CONTINUETK)) {
            return parseContinueStmt();
        } else  {  
            return parseReturnStmt();
        }
    } else {
        if (match(SEMICN)) {
            expect(SEMICN);
            return nullptr;
        } else {
            return parseExpStmt();
        }
    }
}

// ----- 表达式相关 -----
std::unique_ptr<Exp> Parser::parseCommaExp() {
    auto node = parseAssignExp();

    while (match(COMMA)) {
        expect(COMMA);
        auto rhs = parseAssignExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::COMMA, std::move(rhs));
    }

    return node;
}

std::unique_ptr<Exp> Parser::parseAssignExp() {
    auto left = parseLOrExp();

    if (match(ASSIGN)) {
        expect(ASSIGN);
        auto right = parseAssignExp();
        return std::make_unique<AssignExp>(std::move(left), AssignExp::AssignType::EQ, std::move(right));
    }

    return left;
}

std::unique_ptr<Exp> Parser::parseLOrExp() {
    auto node = parseLAndExp();
    while (match(OR)) {
        expect(OR);
        auto rhs = parseLAndExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::OR, std::move(rhs));
    }
    return node;
}

std::unique_ptr<Exp> Parser::parseLAndExp() {
    auto node = parseEqExp();
    while (match(AND)) {
        expect(AND);
        auto rhs = parseEqExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::AND, std::move(rhs));
    }
    return node;
}

std::unique_ptr<Exp> Parser::parseEqExp() {
    auto node = parseRelExp();
    while (match(EQL) || match(NEQ)) {
        if (match(EQL)) {
            expect(EQL);
            auto rhs = parseRelExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::EQL, std::move(rhs));
        } else if (match(NEQ)) {
            expect(NEQ);
            auto rhs = parseRelExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::NEQ, std::move(rhs));
        }
    }
    return node;
}

std::unique_ptr<Exp> Parser::parseRelExp() {
    auto node = parseAddExp();
    while (match(GRE) || match(GEQ) || match(LSS) || match(LEQ)) {
        if (match(GRE)) {
            expect(GRE);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::GRE, std::move(rhs));
        } else if (match(GEQ)) {
            expect(GEQ);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::GEQ, std::move(rhs));
        } else if (match(LSS)) {
            expect(LSS);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::LSS, std::move(rhs));
        } else if (match(LEQ)) {
            expect(LEQ);
            auto rhs = parseAddExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::LEQ, std::move(rhs));
        }
    }
    return node;
}

std::unique_ptr<Exp> Parser::parseAddExp() {
    auto node = parseMulExp();

    while (match(PLUS) || match(MINU)) {
        if (match(PLUS)) {
            expect(PLUS);
            auto rhs = parseMulExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::PLUS, std::move(rhs));
        } else if (match(MINU)) {
            expect(MINU);
            auto rhs = parseMulExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MINU, std::move(rhs));
        }
    }

    return node;
}

std::unique_ptr<Exp> Parser::parseMulExp() {
    auto node = parseUnaryExp();

    while (match(MULT) || match(DIV) || match(MOD)) {
        if (match(MULT)) {
            expect(MULT);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MULT, std::move(rhs));
        } else if (match(DIV)) {
            expect(DIV);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::DIV, std::move(rhs));
        } else if (match(MOD)) {
            expect(MOD);
            auto rhs = parseUnaryExp();
            node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::MOD, std::move(rhs));
        }
    }

    return node;
}

std::unique_ptr<Exp> Parser::parseUnaryExp() {
    if (match(PLUS)) {
        expect(PLUS);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::PLUS, parseUnaryExp());
    } else if (match(MINU)) {
        expect(MINU);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::MINU, parseUnaryExp());
    } else if (match(NOT)) {
        expect(NOT);
        return std::make_unique<UnaryExp>(UnaryExp::UnaryOp::NOT, parseUnaryExp());
    } else {
        return parsePostfixExp();
    }
}

std::unique_ptr<Exp> Parser::parsePostfixExp() {
    auto node = parsePrimaryExp();
    while (match(LBRACK) || match(LPARENT)) {
        if (match(LBRACK)) {
            expect(LBRACK);
            if (match(RBRACK)) {
                expect(RBRACK);
                std::cout << "Error: Expected expression inside '[]'" << std::endl;
                continue;
            }
            auto postfix = parseCommaExp();
            expect(RBRACK);
            node = std::make_unique<SubscriptExp>(std::move(node), std::move(postfix));
        } else {
            expect(LPARENT);
            if (match(RPARENT)) {
                expect(RPARENT);
                node = std::make_unique<FunctionCallExp>(std::move(node), std::vector<std::unique_ptr<Exp>>{});
                continue;
            }
            auto postfix = parseFuncRParams();
            expect(RPARENT);
            node = std::make_unique<FunctionCallExp>(std::move(node), std::move(postfix));
        }
    }
    return node;
}

std::unique_ptr<Exp> Parser::parsePrimaryExp() {
    if (match(LPARENT)) {
        expect(LPARENT);
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

std::unique_ptr<Exp> Parser::parseIdentifierExp() {
    auto idExp = std::make_unique<IdentifierExp>(CURRENT_VALUE);
    expect(IDENFR);
    return idExp;
}

std::unique_ptr<Exp> Parser::parseLiteralExp() {
    if (match(INTCON)) {
        int val = std::stoi(CURRENT_VALUE);
        expect(INTCON);
        return std::make_unique<LiteralExp>(val);
    } else if (match(STRCON)) {
        std::string val = CURRENT_VALUE;
        expect(STRCON);
        return std::make_unique<LiteralExp>(val);
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

        const char* basicTypeName(BasicType t) const {
            switch (t) {
                case BasicType::INT:  return "INT";
                case BasicType::VOID: return "VOID";
                default:              return "UNKNOWN";
            }
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
                case BinaryExp::BinaryOp::COMMA:  return ",";
                default:                           return "?";
            }
        }

        const char* assignOpName(AssignExp::AssignType op) const {
            switch (op) {
                case AssignExp::AssignType::EQ:     return "=";
                case AssignExp::AssignType::ADD_EQ: return "+=";
                case AssignExp::AssignType::SUB_EQ: return "-=";
                case AssignExp::AssignType::MUL_EQ: return "*=";
                case AssignExp::AssignType::DIV_EQ: return "/=";
                case AssignExp::AssignType::MOD_EQ: return "%=";
                default:                            return "?=";
            }
        }

        void printNode(const AstNode* node, int lvl) {
            if (!node) return;

            // ===== Expressions =====
            if (auto n = dynamic_cast<const LiteralExp*>(node)) {
                if (n->getLiteralType() == LiteralExp::LiteralType::STRING) {
                    ofs << indentFn(lvl) << "String: " << n->getStringValue() << "\n";
                } else {
                    ofs << indentFn(lvl) << "Integer: " << n->getIntValue() << "\n";
                }
                return;
            }
            if (auto id = dynamic_cast<const IdentifierExp*>(node)) {
                ofs << indentFn(lvl) << "Identifier: " << id->getIdentifier() << "\n";
                return;
            }
            if (auto fc = dynamic_cast<const FunctionCallExp*>(node)) {
                ofs << indentFn(lvl) << "FunctionCall:\n";
                ofs << indentFn(lvl+1) << "Function:\n";
                printNode(fc->getFunction(), lvl+2);
                ofs << indentFn(lvl+1) << "Params:\n";
                for (size_t i = 0; i < fc->getRealParamsCount(); ++i) {
                    printNode(fc->getRealParam(i), lvl+2);
                }
                return;
            }
            if (auto sub = dynamic_cast<const SubscriptExp*>(node)) {
                ofs << indentFn(lvl) << "Subscript:\n";
                ofs << indentFn(lvl+1) << "Array:\n";
                printNode(sub->getArray(), lvl+2);
                ofs << indentFn(lvl+1) << "Index:\n";
                printNode(sub->getSubscript(), lvl+2);
                return;
            }
            if (auto u = dynamic_cast<const UnaryExp*>(node)) {
                ofs << indentFn(lvl) << "UnaryExp: ";
                switch (u->getUnaryOp()) {
                    case UnaryExp::UnaryOp::PLUS: ofs << "+"; break;
                    case UnaryExp::UnaryOp::MINU: ofs << "-"; break;
                    case UnaryExp::UnaryOp::NOT:  ofs << "!"; break;
                }
                ofs << "\n";
                printNode(u->getExp(), lvl+1);
                return;
            }
            if (auto a = dynamic_cast<const AssignExp*>(node)) {
                ofs << indentFn(lvl) << "AssignExp: " << assignOpName(a->getAssignType()) << "\n";
                ofs << indentFn(lvl+1) << "LHS:\n";
                printNode(a->getLhs(), lvl+2);
                ofs << indentFn(lvl+1) << "RHS:\n";
                printNode(a->getRhs(), lvl+2);
                return;
            }
            if (auto b = dynamic_cast<const BinaryExp*>(node)) {
                ofs << indentFn(lvl) << "BinaryExp: " << binOpName(b->getBinaryOp()) << "\n";
                ofs << indentFn(lvl+1) << "LHS:\n";
                printNode(b->getLhs(), lvl+2);
                ofs << indentFn(lvl+1) << "RHS:\n";
                printNode(b->getRhs(), lvl+2);
                return;
            }

            // ===== Declarations / Params =====
            if (auto cu = dynamic_cast<const CompUnit*>(node)) {
                ofs << indentFn(lvl) << "CompUnit\n";
                ofs << indentFn(lvl+1) << "GlobalVarDecls:\n";
                for (size_t i = 0; i < cu->getGlobalVarDeclCount(); ++i) {
                    printNode(cu->getGlobalVarDecl(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "FuncDecls:\n";
                for (size_t i = 0; i < cu->getFuncDeclCount(); ++i) {
                    printNode(cu->getFuncDecl(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Main:\n";
                printNode(cu->getMainFuncDecl(), lvl+2);
                return;
            }
            if (auto vds = dynamic_cast<const VariableDecl*>(node)) {
                ofs << indentFn(lvl) << "VariableDecl (line " << vds->getLine() << ")\n";
                ofs << indentFn(lvl+1) << "static=" << (vds->getStorageClass() == StorageClass::STATIC ? 1 : 0)
                    << " const=" << (vds->getTypeQualifier() == TypeQualifier::CONST ? 1 : 0) << "\n";
                ofs << indentFn(lvl+1) << "Type: " << basicTypeName(vds->getDeclType()) << "\n";
                for (size_t i = 0; i < vds->getDeclaratorsCount(); ++i) {
                    printNode(vds->getDeclarator(i), lvl+1);
                }
                return;
            }
            if (auto vd = dynamic_cast<const VariableDeclarator*>(node)) {
                ofs << indentFn(lvl) << "VariableDeclarator: " << vd->getIdentifier() << "\n";
                ofs << indentFn(lvl+1) << "Dimensions: " << vd->getDimensionSizesCount() << "\n";
                if ((vd->getDimensionSizesCount()) > 0) {
                    ofs << indentFn(lvl+1) << "DimSizes:\n";
                    for (size_t i = 0; i < vd->getDimensionSizesCount(); ++i) {
                        printNode(vd->getDimensionSize(i), lvl+2);
                    }
                }
                ofs << indentFn(lvl+1) << "Initials:\n";
                for (size_t i = 0; i < vd->getInitialsCount(); ++i) {
                    printNode(vd->getInitial(i), lvl+2);
                }
                return;
            }
            if (auto fd = dynamic_cast<const FunctionDecl*>(node)) {
                ofs << indentFn(lvl) << "FunctionDecl (line " << fd->getLine() << "): " << fd->getIdentifier() << "\n";
                ofs << indentFn(lvl+1) << "ReturnType: " << basicTypeName(fd->getFuncType()) << "\n";
                ofs << indentFn(lvl+1) << "Params:\n";
                for (size_t i = 0; i < fd->getFormatParamsCount(); ++i) {
                    printNode(fd->getFormatParam(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Body:\n";
                printNode(fd->getFunctionBody(), lvl+2);
                return;
            }
            if (auto p = dynamic_cast<const FormatParam*>(node)) {
                ofs << indentFn(lvl) << "FormatParam: " << p->getIdentifier()
                    << " type=" << basicTypeName(p->getParamType())
                    << " dims=" << p->getDimensionSizesCount() << "\n";
                if (p->getDimensionSizesCount() > 0) {
                    ofs << indentFn(lvl+1) << "DimSizes:\n";
                    for (size_t i = 0; i < p->getDimensionSizesCount(); ++i) {
                        printNode(p->getDimensionSize(i), lvl+2);
                    }
                }
                return;
            }

            // ===== Statements =====
            if (auto bs = dynamic_cast<const BlockStmt*>(node)) {
                ofs << indentFn(lvl) << "BlockStmt (line " << bs->getLine()
                    << " - end " << bs->getEndLine() << ")\n";
                for (size_t i = 0; i < bs->getStmtsCount(); ++i) {
                    printNode(bs->getStmt(i), lvl+1);
                }
                return;
            }
            if (auto ifs = dynamic_cast<const IfStmt*>(node)) {
                ofs << indentFn(lvl) << "IfStmt (line " << ifs->getLine() << ")\n";
                ofs << indentFn(lvl+1) << "Cond:\n"; printNode(ifs->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Then:\n"; printNode(ifs->getTrueStmt(), lvl+2);
                ofs << indentFn(lvl+1) << "Else:\n"; printNode(ifs->getFalseStmt(), lvl+2);
                return;
            }
            if (auto fs = dynamic_cast<const ForStmt*>(node)) {
                ofs << indentFn(lvl) << "ForStmt (line " << fs->getLine() << ")\n";
                ofs << indentFn(lvl+1) << "Initial:\n";   printNode(fs->getInitial(),   lvl+2);
                ofs << indentFn(lvl+1) << "Condition:\n"; printNode(fs->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Iteration:\n"; printNode(fs->getIteration(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n";      printNode(fs->getBody(),      lvl+2);
                return;
            }
            if (auto rs = dynamic_cast<const ReturnStmt*>(node)) {
                ofs << indentFn(lvl) << "ReturnStmt (line " << rs->getLine() << ")\n";
                if (rs->getReturnExp()) {
                    ofs << indentFn(lvl+1) << "ReturnExp:\n";
                    printNode(rs->getReturnExp(), lvl+2);
                }
                return;
            }
            if (auto br = dynamic_cast<const BreakStmt*>(node)) {
                ofs << indentFn(lvl) << "BreakStmt (line " << br->getLine() << ")\n";
                return;
            }
            if (auto ct = dynamic_cast<const ContinueStmt*>(node)) {
                ofs << indentFn(lvl) << "ContinueStmt (line " << ct->getLine() << ")\n";
                return;
            }
            if (auto es = dynamic_cast<const ExpStmt*>(node)) {
                ofs << indentFn(lvl) << "ExpStmt (line " << es->getLine() << ")\n";
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