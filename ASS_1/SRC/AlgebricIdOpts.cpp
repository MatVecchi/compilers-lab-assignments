#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallVector.h"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace llvm;

//-----------------------------------------------------------------------------
// AlgebricIdOptsPass implementation
//-----------------------------------------------------------------------------
// è un passo che verifica l'identitò algebrica.
// Seguentemente sono mostrate gli scenari ottimizzabili e la relativa trasformazione.
/*
  x + 0 == 0 + x --> x
  x - 0 --> x
  x * 1 == 1 * x --> x
  x / 1 --> x
*/
namespace {

/*
isOpt ritorna true nel caso in cui l'istruzione passata sia ottimizzabile dal passo, false se non lo è.
Il criterio di calcolo è il seguente:
Se è una somma è ottimizzabile se il valore costante è == 0 (non importa la posizione)
Se è una moltiplicazione è ottimizzabile se il valore costante è == 1 (non importa la posizione)
Se è una sottrazione è ottimizzabile se il valore costante è == 0 ma solo come secondo operando
Se è una divisione è ottimizzabile se il valore costante è == 1 ma solo come secondo operando
*/
bool isOpt(BinaryOperator *BinOp, ConstantInt* constantValue, Value* registerValue, bool firstOperandRegister){
  auto binCode = BinOp->getOpcode();
  if(binCode == Instruction::Add && constantValue->isZero())
    return true;
  if(binCode == Instruction::Mul && constantValue->isOne())
    return true;
  if(binCode == Instruction::Sub && constantValue->isZero() && firstOperandRegister)
    return true;
  if(binCode == Instruction::SDiv && constantValue->isOne() && firstOperandRegister)
    return true;
  return false;
}

// New PM implementation
struct AlgebricIdOpts: PassInfoMixin<AlgebricIdOpts> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

      // Scorro su tutti i BasicBlock
      for( auto bb_i = F.begin(); bb_i != F.end(); bb_i++){
        BasicBlock &BB = *bb_i;
        SmallVector<Instruction*, 0> toDelete;

        // Scorro su tutte le instr di uno specifico BasicBlock
        for( auto instr_i = BB.begin(); instr_i != BB.end(); instr_i++){
          Instruction &Instr = *instr_i;

          // Verifico se l'istruzione considerata è una operazione binaria tramite un dyn_cast
          if( auto *BinOp = dyn_cast<BinaryOperator>(&Instr)  ){
            // Nel caso di oeprazione binaria, verifico la tipologia di operazione, se ADD o MUL
            
            Value *operand_1 = BinOp->getOperand(0);
            Value *operand_2 = BinOp->getOperand(1);

            ConstantInt* constantValue = nullptr; // Operando costante nella operazione
            Value* registerValue = nullptr; // Operando non costante nella operazione
            bool firstOperandRegister = false; // flag che indica la posizione dell'operando non costante

            errs() << Instr << "\n";
            /*
            Verifico con un Dynamic cast quale dei due operandi è costante.
            Quello costante lo inserisco dentro ConstantValue
            Mentre quello non costante lo inserisco dentro registerValue
            In base alla posizione dell'operando non costante setto a true|false
            il valore del flag: firstOperandRegister
            Che servirà per capire quando sono ottimizzabili anche le sottrazioni e divisioni
            */
            if( auto *CV = dyn_cast<ConstantInt>(operand_1)){
              if( !dyn_cast<ConstantInt>(operand_2)){
                constantValue = CV;
                registerValue = operand_2;
                firstOperandRegister = false;
              }else
                continue; // Passo alla prossima istruzione perchè sono entrambi costanti
            }
            else if( auto *CV = dyn_cast<ConstantInt>(operand_2)){
              if( !dyn_cast<ConstantInt>(operand_1)){
                constantValue = CV;
                registerValue = operand_1;
                firstOperandRegister = true;
              }else
                continue; // Passo alla prossima istruzione perchè sono entrambi costanti
            }else continue; // Entrambi non costanti --> passo alla prossima istruzione


            
            errs() << registerValue->getName() << "\n";
            errs() << constantValue->getSExtValue() << "\n";

            //Verifico se è ottimizzabile con la relativa funzione
            if( !isOpt(BinOp, constantValue, registerValue, firstOperandRegister))
              continue; //Continuo se non è ottimizzabile
            
            errs() << "Opt done"<<"\n";

            //Rimpiazzo tutti gli Uses della BinOp con il relativo registro 
            BinOp->replaceAllUsesWith(registerValue);

            //Marco l'istruzione selezionata da eliminare
            toDelete.push_back(BinOp);
          }

        }

        /* Dopo aver analizzato tutte le istruzioni all'interno del BasicBlock, elimino quelle marcate come "da eliminare"
        Ovvero tutte le BinOp che rappresentano delle Algebric Identify e che sono state volta per volta inserite nel vettore. 
        L'operazione di eliminazione viene svolta alla fine per evitare di modificare il codice mentre si scorrono tutte le istruzioni attraverso i vari iteratori
        evitando quindi problemi di iterazione. */
        for( Instruction *instr: toDelete)
          instr->eraseFromParent();
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
llvm::PassPluginLibraryInfo getAlgebricIdOptsPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "AlgebricIdOpts", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "algebric-id-opt") {
                    FPM.addPass(AlgebricIdOpts());
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
  return getAlgebricIdOptsPassPluginInfo();
}
