
#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <cstdio>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <tuple>

#include <iostream>

using namespace llvm;

// definisce il numero massimo di addizioni accettate nell'ottimizzazione
//  delle moltiplicazioni in shift
#define MAX_ADDS 2

// restituisce una tupla contenente rispettivamente:
// il numero di bit da shiftare per ottenere la potenza più vicina
// e il numero di sottrazioni da eseguire
// per sostituire una moltiplicazione con uno shift
std::tuple<int, int> transformInShift(const APInt x) {
  const int exponent = x.getBitWidth() - x.countLeadingZeros() - 1;
  // si constrolla se il moltiplicatore è potenza di 2
  if (x.getSExtValue() == (1 << exponent)) {
    return std::make_tuple(exponent, 0);
  }
  const int shift_amount = exponent + 1;
  const int power = 1 << shift_amount;
  const int subs = (APInt(x.getBitWidth(), power) - x).getSExtValue();
  return std::make_tuple(shift_amount, subs);
}

bool strengthReduction(BinaryOperator *Ope, APInt IntOp, Value *OtherOp,
                       int int_position) {
  if (!Ope or !OtherOp) {
    return false;
  }
  if (Ope->getOpcode() != Instruction::Mul and
      Ope->getOpcode() != Instruction::SDiv and
      Ope->getOpcode() != Instruction::UDiv) {
    return false;
  }

  int shift_amount{};
  int adds{};
  std::tie(shift_amount, adds) = transformInShift(IntOp);

  bool is_div = Ope->getOpcode() == Instruction::SDiv or
                Ope->getOpcode() == Instruction::UDiv;

  // il numero di addizioni non deve superare il massimo
  // non si possono utilizzare addizioni per lo shift a destra (divisione)
  // e non si può ottimizzare se l'intero è il dividendo
  if (adds > MAX_ADDS or (is_div and (adds > 0 or int_position == 0))) {
    return false;
  }
  const auto shift_inst = is_div ? Instruction::AShr : Instruction::Shl;
  const auto add_instruction = is_div ? Instruction::Add : Instruction::Sub;

  Instruction *Shift = BinaryOperator::Create(
      shift_inst, OtherOp, ConstantInt::get(OtherOp->getType(), shift_amount));
  Shift->insertAfter(Ope);

  outs() << "Applying strength reduction optimization\n";

  Instruction *prev_inst = Shift;
  for (int i = 0; i < adds; ++i) {
    Instruction *Add =
        BinaryOperator::Create(add_instruction, prev_inst, OtherOp);
    Add->insertAfter(prev_inst);
    prev_inst = Add;
  }

  Ope->replaceAllUsesWith(prev_inst);
  return true;
}

bool algebraicIdentity(BinaryOperator *Ope, APInt IntOp, Value *OtherOp) {
  if (!Ope or !OtherOp) {
    return false;
  }

  APInt zero = APInt(IntOp.getBitWidth(), 0);
  APInt one = APInt(IntOp.getBitWidth(), 1);

  if (((Ope->getOpcode() == Instruction::Mul or
        Ope->getOpcode() == Instruction::SDiv or
        Ope->getOpcode() == Instruction::UDiv) and
       IntOp.eq(one)) or
      ((Ope->getOpcode() == Instruction::Add or
        Ope->getOpcode() == Instruction::Sub) and
       IntOp.eq(zero))) {
    outs() << "Applying albebraic identity optimization\n";
    Ope->replaceAllUsesWith(OtherOp);
    return true;
  } else {
    return false;
  }
}

bool multiInstructionOptimization(BinaryOperator *Operator, APInt IntOp,
                                  Value *OtherOp) {
  if (!Operator or !OtherOp) {
    return false;
  }

  bool changed = false;
  for (User *U : Operator->users()) {
    Instruction *Inst = dyn_cast<Instruction>(U);
    if (!Inst) {
      continue;
    }

    if (!Inst->isBinaryOp()) {
      continue;
    }

    BinaryOperator *BinaryOpU = dyn_cast<BinaryOperator>(Inst);
    // si prende il valore intero dell'altro operando che non è il
    // l'operazione che si sta cercando di ottimizzare
    Value *OtherOperandU = Inst->getOperand(0) == Operator
                               ? Inst->getOperand(1)
                               : Inst->getOperand(0);

    ConstantInt *OtherOperandIntU = dyn_cast<ConstantInt>(OtherOperandU);
    if (!OtherOperandIntU) {
      continue;
    }

    // se i valori corrispondono e le operazioni sono complementari si
    // sostituiscono tutti gli usi dell'istruzione analizzata con l'altro
    // operando
    if (OtherOperandIntU->getValue() == IntOp and
        ((Operator->getOpcode() == Instruction::Add and
          BinaryOpU->getOpcode() == Instruction::Sub) or
         (Operator->getOpcode() == Instruction::Sub and
          BinaryOpU->getOpcode() == Instruction::Add) or
         (Operator->getOpcode() == Instruction::Mul and
          (BinaryOpU->getOpcode() == Instruction::SDiv or
           BinaryOpU->getOpcode() == Instruction::UDiv)) or
         ((Operator->getOpcode() == Instruction::SDiv or
           Operator->getOpcode() == Instruction::UDiv) and
          BinaryOpU->getOpcode() == Instruction::Mul))) {
      outs() << "Applying multi-instruction optimization\n";
      BinaryOpU->replaceAllUsesWith(OtherOp);
      changed = true;
    }
  }
  return changed;
}

bool runOnBasicBlock(BasicBlock &B) {
  bool changed = false;
  for (auto &Inst : B) {
    if (!Inst.isBinaryOp()) {
      continue;
    }
    BinaryOperator *Operator = cast<BinaryOperator>(&Inst);
    Value *Op1 = Operator->getOperand(0);
    Value *Op2 = Operator->getOperand(1);

    ConstantInt *Op1Int = dyn_cast<ConstantInt>(Op1);
    ConstantInt *Op2Int = dyn_cast<ConstantInt>(Op2);

    ConstantInt *IntOp;
    Value *OtherOp;
    int int_position;

    if (Op1Int) {
      IntOp = Op1Int;
      OtherOp = Op2;
      int_position = 0;
    } else if (Op2Int) {
      IntOp = Op2Int;
      OtherOp = Op1;
      int_position = 1;
    } else {
      continue;
    }
    const bool identity =
        algebraicIdentity(Operator, IntOp->getValue(), OtherOp);
    changed |= identity;

    if (!identity) {
      changed |=
          strengthReduction(Operator, IntOp->getValue(), OtherOp, int_position);
    }

    changed |=
        multiInstructionOptimization(Operator, IntOp->getValue(), OtherOp);
  }
  return changed;
}

PreservedAnalyses LocalOpts::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = false;
  for (auto &F : M) {
    for (auto &BB : F) {
      changed |= runOnBasicBlock(BB);
    }
  }
  if (changed) {
    return PreservedAnalyses::none();
  } else {
    return PreservedAnalyses::all();
  }
}