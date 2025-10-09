#include<fstream>
#include<memory>
#include<vector>

class AstNode {
    private:
        std::string nodeType;
        AstNode* pre;
        std::vector<std::unique_ptr<AstNode>> next;
    public:
        AstNode(std::string nodeType);
        void addNext(std::unique_ptr<AstNode> nextNode);
        void print();
};