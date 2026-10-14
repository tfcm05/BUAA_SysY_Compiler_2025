#ifndef MIDDLEEND_OPTIMIZATION_PASS_MANAGER_H
#define MIDDLEEND_OPTIMIZATION_PASS_MANAGER_H

#include "MiddleEnd/Optimization/Pass.h"

#include <memory>
#include <vector>

// Runs Module-level Passes in registration order. Does not re-walk the AST.
class PassManager {
private:
    std::vector<std::unique_ptr<Pass>> passes;
public:
    void add(std::unique_ptr<Pass> pass) {
        passes.push_back(std::move(pass));
    }

    // Returns sum of pass-specific metrics.
    int run(Module& module, LLVMContext& context) {
        int total = 0;
        for (auto& pass : passes) {
            total += pass->runOnModule(module, context);
        }
        return total;
    }
};

#endif
