#ifndef DECL_ENUMS_H
#define DECL_ENUMS_H

// Shared declaration-specifier enums (FrontEnd DeclSpec + MiddleEnd Symbol).
// Kept out of Type.h so the semantic type system does not depend on AST DeclSpec.

enum class TypeSpecifier { VOID, INT, CHAR };

enum class StorageClass { NONE, STATIC };

enum class TypeQualifier : unsigned int {
    NONE = 0,
    CONST = 1,
};

#endif
