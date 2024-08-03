#include "llvm/Transforms/Utils/MyLoopFusion.h"
#include <cstddef>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
#include <vector>

using namespace llvm;

PreservedAnalyses MyLoopFusion::run(Function &function,
                                    FunctionAnalysisManager &fam) {

  PostDominatorTree &PT = fam.getResult<PostDominatorTreeAnalysis>(function);
  DominatorTree &DT = fam.getResult<DominatorTreeAnalysis>(function);
  ScalarEvolution &SE = fam.getResult<ScalarEvolutionAnalysis>(function);
  DependenceInfo &DI = fam.getResult<DependenceAnalysis>(function);
  LoopInfo &loopInfo = fam.getResult<LoopAnalysis>(function);

  const auto topLevelLoops = loopInfo.getTopLevelLoops();

  std::vector<std::pair<Loop *, Loop *>> adjacentLoops =
      findAdjacentLoops(topLevelLoops);

  bool merged = false;
  bool changed = false;

  std::pair<Loop *, Loop *> lastMerged{};

  for (const auto &pair : adjacentLoops) {
    // ci si può trovare una situazione del genere
    // prima coppia: A, B -> fusione
    // seocnda coppia: B, C
    // visto che A e B sono stati fusi in A, non si deve tentare la fusione
    // tra B e C ma tra A e C
    Loop *receivingLoop = merged and (lastMerged.second == pair.first)
                              ? lastMerged.first
                              : pair.first;

    bool same_trip_count = sameTripCount(SE, receivingLoop, pair.second);
    bool cf_equivalent =
        controlFlowEquivalent(DT, PT, receivingLoop, pair.second);
    bool no_negative_distance_deps = true;
    // commentato perchè il controllo delle dipendenze di llvm è troppo strict
    // no_bool negative_distance_deps =
    //     checkNegativeDistanceDeps(receivingLoop, pair.second, DI);

    merged = false;
    if (same_trip_count and cf_equivalent and no_negative_distance_deps) {
      merged = mergeLoops(receivingLoop, pair.second, SE, loopInfo);
    }

    if (merged) {
      lastMerged = pair;
      changed = true;
    } else {
      lastMerged = {};
    }
  }

  outs() << "Loops after fusion:\n";
  for (Loop *l : loopInfo.getTopLevelLoops()) {
    outs() << "\t" << *l;
  }

  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool MyLoopFusion::areLoopsAdjacent(const Loop *i, const Loop *j) const {
  const BasicBlock *iExitBlock = i->getExitBlock();
  const BasicBlock *jPreheader = j->getLoopPreheader();

  return iExitBlock != nullptr && jPreheader != nullptr &&
         iExitBlock == jPreheader && jPreheader->size() == 1;
  // TODO: remove ?
  /* Non c'è bisogno di controllare anche il tipo dell'istruzione contenuta nel
   * preheader. Per definizione, il preheader di un loop contiene un'istruzione
   * branch all'header del loop. Se quindi nel preheader c'è una sola
   * istruzione, sicuramente quella è una branch.  */
}

std::vector<std::pair<Loop *, Loop *>>
MyLoopFusion::findAdjacentLoops(const std::vector<Loop *> loops) const {
  std::vector<std::pair<Loop *, Loop *>> adjacentPairs{};

  // si scorrono i loop in ordine inverso visto che llvm li presenta al
  // contrario
  for (int i = loops.size() - 1; i >= 0; --i) {
    Loop *current = loops[i];

    // si fa un depth firs, controllando primna i loop all'interno del loop
    const auto adjSubloops = findAdjacentLoops(current->getSubLoops());
    adjacentPairs.insert(adjacentPairs.end(), adjSubloops.begin(),
                         adjSubloops.end());

    // all'ultima iterazione non ha senso controllare l'adiacenza con il loop
    // precedente visto che non esiste
    if (i <= 0) {
      continue;
    }
    outs() << "checking if loops: " << *current << " and " << *loops[i - 1]
           << " are adjacent\n";

    Loop *next = loops[i - 1];
    if (areLoopsAdjacent(current, next)) {
      outs() << "Two adjacent loops found: \n\t" << *current << "\t" << *next
             << "\n";
      adjacentPairs.emplace_back(current, next);
    }
  }
  return adjacentPairs;
}

// TODO: CHECK FOR ZERO
bool MyLoopFusion::sameTripCount(ScalarEvolution &SE, Loop const *loopi,
                                 Loop const *loopj) const {
  //   unsigned int tripi = SE.getSmallConstantTripCount(loopi);
  const SCEV *tripi = SE.getBackedgeTakenCount(loopi);
  if (isa<SCEVCouldNotCompute>(tripi)) {
    errs() << "Trip count could not be computed for the loop i.\n";
  }
  //   unsigned int tripj = SE.getSmallConstantTripCount(loopj);
  const SCEV *tripj = SE.getBackedgeTakenCount(loopj);

  if (isa<SCEVCouldNotCompute>(tripi)) {
    errs() << "Trip count could not be computed for the loop j.\n";
  }

  return tripi == tripj;
}

bool MyLoopFusion::controlFlowEquivalent(DominatorTree &DT,
                                         PostDominatorTree &PT,
                                         const Loop *loopi,
                                         const Loop *loopj) const {
  return DT.dominates(loopi->getHeader(), loopj->getHeader()) &&
         PT.dominates(loopj->getHeader(), loopi->getHeader());
}

bool MyLoopFusion::checkNegativeDistanceDeps(Loop *l1, Loop *l2,
                                             DependenceInfo &DI) const {
  for (auto *BBLoop1 : l1->blocks()) {
    for (auto &InstLoop1 : *BBLoop1) {
      for (auto *BBLoop2 : l2->blocks()) {
        for (auto &InstLoop2 : *BBLoop2) {
          if (auto Dep = DI.depends(&InstLoop1, &InstLoop2, true)) {
            if (Dep) {
              errs() << "Dependence found between:\n";
              errs() << "  Instruction in L1: " << InstLoop1 << "\n";
              errs() << "  Instruction in L2: " << InstLoop2 << "\n";
              return false;
            }
          }
        }
      }
    }
  }
  outs() << "No dep found\n";
  return true;
}

bool MyLoopFusion::mergeLoops(Loop *l1, Loop *l2, ScalarEvolution &SE,
                              LoopInfo &LI) const {
  PHINode *l1IndV = l1->getCanonicalInductionVariable();
  PHINode *l2IndV = l2->getCanonicalInductionVariable();

  if (!l2IndV || !l1IndV) {
    return false;
  }

  // utilizzo della variabile induttiva di l1 anche per l2
  l2IndV->replaceAllUsesWith(l1IndV);

  // l'header di l1 deve puntare all'exit block di l2
  l1->getHeader()->getTerminator()->replaceSuccessorWith(l1->getExitBlock(),
                                                         l2->getExitBlock());

  BasicBlock *l2BodyEntry = nullptr;
  BasicBlock *l2Header = l2->getHeader();
  BasicBlock *l2Latch = l2->getLoopLatch();
  BasicBlock *l1Latch = l1->getLoopLatch();

  // richerca dell'entry del body di l2
  for (BasicBlock *succ : successors(l2Header)) {
    if (l2->contains(succ)) {
      l2BodyEntry = succ;
      break;
    }
  }

  // predecessori del latch di l1 devono puntare al body di l2
  for (BasicBlock *pred : predecessors(l1Latch)) {
    pred->getTerminator()->replaceSuccessorWith(l1Latch, l2BodyEntry);
  }

  // i predecessori del latch di l2 devono puntare al latch di l1
  for (BasicBlock *pred : predecessors(l2Latch)) {
    pred->getTerminator()->replaceSuccessorWith(l2Latch, l1Latch);
  }

  // lheader di l2 deve puntare al suo latch
  l2->getHeader()->getTerminator()->replaceSuccessorWith(l2BodyEntry, l2Latch);

  // si posstano i blocchi di l2 in l1
  for (auto &bb : l2->blocks()) {
    // si mantengono da soli header e latch nella posizione originale
    if (bb != l2Header && bb != l2Latch) {
      l1->addBasicBlockToLoop(bb, LI);
      l2->removeBlockFromLoop(bb);
    }
  }

  // spostamento dei loop figli di l2 in l1
  while (!l2->isInnermost()) {
    // prendo solo il primo figlio e continuo fin quando non ho finito
    Loop *child = *(l2->begin());
    l2->removeChildLoop(child);
    l1->addChildLoop(child);
  }

  LI.erase(l2);

  return true;
}
