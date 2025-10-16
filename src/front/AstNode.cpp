#include<fstream>
#include"../../include/front/AstNode.h"

AstNode::AstNode(std::string name) : name(name), pre(nullptr) {}

TokenAstNode::TokenAstNode(std::string name, std::string value) : AstNode(name), value(value) {}

void AstNode::addNext(std::unique_ptr<AstNode> nextNode) {
    if (nextNode) {
        nextNode->pre = this;
        next.push_back(std::move(nextNode));
    }
}

void AstNode::print() {
    std::ofstream parser("parser.txt", std::ios::app);
    
    for (auto& child : next) {
        TokenAstNode* tokenNode = dynamic_cast<TokenAstNode*>(child.get());
        if (tokenNode) {
            tokenNode->print();
        } else {
            child->print();
        }
    }

    if (name != "<MulExp>" && name != "<AddExp>" && name != "<RelExp>" && name != "<EqExp>" && name != "<LAndExp>" && name != "<LOrExp>") {
        parser << name << std::endl;
    }

    if (name == "<UnaryExp>" && pre && pre->name == "<MulExp>") {
        parser << "<MulExp>" << std::endl;
    } else if (name == "<MulExp>" && pre && pre->name == "<AddExp>") {
        parser << "<AddExp>" << std::endl;
    } else if (name == "<AddExp>" && pre && pre->name == "<RelExp>") {
        parser << "<RelExp>" << std::endl;
    } else if (name == "<RelExp>" && pre && pre->name == "<EqExp>") {
        parser << "<EqExp>" << std::endl;
    } else if (name == "<EqExp>" && pre && pre->name == "<LAndExp>") {
        parser << "<LAndExp>" << std::endl;
    } else if (name == "<LAndExp>" && pre && pre->name == "<LOrExp>") {
        parser << "<LOrExp>" << std::endl;
    }
}

void TokenAstNode::print() {
    std::ofstream parser("parser.txt", std::ios::app);
    parser << name << " " << value << std::endl;
}