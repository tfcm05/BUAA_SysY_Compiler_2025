#ifndef INITIALDECLARATOR_H
#define INITIALDECLARATOR_H
#include "Declarator.h"
#include "Initializer/Initializer.h"

class InitialDeclarator : public Declarator {
private:
    DeclaratorPtr declarator;
    InitializerPtr initializer;
public:
    InitialDeclarator(DeclaratorPtr decl, InitializerPtr init, size_t sl);

    Declarator* getDeclarator() const;
    DeclaratorPtr releaseDeclarator();
    void setDeclarator(DeclaratorPtr decl);

    Initializer* getInitializer() const;
    InitializerPtr releaseInitializer();
    void setInitializer(InitializerPtr init);

    InfoPtr accept(AstVisitor& visitor, ContextPtr context) override;
};

using InitialDeclaratorPtr = std::unique_ptr<InitialDeclarator>;

#endif