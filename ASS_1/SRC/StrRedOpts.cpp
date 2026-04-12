#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

int found_pow( int n, double &log_value){
    if( n == 1){
        log_value = 0;
        return 0;
    }
    if(n <= 0) return -2;

    log_value = std::log2(n);
    if( std::fmod(log_value, 1) == 0) return 0;


    log_value = std::log2(n + 1);
    if( std::fmod(log_value, 1) == 0) return 1;


    log_value = std::log2(n - 1);
    if( std::fmod(log_value, 1) == 0) return -1;

    return -2;
}

// New PM implementation
struct StrRedOptsPass: PassInfoMixin<StrRedOptsPass> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

        // Scorro su tutti i BasciBlock
        for(auto bb_i = F.begin(); bb_i != F.end(); ++bb_i){
            BasicBlock &BB = *bb_i;
            SmallVector<Instruction*, 0> toDelete;

            // Scorro su tutte le istruzioni
            for( auto instr_i = BB.begin(); instr_i != BB.end(); ++instr_i){
                Instruction &I = *instr_i;

                // Verifico se è una operazione binaria con un dyn_cast a BinaryOperator
                if( auto *BinOp = dyn_cast<BinaryOperator>(&I)){
                    /* Verifico se l'istruzione è una moltiplicazione (intera o float) verificando se il suo
                    OP code (identificatore univoco del tipo di istruzione) è uguale a quello di una Mul o di una Fmul */
                    
                    if( BinOp->getOpcode() == Instruction::Mul || BinOp->getOpcode() == Instruction::FMul){
                        auto *operand_1 = BinOp->getOperand(0);
                        auto *operand_2 = BinOp->getOperand(1);

                        if(auto *constant_value = dyn_cast<ConstantInt>(operand_1)){
                            if(!dyn_cast<ConstantInt>(operand_2)){
                                double log_value = 0;
                                int result = found_pow(constant_value->getSExtValue(), log_value);
                                
                                Type *opType = operand_2->getType();
                                Value *val = ConstantInt::get(opType, log_value);

                                if( result == 0 ){
                                    Instruction *NewInst = BinaryOperator::CreateShl( operand_2, val);
                                    BinOp->replaceAllUsesWith(NewInst);
                                    NewInst->insertBefore(BinOp);
                                    toDelete.push_back(BinOp);
                                }
                                else if(result == -1){
                                    // Crea lo shift
                                    Instruction *NewShL = BinaryOperator::CreateShl(operand_2, val);
                                    NewShL->insertBefore(BinOp);  // prima inserisci nel BB

                                    // Crea l'add
                                    Instruction *NewAdd = BinaryOperator::CreateAdd(NewShL, operand_2);
                                    NewAdd->insertAfter(NewShL);  // subito dopo lo shift

                                    //  Sostituisci tutti gli usi del vecchio BinOp
                                    BinOp->replaceAllUsesWith(NewAdd);

                                    //  Metti BinOp in lista di cancellazione
                                    toDelete.push_back(BinOp);
                                }
                                else if (result == 1){
                                    // Crea lo shift
                                    Instruction *NewShL = BinaryOperator::CreateShl(operand_2, val);
                                    NewShL->insertBefore(BinOp);  // prima inserisci nel BB

                                    // Crea la sub
                                    Instruction *NewSub = BinaryOperator::CreateSub(NewShL, operand_2);
                                    NewSub->insertAfter(NewShL);  // subito dopo lo shift

                                    //  Sostituisci tutti gli usi del vecchio BinOp
                                    BinOp->replaceAllUsesWith(NewSub);

                                    //  Metti BinOp in lista di cancellazione
                                    toDelete.push_back(BinOp);
                                }
                            }
                            
                        } else if(auto *constant_value = dyn_cast<ConstantInt>(operand_2)){
                            if(!dyn_cast<ConstantInt>(operand_1)){
                                double log_value = 0;
                                int result = found_pow(constant_value->getSExtValue(), log_value);
                                
                                Type *opType = operand_1->getType();
                                Value *val = ConstantInt::get(opType, log_value);

                                if( result == 0 ){
                                    Instruction *NewInst = BinaryOperator::CreateShl( operand_1, val);
                                    BinOp->replaceAllUsesWith(NewInst);
                                    NewInst->insertBefore(BinOp);
                                    toDelete.push_back(BinOp);
                                }
                                else if(result == -1){
                                    // Crea lo shift
                                    Instruction *NewShL = BinaryOperator::CreateShl(operand_1, val);
                                    NewShL->insertBefore(BinOp);  // prima inserisci nel BB

                                    // Crea l'add
                                    Instruction *NewAdd = BinaryOperator::CreateAdd(NewShL, operand_1);
                                    NewAdd->insertAfter(NewShL);  // subito dopo lo shift

                                    //  Sostituisci tutti gli usi del vecchio BinOp
                                    BinOp->replaceAllUsesWith(NewAdd);

                                    //  Metti BinOp in lista di cancellazione
                                    toDelete.push_back(BinOp);
                                }
                                else if (result == 1){
                                    // Crea lo shift
                                    Instruction *NewShL = BinaryOperator::CreateShl(operand_1, val);
                                    NewShL->insertBefore(BinOp);  // prima inserisci nel BB

                                    // Crea la sub
                                    Instruction *NewSub = BinaryOperator::CreateSub(NewShL, operand_1);
                                    NewSub->insertAfter(NewShL);  // subito dopo lo shift

                                    //  Sostituisci tutti gli usi del vecchio BinOp
                                    BinOp->replaceAllUsesWith(NewSub);

                                    //  Metti BinOp in lista di cancellazione
                                    toDelete.push_back(BinOp);
                                }
                            }
                            
                        }
                    } else if( BinOp->getOpcode() == Instruction::SDiv || BinOp->getOpcode() == Instruction::FDiv){
                        auto *operand_1 = BinOp->getOperand(0);
                        auto *operand_2 = BinOp->getOperand(1);

                        if(auto *constant_value = dyn_cast<ConstantInt>(operand_1)){
                            if(!dyn_cast<ConstantInt>(operand_2)){
                                double log_value = 0;
                                int result = found_pow(constant_value->getSExtValue(), log_value);
                                
                                Type *opType = operand_2->getType();
                                Value *val = ConstantInt::get(opType, log_value);

                                if( result == 0 ){
                                    Instruction *NewInst = BinaryOperator::CreateAShr( operand_2, val);
                                    BinOp->replaceAllUsesWith(NewInst);
                                    NewInst->insertBefore(BinOp);
                                    toDelete.push_back(BinOp);
                                }
                                
                            }
                            
                        } else if(auto *constant_value = dyn_cast<ConstantInt>(operand_2)){
                            if(!dyn_cast<ConstantInt>(operand_1)){
                                double log_value = 0;
                                int result = found_pow(constant_value->getSExtValue(), log_value);
                                
                                Type *opType = operand_1->getType();
                                Value *val = ConstantInt::get(opType, log_value);

                                if( result == 0 ){
                                    Instruction *NewInst = BinaryOperator::CreateAShr( operand_1, val);
                                    BinOp->replaceAllUsesWith(NewInst);
                                    NewInst->insertBefore(BinOp);
                                    toDelete.push_back(BinOp);
                                }
                                
                            }
                            
                        }
                    }




                }
            }

            for(auto *I: toDelete)
                I->removeFromParent();
        }

        return PreservedAnalyses::all();
    }


  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};




} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getStrRedOptsPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "StrRedOptsPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "str-red-opt") {
                    FPM.addPass(StrRedOptsPass());
                    return true;
                  }
                  return false;
                });
            
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getStrRedOptsPassPluginInfo();
}
