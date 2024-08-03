#ifndef MY_LOOP_FUSION_H
#define MY_LOOP_FUSION_H

#include <concepts>
#include <llvm/Analysis/DependenceAnalysis.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>

namespace llvm {
class MyLoopFusion : public llvm::PassInfoMixin<MyLoopFusion> {
private:
  bool areLoopsAdjacent(const llvm::Loop *, const llvm::Loop *) const;

  std::vector<std::pair<Loop *, Loop *>>
  findAdjacentLoops(const std::vector<Loop *> loops) const;
  bool sameTripCount(llvm::ScalarEvolution &SE, llvm::Loop const *loopi,
                     llvm::Loop const *loopj) const;
  bool controlFlowEquivalent(llvm::DominatorTree &DT,
                             llvm::PostDominatorTree &PT,
                             const llvm::Loop *loopi,
                             const llvm::Loop *loopj) const;
  bool checkNegativeDistanceDeps(Loop *l1, Loop *l2, DependenceInfo &DI) const;
  bool mergeLoops(llvm::Loop *loopFused, llvm::Loop *loopToFuse,
                  llvm::ScalarEvolution &SE, llvm::LoopInfo &LI) const;
  llvm::PHINode *getInductionVariable(llvm::Loop *L,
                                      llvm::ScalarEvolution &SE) const;

public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};
} // namespace llvm

#endif