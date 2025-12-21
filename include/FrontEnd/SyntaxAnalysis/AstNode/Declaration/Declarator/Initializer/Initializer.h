#ifndef INITIALIZER_H
#define INITIALIZER_H
#include "../../../AstNode.h"

class Initializer : public AstNode {
public:
    Initializer(size_t sl);
    virtual ~Initializer() = default;
};

using InitializerPtr = std::unique_ptr<Initializer>;

#endif