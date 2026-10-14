#include "FrontEnd/SyntaxAnalysis/AstNodes.h"

/*
 * Declaration and derived classes
 */

Decl::Decl(size_t sl) : AstNode(sl) {}

// ----- VariableDecl -----
VariableDecl::VariableDecl(size_t sl) : Decl(sl) {}

DeclSpec VariableDecl::getDeclSpec() const {
    return declSpec;
}
void VariableDecl::setDeclSpec(const DeclSpec& ds) {
    declSpec = ds;
}

size_t VariableDecl::getDeclaratorsCount() const {
    return declaratorList.size();
}
Declarator* VariableDecl::getDeclarator(size_t index) const {
    if (index < declaratorList.size()) {
        return declaratorList[index].get();
    }
    return nullptr;
}
void VariableDecl::addDeclarator(DeclaratorPtr decl) {
    declaratorList.push_back(std::move(decl));
}
const std::vector<DeclaratorPtr>& VariableDecl::getDeclarators() const {
    return declaratorList;
}

InfoPtr VariableDecl::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitVariableDecl(this, context);
}

// ----- FunctionDecl -----
FunctionDecl::FunctionDecl(size_t sl) : Decl(sl) {}

DeclSpec FunctionDecl::getDeclSpec() const {
    return declSpec;
}
void FunctionDecl::setDeclSpec(const DeclSpec& ds) {
    declSpec = ds;
}

Declarator* FunctionDecl::getDeclarator() const {
    return declarator.get();
}
DeclaratorPtr FunctionDecl::releaseDeclarator() {
    return std::move(declarator);
}
void FunctionDecl::setDeclarator(DeclaratorPtr decl) {
    declarator = std::move(decl);
}

BlockStmt* FunctionDecl::getBody() const {
    return body.get();
}
BlockStmtPtr FunctionDecl::releaseBody() {
    return std::move(body);
}
void FunctionDecl::setBody(BlockStmtPtr b) {
    body = std::move(b);
}

InfoPtr FunctionDecl::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitFunctionDecl(this, context);
}

// ----- ParameterDecl -----
ParameterDecl::ParameterDecl(size_t sl) : Decl(sl) {}

DeclSpec ParameterDecl::getDeclSpec() const {
    return declSpec;
}
void ParameterDecl::setDeclSpec(const DeclSpec& ds) {
    declSpec = ds;
}

Declarator* ParameterDecl::getDeclarator() const {
    return declarator.get();
}
DeclaratorPtr ParameterDecl::releaseDeclarator() {
    return std::move(declarator);
}
void ParameterDecl::setDeclarator(DeclaratorPtr decl) {
    declarator = std::move(decl);
}

InfoPtr ParameterDecl::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitParameterDecl(this, context);
}

// ----- DeclSpec -----
DeclSpec::DeclSpec()
    : typeSpecifier(TypeSpecifier::VOID), typeQualifier(0), storageClass(StorageClass::NONE) {}

TypeSpecifier DeclSpec::getTypeSpecifier() const {
    return typeSpecifier;
}
void DeclSpec::setTypeSpecifier(TypeSpecifier ts) {
    typeSpecifier = ts;
}

unsigned int DeclSpec::getTypeQualifier() const {
    return typeQualifier;
}
void DeclSpec::setTypeQualifier(TypeQualifier tq) {
    typeQualifier |= static_cast<unsigned int>(tq);
}
void DeclSpec::setTypeQualifier(unsigned int tq) {
    typeQualifier = tq;
}
bool DeclSpec::isConst() const {
    return (typeQualifier & static_cast<unsigned int>(TypeQualifier::CONST)) != 0;
}

StorageClass DeclSpec::getStorageClass() const {
    return storageClass;
}
void DeclSpec::setStorageClass(StorageClass sc) {
    storageClass = sc;
}
bool DeclSpec::isStatic() const {
    return storageClass == StorageClass::STATIC;
}

// ----- Declarator -----
Declarator::Declarator(size_t sl) : AstNode(sl) {}

IdentDeclarator::IdentDeclarator(const std::string& id, size_t sl)
    : Declarator(sl), identifier(id) {}

const std::string& IdentDeclarator::getIdentifier() const {
    return identifier;
}
void IdentDeclarator::setIdentifier(const std::string& id) {
    identifier = id;
}

InfoPtr IdentDeclarator::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitIdentDeclarator(this, context);
}

PointerDeclarator::PointerDeclarator(DeclaratorPtr ptr, size_t sl)
    : Declarator(sl), pointer(std::move(ptr)) {}

Declarator* PointerDeclarator::getPointer() const {
    return pointer.get();
}
DeclaratorPtr PointerDeclarator::releasePointer() {
    return std::move(pointer);
}
void PointerDeclarator::setPointer(DeclaratorPtr ptr) {
    pointer = std::move(ptr);
}

InfoPtr PointerDeclarator::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitPointerDeclarator(this, context);
}

ArrayDeclarator::ArrayDeclarator(DeclaratorPtr arr, std::vector<ExpPtr> sizeList, size_t sl)
    : Declarator(sl), array(std::move(arr)), arraySizeList(std::move(sizeList)) {}

Declarator* ArrayDeclarator::getArray() const {
    return array.get();
}
DeclaratorPtr ArrayDeclarator::releaseArray() {
    return std::move(array);
}
void ArrayDeclarator::setArray(DeclaratorPtr arr) {
    array = std::move(arr);
}

size_t ArrayDeclarator::getArraySizesCount() const {
    return arraySizeList.size();
}
Exp* ArrayDeclarator::getArraySize(size_t index) const {
    if (index < arraySizeList.size()) {
        return arraySizeList[index].get();
    }
    return nullptr;
}
void ArrayDeclarator::addArraySize(ExpPtr size) {
    arraySizeList.push_back(std::move(size));
}
const std::vector<ExpPtr>& ArrayDeclarator::getArraySizes() const {
    return arraySizeList;
}

InfoPtr ArrayDeclarator::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitArrayDeclarator(this, context);
}

FuncDeclarator::FuncDeclarator(DeclaratorPtr func, std::vector<ParameterDeclPtr> params, size_t sl, bool vari)
    : Declarator(sl), function(std::move(func)), parameterList(std::move(params)), variadic(vari) {}

Declarator* FuncDeclarator::getFunction() const {
    return function.get();
}
DeclaratorPtr FuncDeclarator::releaseFunction() {
    return std::move(function);
}
void FuncDeclarator::setFunction(DeclaratorPtr func) {
    function = std::move(func);
}

size_t FuncDeclarator::getParametersCount() const {
    return parameterList.size();
}
ParameterDecl* FuncDeclarator::getParameter(size_t index) const {
    if (index < parameterList.size()) {
        return parameterList[index].get();
    }
    return nullptr;
}
void FuncDeclarator::addParameter(ParameterDeclPtr param) {
    parameterList.push_back(std::move(param));
}
const std::vector<ParameterDeclPtr>& FuncDeclarator::getParameters() const {
    return parameterList;
}

bool FuncDeclarator::isVariadic() const {
    return variadic;
}

InfoPtr FuncDeclarator::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitFuncDeclarator(this, context);
}

InitialDeclarator::InitialDeclarator(DeclaratorPtr decl, InitializerPtr init, size_t sl)
    : Declarator(sl), declarator(std::move(decl)), initializer(std::move(init)) {}

Declarator* InitialDeclarator::getDeclarator() const {
    return declarator.get();
}
DeclaratorPtr InitialDeclarator::releaseDeclarator() {
    return std::move(declarator);
}
void InitialDeclarator::setDeclarator(DeclaratorPtr decl) {
    declarator = std::move(decl);
}

Initializer* InitialDeclarator::getInitializer() const {
    return initializer.get();
}
InitializerPtr InitialDeclarator::releaseInitializer() {
    return std::move(initializer);
}
void InitialDeclarator::setInitializer(InitializerPtr init) {
    initializer = std::move(init);
}

InfoPtr InitialDeclarator::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitInitialDeclarator(this, context);
}

// ----- Initializer -----
Initializer::Initializer(size_t sl) : AstNode(sl) {}

ExpInitializer::ExpInitializer(ExpPtr exp, size_t sl)
    : Initializer(sl), expression(std::move(exp)) {}

Exp* ExpInitializer::getExp() const {
    return expression.get();
}
ExpPtr ExpInitializer::releaseExp() {
    return std::move(expression);
}
void ExpInitializer::setExp(ExpPtr exp) {
    expression = std::move(exp);
}

int64_t ExpInitializer::getConstValue() const {
    return constValue;
}
void ExpInitializer::setConstValue(int64_t cv) {
    constValue = cv;
}

InfoPtr ExpInitializer::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitExpInitializer(this, context);
}

ListInitializer::ListInitializer(size_t sl) : Initializer(sl) {}

size_t ListInitializer::getInitializersCount() const {
    return initializerList.size();
}
Initializer* ListInitializer::getInitializer(size_t index) const {
    if (index < initializerList.size()) {
        return initializerList[index].get();
    }
    return nullptr;
}
void ListInitializer::addInitializer(InitializerPtr init) {
    initializerList.push_back(std::move(init));
}
const std::vector<InitializerPtr>& ListInitializer::getInitializers() const {
    return initializerList;
}

InfoPtr ListInitializer::accept(AstVisitor& visitor, ContextPtr context) {
    return visitor.visitListInitializer(this, context);
}

// ----- End of Declaration and derived classes -----