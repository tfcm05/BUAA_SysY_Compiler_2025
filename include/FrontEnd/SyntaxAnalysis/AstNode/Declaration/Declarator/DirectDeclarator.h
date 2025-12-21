#ifndef DIRECTDECLARATOR_H
#define DIRECTDECLARATOR_H
#include "Declarator.h"
#include "../ParameterDecl.h"
#include "../../Expression/Expression.h"

class IdentDeclarator : public Declarator {
private:
    std::string identifier;
public:
    IdentDeclarator(const std::string& id, size_t sl);

    const std::string& getIdentifier() const;
    void setIdentifier(const std::string& id);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class ArrayDeclarator : public Declarator {
private:
    DeclaratorPtr array;
    std::vector<ExpPtr> arraySizeList;
public:
    ArrayDeclarator(DeclaratorPtr arr, std::vector<ExpPtr> sizeList, size_t sl);

    Declarator* getArray() const;
    DeclaratorPtr releaseArray();
    void setArray(DeclaratorPtr arr);

    size_t getArraySizesCount() const;
    Exp* getArraySize(size_t index) const;
    void addArraySize(ExpPtr size);
    const std::vector<ExpPtr>& getArraySizes() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

class FuncDeclarator : public Declarator {
private:
    DeclaratorPtr function;
    std::vector<ParameterDeclPtr> parameterList;
public:
    FuncDeclarator(DeclaratorPtr func, std::vector<ParameterDeclPtr> params, size_t sl);
    
    Declarator* getFunction() const;
    DeclaratorPtr releaseFunction();
    void setFunction(DeclaratorPtr func);

    size_t getParametersCount() const;
    ParameterDecl* getParameter(size_t index) const;
    void addParameter(ParameterDeclPtr param);
    const std::vector<ParameterDeclPtr>& getParameters() const;

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using DirectDeclaratorPtr = std::unique_ptr<Declarator>;
using IdentDeclaratorPtr = std::unique_ptr<IdentDeclarator>;
using ArrayDeclaratorPtr = std::unique_ptr<ArrayDeclarator>;

#endif