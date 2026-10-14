#ifndef EXPRESSION_H
#define EXPRESSION_H
#include "../AstNode.h"

#include <string>
#include <vector>
#include <memory>

/*
 * this is expression part
 */

class Exp : public AstNode {
public:
    Exp(size_t sl);
    virtual ~Exp() = default;
};

using ExpPtr = std::unique_ptr<Exp>;

#endif
