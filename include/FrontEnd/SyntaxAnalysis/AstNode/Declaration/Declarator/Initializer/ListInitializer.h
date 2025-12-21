#ifndef INITIALIZERLIST_H
#define INITIALIZERLIST_H
#include "Initializer.h"

class ListInitializer : public Initializer {
private:
    std::vector<InitializerPtr> initializerList;
public:
    ListInitializer(size_t sl);

    size_t getInitializersCount() const;
    Initializer* getInitializer(size_t index) const;
    void addInitializer(InitializerPtr init);
    const std::vector<InitializerPtr>& getInitializers() const;

    InfoPtr accept(class AstVisitor& visitor, ContextPtr context) override;
};

using ListInitializerPtr = std::unique_ptr<ListInitializer>;

#endif