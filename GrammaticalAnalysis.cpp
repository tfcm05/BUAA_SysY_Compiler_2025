#include"include/GrammaticalAnalysis.h"
#include"include/Token.h"
#include"include/LexicalAnalysis.h"
#include"include/Error.h"

Parse::Parse(LexicalAnalysis& lex): lexicalAnalysis(lex) {
    initial();
}

std::unique_ptr<AstNode> Parse::parse() {
    return parseCompUnit();
}

std::unique_ptr<AstNode> Parse::parseCompUnit() {
    auto node = std::make_unique<AstNode>("<CompUnit>");

    while (match(CONSTTK) || (match(CONSTTK) && match(IDENFR, 1) && !match(LPARENT, 2))) {
        node->addNext(parseDecl());
    }

    while (match(VOIDTK) || (match(INTTK) && match(IDENFR, 1))) {
        node->addNext(parseFuncDef());
    }

    node->addNext(parseMainFuncDef());
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseDecl() {
    auto node = std::make_unique<AstNode>("<Decal>");

    if (match(CONSTTK)) {
        node->addNext(parseConstDecl());
    } else {
        node->addNext(parseVarDecl());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseConstDecl() {
    auto node = std::make_unique<AstNode>("<ConstDecl>");

    expect(CONSTTK, node);

    node->addNext(parseBType());

    node->addNext(parseConstDef());
    while (match(COMMA)) {
        expect(COMMA, node);
        node->addNext(parseConstDef());
    }

    expect(SEMICN, node);

    return node;
}

std::unique_ptr<AstNode> Parse::parseVarDecl() {
    auto node = std::make_unique<AstNode>("<VarDecl>");

    if (match(STATICTK)) {
        expect(STATICTK, node);
    }

    node->addNext(parseBType());

    node->addNext(parseVarDef());
    while (match(COMMA)) {
        expect(COMMA, node);
        node->addNext(parseVarDef());
    }

    expect(SEMICN, node);

    return node;
}

std::unique_ptr<AstNode> Parse::parseBType() {
    auto node = std::make_unique<AstNode>("<BType>");
    expect(INTTK, node);

    return node;
}

std::unique_ptr<AstNode> Parse::parseConstDef() {
    auto node = std::make_unique<AstNode>("<ConstDef>");

    expect(IDENFR, node);

    if (match(LBRACK)) {
        expect(LBRACK, node);
        node->addNext(parseConstExp());
        expect(RBRACK, node);
    }

    expect(ASSIGN, node);
    node->addNext(parseConstInitVal());

    return node;
}

std::unique_ptr<AstNode> Parse::parseVarDef() {
    auto node = std::make_unique<AstNode>("<VarDef>");

    expect(IDENFR, node);

    if (match(LBRACK)) {
        expect(LBRACK, node);
        node->addNext(parseConstExp());
        expect(RBRACK, node);
    }

    if (match(ASSIGN)) {
        expect(ASSIGN, node);
        node->addNext(parseConstInitVal());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseFuncDef() {
    auto node = std::make_unique<AstNode>("<FuncDef>");


    node->addNext(parseFuncType());
    expect(IDENFR, node);

    expect(LPARENT, node);

    if (!match(RPARENT)) {
        node->addNext(parseFuncFParams());
    }

    expect(RPARENT, node);
    node->addNext(parseBlock());

    return node;
}

std::unique_ptr<AstNode> Parse::parseMainFuncDef() {
    auto node = std::make_unique<AstNode>("<MainFuncDef>");

    expect(INTTK, node);

    expect(MAINTK, node);

    expect(LPARENT, node);
    expect(RPARENT, node);

    node->addNext(parseBlock());

    return node;
}

std::unique_ptr<AstNode> Parse::parseFuncType() {
    auto node = std::make_unique<AstNode>("<FuncType>");

    if (match(VOIDTK)) {
        expect(VOIDTK, node);
    } else if (match(INTTK)) {
        expect(INTTK, node);
    } 

    return node;
}

std::unique_ptr<AstNode> Parse::parseBlock() {
    auto node = std::make_unique<AstNode>("<Block>");
    
    expect(LBRACE, node);
    
    while (!match(RBRACE)) {
        node->addNext(parseBlockItem());
    }
    
    expect(RBRACE, node);
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseBlockItem() {
    auto node = std::make_unique<AstNode>("<BlockItem>");

    if (match(CONSTTK) || (match(INTTK) && match(IDENFR, 1) && !match(RPARENT, 2))) {
        node->addNext(parseDecl());
    } else {
        node->addNext(parseStmt());
    }
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseStmt() {
    auto node = std::make_unique<AstNode>("<Stmt>");

    if (match(IDENFR)) {
        node->addNext(parseLVal());

        expect(ASSIGN, node);

        node->addNext(parseExp());

        expect(SEMICN, node);
    }  else if (match(LBRACE)) {
        expect(LBRACE, node);
        node->addNext(parseBlock());
    } else if (match(IFTK)) {
        expect(LPARENT, node);
        node->addNext(parseCond());
        expect(RPARENT, node);
        node->addNext(parseStmt());
        if (match(ELSETK)) {
            expect(ELSETK, node);
            node->addNext(parseStmt());
        }
    } else if (match(FORTK)) {
        expect(FORTK, node);
        expect(LPARENT, node);
        if (!match(SEMICN)) {
            node->addNext(parseForStmt());
        }
        expect(SEMICN, node);
        if (!match(SEMICN)) {
            node->addNext(parseCond());
        }
        expect(SEMICN, node);
        if (!match(RPARENT)) {
            node->addNext(parseForStmt());
        }
        expect(RPARENT, node);
        node->addNext(parseStmt());
    } else if (match(BREAKTK)) {
        expect(BREAKTK, node);
        expect(SEMICN, node);
    } else if (match(CONTINUETK)) {
        expect(CONTINUETK, node);
        expect(SEMICN, node);
    } else if (match(RETURNTK)) {
        expect(RETURNTK, node);
        if (!match(SEMICN)) {
            node->addNext(parseExp());
        }
        expect(SEMICN, node);
    } else if (match(PRINTFTK)) {
        expect(PRINTFTK, node);
        expect(LPARENT, node);
        expect(STRCON, node);
        while (match(COMMA)) {
            expect(COMMA, node);
            node->addNext(parseExp());
        }
        expect(RPARENT, node);
        expect(SEMICN, node);
    } else {
        if (!match(SEMICN)) {
            node->addNext(parseExp());
        }
        expect(SEMICN, node);
    }
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseConstExp() {
    auto node = std::make_unique<AstNode>("<ConstExp>");

    node->addNext(parseAddExp());

    return node;
}

std::unique_ptr<AstNode> Parse::parseConstInitVal() {
    auto node = std::make_unique<AstNode>("<ConstInitVal>");
    
    if (match(LBRACE)) {
        expect(LBRACE, node);
        
        if (!match(RBRACE)) {
            node->addNext(parseConstExp());
            
            while (match(COMMA)) {
                expect(COMMA, node);
                node->addNext(parseConstExp());
            }
        }
        
        expect(RBRACE, node);
    } else {
        node->addNext(parseConstExp());
    }
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseInitVal() {
    auto node = std::make_unique<AstNode>("<InitVal>");
    
    if (match(LBRACE)) {
        expect(LBRACE, node);
        
        if (!match(RBRACE)) {
            node->addNext(parseExp());
            
            while (match(COMMA)) {
                expect(COMMA, node);
                node->addNext(parseExp());
            }
        }
        
        expect(RBRACE, node);
    } else {
        node->addNext(parseExp());
    }
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseExp() {
    auto node = std::make_unique<AstNode>("<Exp>");

    node->addNext(parseAddExp());

    return node;
}

std::unique_ptr<AstNode> Parse::parseAddExp() {
    auto node = std::make_unique<AstNode>("<AddExp>");

    node->addNext(parseMulExp());

    while (match(PLUS) || match(MINU)) {
        if (match(PLUS)) {
            expect(PLUS, node);
        } else {
            expect(MINU, node);
        }

        node->addNext(parseMulExp());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseMulExp() {
    auto node = std::make_unique<AstNode>("<MulExp>");

    node->addNext(parseUnaryExp());

    while (match(MULT) || match(DIV) || match(MOD)) {
        if (match(MULT)) {
            expect(MULT, node);
        } else if (match(DIV)) {
            expect(DIV, node);
        } else {
            expect(MOD, node);
        }

        node->addNext(parseUnaryExp());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseUnaryExp() {
    auto node = std::make_unique<AstNode>("<UnaryExp>");

    if (match(IDENFR)) {
        expect(IDENFR, node);

        expect(LPARENT, node);
        if (!match(RPARENT)) {
            node->addNext(parseFuncRParams());
        }
        expect(RPARENT, node);
    } else if (match(PLUS) || match(MINU) || match(NOT)) {
        if (match(PLUS)) {
            expect(PLUS, node);
        } else if (match(MINU)) {
            expect(MINU, node);
        } else {
            expect(NOT, node);
        }
        node->addNext(parseUnaryExp());
    } else {
        node->addNext(parsePrimaryExp());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parsePrimaryExp() {
    auto node = std::make_unique<AstNode>("<PrimaryExp>");

    if (match(LPARENT)) {
        expect(LPARENT, node);
        node->addNext(parseExp());
        expect(RPARENT, node);
    } else if (match(IDENFR)) {
        node->addNext(parseLVal());
    } else {
        expect(INTCON, node);
    }
    
    return node;
}

std::unique_ptr<AstNode> Parse::parseLVal() {
    auto node = std::make_unique<AstNode>("<LVal>");

    expect(IDENFR, node);

    if (match(LBRACK)) {
        expect(LBRACK, node);
        node->addNext(parseExp());
        expect(RBRACK, node);
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseFuncFParams() {
    auto node = std::make_unique<AstNode>("<FuncFParams>");

    node->addNext(parseFuncFParam());

    while (match(COMMA)) {
        expect(COMMA, node);
        node->addNext(parseFuncFParam());
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseFuncFParam() {
    auto node = std::make_unique<AstNode>("<FuncFParam>");

    node->addNext(parseBType());
    expect(IDENFR, node);

    if (match(LBRACK)) {
        expect(LBRACK, node);
        expect(RBRACK, node);
    }

    return node;
}

std::unique_ptr<AstNode> Parse::parseFuncRParams() {
    auto node = std::make_unique<AstNode>("<FuncRParams>");
    return node;
}

std::unique_ptr<AstNode> Parse::parseCond() {
    auto node = std::make_unique<AstNode>("<Cond>");
    return node;
}

std::unique_ptr<AstNode> Parse::parseForStmt() {
    auto node = std::make_unique<AstNode>("<ForStmt>");
    return node;
}

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

void Parse::expect(TokenType expect, std::unique_ptr<AstNode>& node) {
    if (!match(expect)) {
        // 错误处理
        if (expect == SEMICN) {
            errorLog(ErrorCode::i, preLine);
        } else if (expect == RPARENT) {
            errorLog(ErrorCode::j, preLine);
        } else if (expect == RBRACK) {
            errorLog(ErrorCode::k, preLine);
        }
    }

    node->addNext(std::make_unique<AstNode>(getTypeCode(CURRENT_TYPE) + " " + currentToken.getValue()));

    advance();
}