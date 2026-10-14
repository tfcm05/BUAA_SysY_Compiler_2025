#ifndef DECLSPEC_H
#define DECLSPEC_H

#include "DeclEnums.h"

class DeclSpec {
private:
    TypeSpecifier typeSpecifier;
    unsigned int typeQualifier;
    StorageClass storageClass;
public:
    DeclSpec();

    TypeSpecifier getTypeSpecifier() const;
    void setTypeSpecifier(TypeSpecifier ts);

    unsigned int getTypeQualifier() const;
    void setTypeQualifier(TypeQualifier tq);
    void setTypeQualifier(unsigned int tq);
    bool isConst() const;

    StorageClass getStorageClass() const;
    void setStorageClass(StorageClass sc);
    bool isStatic() const;
};

#endif
