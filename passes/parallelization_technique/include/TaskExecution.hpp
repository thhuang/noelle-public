#pragma once

#include <unordered_map>

using namespace std;

namespace llvm {

  struct TaskExecution {
    int order;
    Function *F;
    BasicBlock *entryBlock, *exitBlock;
    std::vector<BasicBlock *> loopExitBlocks;

    Value *envArg;
    Value *instanceIndexV;

    std::unordered_map<BasicBlock *, BasicBlock *> basicBlockClones;
    std::unordered_map<Instruction *, Instruction *> instructionClones;
    std::unordered_map<Value *, Value *> liveInClones;

    virtual void extractFuncArgs () = 0;
  };
}
