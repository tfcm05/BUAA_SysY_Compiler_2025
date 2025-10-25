#include<iostream>
#include"../../include/front/GrammaticalAnalysis.h"
#include"../../include/front/Token.h"
#include"../../include/front/LexicalAnalysis.h"
#include"../../include/Error.h"
#include <fstream>
#include <sstream>

Parse::Parse(LexicalAnalysis& lex): lexicalAnalysis(lex) {
    initial();
}

std::unique_ptr<AstNode> Parse::parse() {
    return parseCompUnit();
}

// ----- Top-level: compilation unit -----
std::unique_ptr<CompUnit> Parse::parseCompUnit() {
    auto node = std::make_unique<CompUnit>();

    while (match(CONSTTK) || (match(INTTK) && match(IDENFR, 1) && !match(LPARENT, 2))) {
        node->addGlobalVarDefStmt(parseVarDefStmt());
    }

    while (match(VOIDTK) || (match(INTTK) && match(IDENFR, 1))) {
        node->addFuncDefStmt(parseFuncDefStmt());
    }

    node->setMainFuncDef(parseMainFuncDef());

    return node;
}

// ----- Initialization & token helpers -----
void Parse::initial() {
    preLine = 0;
    currentToken = lexicalAnalysis.getNextToken();
    lexerLog(currentToken);
}

void Parse::advance() {
    preLine = currentToken.getLine();
    currentToken = lexicalAnalysis.getNextToken();
    lexerLog(currentToken);
}

Token Parse::peekNext() {
    return lexicalAnalysis.peekNextToken();
}

Token Parse::peekSecond() {
    return lexicalAnalysis.peekSecondToken();
}

const Token& Parse::peekNextRef() {
    return lexicalAnalysis.peekNextTokenRef();
}

const Token& Parse::peekSecondRef() {
    return lexicalAnalysis.peekSecondTokenRef();
}

bool Parse::match(TokenType expected, int mode) {
    switch (mode) {
        case 0 : return CURRENT_TYPE == expected;
        case 1 : return NEXT_TYPE == expected;
        case 2 : return SECOND_TYPE == expected;
        default : return -1;
    }
}

bool Parse::match(TokenType expected) {
    return CURRENT_TYPE == expected;
}

void Parse::expect(TokenType expect) {
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

// ----- 变量和定义相关 -----
std::unique_ptr<VariableDefStmt> Parse::parseVarDefStmt() {
    auto node = std::make_unique<VariableDefStmt>(CURRENT_LINE);

    if (match(CONSTTK)) {
        expect(CONSTTK);
        node->setConstFlag(true);
    } else if (match(STATICTK)) {
        expect(STATICTK);
        node->setStaticFlag(true);
    }

    // BType
    node->setVarType(BasicType::INT);
    expect(INTTK);

    node->addDef(parseVarDef());
    while (match(COMMA)) {
        expect(COMMA);
        node->addDef(parseVarDef());
    }

    expect(SEMICN);

    return node;
}

std::unique_ptr<VariableDef> Parse::parseVarDef() {
    auto node = std::make_unique<VariableDef>(CURRENT_VALUE);
    expect(IDENFR);

    if (match(LBRACK)) {
        expect(LBRACK);
        node->setIsArray(true);
        node->setArraySize(parseAddExp());
        expect(RBRACK);
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

std::vector<std::unique_ptr<Exp>> Parse::parseInitVal() {
    std::vector<std::unique_ptr<Exp>> vals;
    if (match(LBRACE)) {
        expect(LBRACE);
        if (!match(RBRACE)) {
            vals.push_back(parseExpIgnoreComma());
            while (match(COMMA)) {
                expect(COMMA);
                if (match(RBRACE)) break;
                vals.push_back(parseExpIgnoreComma());
            }
        }
        expect(RBRACE);
    } else {
        vals.push_back(parseExp());
    }
    return vals;
}

// ----- 函数相关 -----
std::unique_ptr<FunctionDefStmt> Parse::parseFuncDefStmt() {
    // funcType
    auto node = std::make_unique<FunctionDefStmt>(CURRENT_LINE, match(VOIDTK) ? BasicType::VOID : BasicType::INT);
    expect(node->getFuncType() == BasicType::VOID ? VOIDTK : INTTK);

    node->setIdentifier(CURRENT_VALUE);
    expect(IDENFR);

    expect(LPARENT);
    if (!match(RPARENT)) {
        node->setFormatParams(parseFuncFParams());
    }
    expect(RPARENT);

    node->setStmt(parseBlock());

    return node;
}

std::unique_ptr<FunctionDefStmt> Parse::parseMainFuncDef() {
    auto node = std::make_unique<FunctionDefStmt>(CURRENT_LINE, BasicType::INT);
    expect(INTTK);

    node->setIdentifier("main");
    expect(MAINTK);

    node->setFormatParams({});
    expect(LPARENT);
    expect(RPARENT);

    node->setStmt(parseBlock());

    return node;
}

std::vector<std::unique_ptr<Param>> Parse::parseFuncFParams() {
    std::vector<std::unique_ptr<Param>> params;
    params.push_back(parseFuncFParam());
    while (match(COMMA)) {
        expect(COMMA);
        params.push_back(parseFuncFParam());
    }
    return params;
}

std::unique_ptr<Param> Parse::parseFuncFParam() {
    expect(INTTK);
    std::string id = CURRENT_VALUE;
    expect(IDENFR);
    bool isArr = false;
    if (match(LBRACK)) {
        expect(LBRACK);
        expect(RBRACK);
        isArr = true;
    }
    return std::make_unique<Param>(id, BasicType::INT, isArr);
}

std::vector<std::unique_ptr<Exp>> Parse::parseFuncRParams() {
    std::vector<std::unique_ptr<Exp>> params;
    params.push_back(parseExpIgnoreComma());
    while (match(COMMA)) {
        expect(COMMA);
        params.push_back(parseExpIgnoreComma());
    }
    return params;
}

// ----- 语句与语句块 -----
std::unique_ptr<BlockStmt> Parse::parseBlock() {
    auto node = std::make_unique<BlockStmt>(CURRENT_LINE);
    
    expect(LBRACE);
    
    while (!match(RBRACE)) {
        node->addStmt(parseStmt());
    }
    
    node->setEndLine(CURRENT_LINE);
    expect(RBRACE);
    
    return node;
}

std::unique_ptr<ExpStmt> Parse::parseExpStmt() {

    if (!match(SEMICN)) {
        auto node = std::make_unique<ExpStmt>(CURRENT_LINE, parseExp());
        expect(SEMICN);
        return node;
    }
    expect(SEMICN);

    return nullptr;
}

std::unique_ptr<IfStmt> Parse::parseIfStmt() {
    auto node = std::make_unique<IfStmt>(CURRENT_LINE);

    expect(IFTK);
    expect(LPARENT);
    node->setCondition(parseExp());
    expect(RPARENT);
    node->setTrueStmt(parseStmt());
    if (match(ELSETK)) {
        expect(ELSETK);
        node->setFalseStmt(parseStmt());
    }

    return node;
}

std::unique_ptr<ForStmt> Parse::parseForStmt() {
    auto node = std::make_unique<ForStmt>(CURRENT_LINE);

    expect(FORTK);
    expect(LPARENT);
    if (!match(SEMICN)) {
        if (match(CONSTTK) || match(STATICTK) || match(INTTK)) {
            node->setInitial(parseVarDefStmt());
        } else {
            node->setInitial(parseExpStmt());
        }
    }
    if (!match(SEMICN)) {
        node->setCondition(parseExp());
    } 
    expect(SEMICN);
    if (!match(SEMICN)) {
        node->setIteration(parseExp());
    }
    expect(RPARENT);
    node->setStmt(parseStmt());

    return node;
}

std::unique_ptr<JumpStmt> Parse::parseJumpStmt() {
    auto node = std::make_unique<JumpStmt>(CURRENT_LINE,
        match(RETURNTK) ? JumpStmt::JumpStmtType::RETURN :
        (match(BREAKTK) ? JumpStmt::JumpStmtType::BREAK :
        JumpStmt::JumpStmtType::CONTINUE));
    
    if (match(RETURNTK)) {
        expect(RETURNTK);
        if (!match(SEMICN)) {
            node->setReturnExp(parseExp());
        }
    } else if (match(BREAKTK)) {
        expect(BREAKTK);
    } else if (match(CONTINUETK)) {
        expect(CONTINUETK);
    } else {
        // 错误处理
        return nullptr;
    }
    expect(SEMICN);

    return node;
}

std::unique_ptr<Stmt> Parse::parseStmt() {
    if (match(LBRACE)) {
        return parseBlock();
    } else if (match(CONSTTK) || match(STATICTK) || match(INTTK)) {
        return parseVarDefStmt();
    } else if (match(IFTK)) {
        return parseIfStmt();
    } else if (match(FORTK)) {
        return parseForStmt();
    } else if (match(BREAKTK) || match(CONTINUETK) || match(RETURNTK)) {
        return parseJumpStmt();
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
std::unique_ptr<Exp> Parse::parseExp() {
    return parseCommaExp();
}

std::unique_ptr<Exp> Parse::parseCommaExp() {
    auto node = parseAssignExp();

    while (match(COMMA)) {
        expect(COMMA);
        auto rhs = parseAssignExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::COMMA, std::move(rhs));
    }

    return node;
}

std::unique_ptr<Exp> Parse::parseExpIgnoreComma() {
    return parseAssignExp();
}

std::unique_ptr<Exp> Parse::parseAssignExp() {
    auto left = parseLOrExp();

    if (match(ASSIGN)) {
        expect(ASSIGN);
        auto right = parseAssignExp();
        auto node = std::make_unique<BinaryExp>(std::move(left), BinaryExp::BinaryOp::ASSIGN, std::move(right));
        return node;
    }

    return left;
}

std::unique_ptr<Exp> Parse::parseLOrExp() {
    auto node = parseLAndExp();
    while (match(OR)) {
        expect(OR);
        auto rhs = parseLAndExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::OR, std::move(rhs));
    }
    return node;
}

std::unique_ptr<Exp> Parse::parseLAndExp() {
    auto node = parseEqExp();
    while (match(AND)) {
        expect(AND);
        auto rhs = parseEqExp();
        node = std::make_unique<BinaryExp>(std::move(node), BinaryExp::BinaryOp::AND, std::move(rhs));
    }
    return node;
}

std::unique_ptr<Exp> Parse::parseEqExp() {
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

std::unique_ptr<Exp> Parse::parseRelExp() {
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

std::unique_ptr<Exp> Parse::parseAddExp() {
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

std::unique_ptr<Exp> Parse::parseMulExp() {
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

std::unique_ptr<Exp> Parse::parseUnaryExp() {
    if (match(PLUS)) {
        expect(PLUS);
        return std::make_unique<UnaryExp>(UnaryExp::Op::PLUS, parseUnaryExp());
    } else if (match(MINU)) {
        expect(MINU);
        return std::make_unique<UnaryExp>(UnaryExp::Op::MINU, parseUnaryExp());
    } else if (match(NOT)) {
        expect(NOT);
        return std::make_unique<UnaryExp>(UnaryExp::Op::NOT, parseUnaryExp());
    } else {
        return parsePrimaryExp();
    }
}

std::unique_ptr<Exp> Parse::parsePrimaryExp() {
    if (match(LPARENT)) {
        expect(LPARENT);
        auto e = parseExp();
        expect(RPARENT);
        return e;
    } else if (match(INTCON) || match(STRCON)) {
        return parseLiteralExp();
    } else if (match(IDENFR)) {
        std::string id = CURRENT_VALUE;
        expect(IDENFR);

        // function call
        if (match(LPARENT)) {
            expect(LPARENT);
            std::vector<std::unique_ptr<Exp>> params;
            if (!match(RPARENT)) {
                params = parseFuncRParams();
            }
            expect(RPARENT);
            auto func = std::make_unique<IdentifierExp>(id);
            return std::make_unique<FunctionCallExp>(std::move(func), std::move(params));
        }

        // array subscript
        if (match(LBRACK)) {
            expect(LBRACK);
            auto idx = parseExp();
            expect(RBRACK);
            auto base = std::make_unique<IdentifierExp>(id);
            return std::make_unique<SubscriptExp>(std::move(base), std::move(idx));
        }

        return std::make_unique<IdentifierExp>(id);
    }

    return nullptr;
}

std::unique_ptr<Exp> Parse::parseLiteralExp() {
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
void Parse::parserLog(const AstNode* root) const {
    std::ofstream ofs("parser.txt");
    if (!ofs.is_open()) return;

    auto indent = [](int lvl) {
        return std::string(lvl * 2, ' ');
    };

    struct Printer {
        std::ofstream& ofs;
        decltype(indent)& indentFn;

        void printNode(const AstNode* node, int lvl) {
            if (!node) {
                ofs << indentFn(lvl) << "null\n";
                return;
            }

            // Expressions
            if (auto n = dynamic_cast<const LiteralExp*>(node)) {
                if (n->isString()) {
                    ofs << indentFn(lvl) << "String: " << n->getStringValue() << "\n";
                    return;
                } else {
                    ofs << indentFn(lvl) << "Integer: " << n->getIntValue() << "\n";
                    return;
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
                switch (u->getOp()) {
                    case UnaryExp::Op::PLUS: ofs << "+"; break;
                    case UnaryExp::Op::MINU: ofs << "-"; break;
                    case UnaryExp::Op::NOT:  ofs << "!"; break;
                }
                ofs << "\n";
                printNode(u->getExp(), lvl+1);
                return;
            }
            if (auto b = dynamic_cast<const BinaryExp*>(node)) {
                ofs << indentFn(lvl) << "BinaryExp: ";
                switch (b->getOp()) {
                    case BinaryExp::BinaryOp::PLUS:  ofs << "+"; break;
                    case BinaryExp::BinaryOp::MINU:  ofs << "-"; break;
                    case BinaryExp::BinaryOp::MULT:  ofs << "*"; break;
                    case BinaryExp::BinaryOp::DIV:   ofs << "/"; break;
                    case BinaryExp::BinaryOp::MOD:   ofs << "%"; break;
                    case BinaryExp::BinaryOp::ASSIGN:ofs << "="; break;
                    case BinaryExp::BinaryOp::OR:    ofs << "||"; break;
                    case BinaryExp::BinaryOp::AND:   ofs << "&&"; break;
                    case BinaryExp::BinaryOp::EQL:   ofs << "=="; break;
                    case BinaryExp::BinaryOp::NEQ:   ofs << "!="; break;
                    case BinaryExp::BinaryOp::GRE:   ofs << ">"; break;
                    case BinaryExp::BinaryOp::GEQ:   ofs << ">="; break;
                    case BinaryExp::BinaryOp::LSS:   ofs << "<"; break;
                    case BinaryExp::BinaryOp::LEQ:   ofs << "<="; break;
                    case BinaryExp::BinaryOp::COMMA: ofs << ","; break;
                    default: ofs << "?"; break;
                }
                ofs << "\n";
                ofs << indentFn(lvl+1) << "LHS:\n";
                printNode(b->getLhs(), lvl+2);
                ofs << indentFn(lvl+1) << "RHS:\n";
                printNode(b->getRhs(), lvl+2);
                return;
            }

            // Statements and types
            if (auto cu = dynamic_cast<const CompUnit*>(node)) {
                ofs << indentFn(lvl) << "CompUnit\n";
                ofs << indentFn(lvl+1) << "GlobalVarDefs:\n";
                for (size_t i = 0; i < cu->getGlobalVarDefStmtsCount(); ++i) {
                    printNode(cu->getGlobalVarDefStmt(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "FuncDefs:\n";
                for (size_t i = 0; i < cu->getFuncDefStmtsCount(); ++i) {
                    printNode(cu->getFuncDefStmt(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Main:\n";
                printNode(cu->getMainFuncDef(), lvl+2);
                return;
            }
            if (auto vds = dynamic_cast<const VariableDefStmt*>(node)) {
                ofs << indentFn(lvl) << "VariableDefStmt (line " << vds->getLine() << ")\n";
                bool s = vds->isStatic(), c = vds->isConst();
                ofs << indentFn(lvl+1) << "static=" << s << " const=" << c << "\n";
                ofs << indentFn(lvl+1) << "Type: INT\n";
                for (size_t i = 0; i < vds->getDefsCount(); ++i) {
                    printNode(vds->getDef(i), lvl+1);
                }
                return;
            }
            if (auto vd = dynamic_cast<const VariableDef*>(node)) {
                ofs << indentFn(lvl) << "VariableDef: " << vd->getIdentifier() << "\n";
                ofs << indentFn(lvl+1) << "isArray=" << (vd->isArray() ? 1 : 0) << "\n";
                if (vd->getArraySize()) {
                    ofs << indentFn(lvl+1) << "ArraySize:\n";
                    printNode(vd->getArraySize(), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Initials:\n";
                for (size_t i = 0; i < vd->getInitialCount(); ++i) {
                    printNode(vd->getInitial(i), lvl+2);
                }
                return;
            }
            if (auto fd = dynamic_cast<const FunctionDefStmt*>(node)) {
                ofs << indentFn(lvl) << "FunctionDef (line " << fd->getLine() << "): " << fd->getIdentifier() << "\n";
                ofs << indentFn(lvl+1) << "ReturnType: " << (fd->getFuncType() == BasicType::INT ? "INT" : "VOID") << "\n";
                ofs << indentFn(lvl+1) << "Params:\n";
                for (size_t i = 0; i < fd->getFormatParamsCount(); ++i) {
                    printNode(fd->getFormatParam(i), lvl+2);
                }
                ofs << indentFn(lvl+1) << "Body:\n";
                printNode(fd->getStmt(), lvl+2);
                return;
            }
            if (auto p = dynamic_cast<const Param*>(node)) {
                ofs << indentFn(lvl) << "Param: " << p->getIdentifier() << " type=" << (p->getParamType()==BasicType::INT?"INT":"VOID")
                    << " isArray=" << (p->isArray()?1:0) << "\n";
                return;
            }
            if (auto bs = dynamic_cast<const BlockStmt*>(node)) {
                ofs << indentFn(lvl) << "BlockStmt (line " << bs->getLine() << " - end " << bs->getEndLine() << ")\n";
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
                ofs << indentFn(lvl+1) << "Initials:\n"; printNode(fs->getInitial(), lvl+2);
                ofs << indentFn(lvl+1) << "Condition:\n"; printNode(fs->getCondition(), lvl+2);
                ofs << indentFn(lvl+1) << "Iterations:\n";printNode(fs->getIteration(), lvl+2);
                ofs << indentFn(lvl+1) << "Body:\n"; printNode(fs->getStmt(), lvl+2);
                return;
            }
            if (auto js = dynamic_cast<const JumpStmt*>(node)) {
                ofs << indentFn(lvl) << "JumpStmt (line " << js->getLine() << ") type=";
                switch (js->getJumpStmtType()) {
                    case JumpStmt::JumpStmtType::RETURN: ofs << "RETURN"; break;
                    case JumpStmt::JumpStmtType::BREAK:  ofs << "BREAK"; break;
                    case JumpStmt::JumpStmtType::CONTINUE: ofs << "CONTINUE"; break;
                }
                ofs << "\n";
                if (js->getReturnExp()) {
                    ofs << indentFn(lvl+1) << "ReturnExp:\n";
                    printNode(js->getReturnExp(), lvl+2);
                }
                return;
            }
            if (auto es = dynamic_cast<const ExpStmt*>(node)) {
                ofs << indentFn(lvl) << "ExpStmt (line " << es->getLine() << ")\n";
                printNode(es->getExp(), lvl+1);
                return;
            }

            // fallback
            ofs << indentFn(lvl) << "UnknownNode (type info)\n";
        }
    };

    Printer p{ofs, const_cast<decltype(indent)&>(indent)}; // indent is stateless, safe to cast
    p.printNode(root, 0);

    ofs.close();
}