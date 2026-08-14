#ifndef CONSTVALUE_H
#define CONSTVALUE_H

#include <cstdint>
#include <vector>
#include <memory>

class ClangContext;

namespace Clang {

class ClangContextImpl;

class ConstValue {
public:
    enum class ConstKind {
        CONST_INTEGER,
        CONST_ARRAY,
    };
private:
    ConstKind kind;
    ClangContext& context;
protected:
    // 明确通过构造函数创建实例
    explicit ConstValue(ClangContext& context, ConstKind ck);
public:
    // 禁用拷贝构造和赋值操作
    ConstValue(const ConstValue&) = delete;
    ConstValue& operator=(const ConstValue&) = delete;

    virtual ~ConstValue() = default;

    ConstKind getKind() const;

    bool isConstInteger() const;
    bool isConstArray() const;
};

class ConstInteger : public ConstValue {
private:
    int64_t intValue;
    ConstInteger(ClangContext& context, int64_t val);
    friend class ClangContextImpl;
public:
    int64_t getIntValue() const;
    void setIntValue(int64_t val);
};

class ConstArray : public ConstValue {
private:
    std::vector<ConstValue*> elements;
    ConstArray(ClangContext& context, std::vector<ConstValue*> elems);
    friend class ClangContextImpl;
public:
    const size_t getElementsSize() const;
    ConstValue* getElement(size_t index) const;
    void addElement(ConstValue* elem);
    const std::vector<ConstValue*>& getElements() const;
};

}

#endif