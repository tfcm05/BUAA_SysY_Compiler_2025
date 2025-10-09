#include<iostream>
#include"include/AstNode.h"

AstNode::AstNode(std::string nodeType) : nodeType(nodeType), pre(nullptr) {}

void AstNode::addNext(std::unique_ptr<AstNode> nextNode) {
    if (nextNode) {
        nextNode->pre = this;
        next.push_back(std::move(nextNode));
    }
}

void AstNode::print() {
    for (auto& child : next) {
        child->print();
    }
    std::cout << nodeType << std::endl;
}