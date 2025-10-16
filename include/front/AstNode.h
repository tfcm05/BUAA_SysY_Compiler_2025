#include<fstream>
#include<memory>
#include<vector>

class AstNode {
    protected:
        std::string name;
        AstNode* pre;
        std::vector<std::unique_ptr<AstNode>> next;
    public:
        ~AstNode() = default;
        AstNode(std::string name);
        void addNext(std::unique_ptr<AstNode> nextNode);
        virtual void print();
};

class TokenAstNode : public AstNode {
    private:
        std::string value;
    public:
        ~TokenAstNode() = default;
        TokenAstNode(std::string name, std::string value);
        void print() override;
};