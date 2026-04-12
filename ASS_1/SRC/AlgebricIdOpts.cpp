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
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


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
            if( BinOp->getOpcode() == Instruction::Add || BinOp->getOpcode() == Instruction::FAdd){

              /* Nel caso sia una ADD, devo verificare se uno dei due operandi è un valore costante uguale a 0
              e che, contemporaneamente, l'altro operando sia un valore non costante*/

              Value *operand_1 = BinOp->getOperand(0);
              Value *operand_2 = BinOp->getOperand(1);
              
              //Verifico che il primo operando sia costante con un dyn_cast a ConstantInt
              if( auto *constant_value = dyn_cast<ConstantInt>(operand_1)){
                //Se il primo operando è costnate, verifico che esso sia uguale a 0 e che il secondo operando non sia una costante
                if(constant_value->isZero() && !dyn_cast<ConstantInt>(operand_2)){
                  /* Se entrambi i controlli vanno a buon fine, ho trovato una Algebric Identify in una somma
                  Rimpiazzo tutti gli USES della istruzione che rappresenta la Algebric identify direttamente con l'operando 
                  non costante (ex: 0 + x --> x).
                  Successivamente marco la istruzione da rimuovere inserendola in un apposito vettore */
                  BinOp->replaceAllUsesWith(operand_2);
                  toDelete.push_back(BinOp);
                }
                // Controllo invece che il secondo operando sia costante con un dyn_cast ad ConstantInt
              }else if( auto *constant_value = dyn_cast<ConstantInt>(operand_2) ){
                // Verifico poi che il valore costante sia effettivamente uguale a 0 e che il primo operando non sia un valore costante
                if(constant_value->isZero() && !dyn_cast<ConstantInt>(operand_1) ){
                  /* Anche in questo caso ho trovato una Algebric Identify, dove dovrò sostituire a tutti gli uses della istruzione direttamente l'operando (ex: x + 0 --> x) non costante
                  e indicare la BinOp trovata come istruzione da rimuovere, inserendola nell'apposito vettore*/
                  BinOp->replaceAllUsesWith(operand_1);
                  toDelete.push_back(BinOp);
                }
              }

            // Nel caso in cui non sia una somma, verifico se si tratti di una moltiplicazione
            }else if( BinOp->getOpcode() == Instruction::Mul || BinOp->getOpcode() == Instruction::FMul){
              Value *operand_1 = BinOp->getOperand(0);
              Value *operand_2 = BinOp->getOperand(1);

              /* Nel caso di una moltiplicazione, devo verificare che uno dei due operandi sia un valore costante uguale ad 1
              e che l'altro operando non sia un valore costante.
              (ex: x * 1 oppure 1 * x)*/

              //Verifico con dyn_cast che il primo operando sia un ConstantInt
              if( auto *constant_value = dyn_cast<ConstantInt>(operand_1)){
                // Verifico in seguito che il secondo operando non sia costante e che il valore costante relativo al primo operando sia 1
                if(constant_value->isOne() && !dyn_cast<ConstantInt>(operand_2)){
                  /* Se entrambe le condizioni sono soddisfatte, allora significa che ho trovato una Algebric Identify.
                  Devo quindi sostituire tutti gli utilizi della BinOp direttamente con l'operando non costante (ex: 1 * x --> x)
                  e marcare l'istruzione come "da eliminare" inserendola nell'apposito vettore */
                  BinOp->replaceAllUsesWith(operand_2);
                  toDelete.push_back(BinOp);
                }

                // Se il primo operando non è costante, verifico che lo sia il secondo con un dyn_cast a ConstantInt
              }else if( auto *constant_value = dyn_cast<ConstantInt>(operand_2) ){
                // Verifico a sua volta che il valore costante sia uguale ad 1 e che il primo operando non sia costante
                if(constant_value->isOne() && !dyn_cast<ConstantInt>(operand_1) ){
                  /* Anche in questo casto ho trovato una ALgebric Identify, dove dovrò sostituire a tutti
                  li uses della BinOp direttaemnte l'operando non costante (in questo caso il primo) (ex: x * 1-->x)
                  marcando poi l'istruzione trovata come "da eliminare", inserendola nel realtivo vettore */
                  BinOp->replaceAllUsesWith(operand_1);
                  toDelete.push_back(BinOp);
                }
              }
            }
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
