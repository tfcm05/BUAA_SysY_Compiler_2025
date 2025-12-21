#ifndef CONST_ARRAY_H
#define CONST_ARRAY_H

#include "Constant.h"

#include <vector>

class ConstantArray : public Constant {
private:
    LLVM::Type* elementType;
    std::vector<Constant*> elements;
public:
    ConstantArray(LLVM::Type* type, std::vector<Constant*> elements, const std::string& name);

    LLVM::Type* getElementType() const;
    void setElementType(LLVM::Type* newElementType);

    size_t getElementsCount() const;
    Constant* getElement(size_t index) const;
    void addElement(Constant* element);
    void setElements(std::vector<Constant*> newElements);
    const std::vector<Constant*>& getElements() const;
    
};

#endif