#include "llvm/Transforms/Utils/MyLICM.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <unordered_map>
#include <unordered_set>

using namespace llvm;

PreservedAnalyses MyLICM::run(Loop &L, LoopAnalysisManager &AM,
                              LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  outs() << "Running MyLICM\n";

  DominatorTree &DT = AR.DT;
  LoopInfo &LI = AR.LI;
  bool Changed = false;

  // identificazione del preheader del loop
  BasicBlock *Preheader = L.getLoopPreheader();
  if (!Preheader)
    return PreservedAnalyses::all();

  std::set<Instruction *> loopInvariantInstructionsSet;
  std::set<Instruction *> hoistableInstructions{};

  // identificazione delle istruzioni loop-invariant
  for (BasicBlock *BB : L.blocks()) {
    for (Instruction &Instr : *BB) {
      if (isInvariant(&Instr, &L, loopInvariantInstructionsSet)) {
        loopInvariantInstructionsSet.insert(&Instr);
      }
    }
  }

  for (Instruction *Inst : loopInvariantInstructionsSet) {
    outs() << "moving instruction: " << *Inst << " to the preheader\n";
    Inst->moveBefore(Preheader->getTerminator());
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool MyLICM::isHoistable(Instruction *inst, Loop *L, DominatorTree &DT) {
  // check if instructions dominates all exits
  SmallVector<BasicBlock *> ExitBlocks{};
  L->getExitBlocks(ExitBlocks);

  BasicBlock *instructionParentBB = inst->getParent();

  bool dominates_exits = true;
  for (auto *ExitBlock : ExitBlocks) {
    if (!DT.dominates(instructionParentBB, ExitBlock)) {
      dominates_exits = false;
      break;
    }
  }

  if (dominates_exits) {
    return true;
  }

  // check if instruction is dead outside loop
  for (auto user = inst->user_begin(); user != inst->user_end(); ++user) {
    Instruction *user_inst = dyn_cast<Instruction>(*user);

    if (!L->contains(user_inst))
      return false;
  }

  return true;
}

bool MyLICM::isInvariant(Instruction *inst, Loop *L,
                         std::set<Instruction *> loopInvariantInstructionSet) {

  outs() << "Checking instruction " << *inst << "\n";

  // Check if the istruction may have side effects
  if (inst->mayHaveSideEffects() or inst->isTerminator() or
      inst->getOpcode() == Instruction::Br or
      inst->getOpcode() == Instruction::ICmp or isa<PHINode>(inst)) {
    return false;
  }

  // All ops must be loop invariant
  for (auto op = inst->op_begin(); op != inst->op_end(); ++op) {
    outs() << "Checking operand: " << **op << "\n";

    if (isa<Constant>(op) or isa<Argument>(op)) {
      continue;
    }

    Instruction *op_inst = dyn_cast<Instruction>(op);

    if (not op_inst) {
      outs() << "Not an instruction \n";
      return false;
    }

    if (!L->contains(op_inst)) {
      continue;
    }

    if (loopInvariantInstructionSet.count(op_inst) == 0) {
      return false;
    }
  }

  outs() << "Instruction is loop invariant\n";
  return true;
}