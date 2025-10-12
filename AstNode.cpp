#include<fstream>
#include"include/AstNode.h"

AstNode::AstNode(std::string nodeType) : nodeType(nodeType), pre(nullptr) {}

void AstNode::addNext(std::unique_ptr<AstNode> nextNode) {
    if (nextNode) {
        nextNode->pre = this;
        next.push_back(std::move(nextNode));
    }
}

void AstNode::print() {
    std::ofstream parser("parser.txt", std::ios::app);
    
    for (auto& child : next) {
        child->print();
    }

    if (nodeType != "<MulExp>" && nodeType != "<AddExp>" && nodeType != "<RelExp>" && nodeType != "<EqExp>" && nodeType != "<LAndExp>" && nodeType != "<LOrExp>") {
        parser << nodeType << std::endl;
    }

    if (nodeType == "<UnaryExp>" && pre && pre->nodeType == "<MulExp>") {
        parser << "<MulExp>" << std::endl;
    } else if (nodeType == "<MulExp>" && pre && pre->nodeType == "<AddExp>") {
        parser << "<AddExp>" << std::endl;
    } else if (nodeType == "<AddExp>" && pre && pre->nodeType == "<RelExp>") {
        parser << "<RelExp>" << std::endl;
    } else if (nodeType == "<RelExp>" && pre && pre->nodeType == "<EqExp>") {
        parser << "<EqExp>" << std::endl;
    } else if (nodeType == "<EqExp>" && pre && pre->nodeType == "<LAndExp>") {
        parser << "<LAndExp>" << std::endl;
    } else if (nodeType == "<LAndExp>" && pre && pre->nodeType == "<LOrExp>") {
        parser << "<LOrExp>" << std::endl;
    }
}