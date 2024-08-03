#ifndef MYLICM_H
#define MYLICM_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

namespace llvm {

class MyLICM : public PassInfoMixin<MyLICM> {
public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

private:
  bool isInvariant(Instruction *inst, Loop *L,
                   std::set<Instruction *> LoopInvariantSet);
  bool isHoistable(Instruction *inst, Loop *L, DominatorTree &DT);
};

} // namespace llvm

#endif