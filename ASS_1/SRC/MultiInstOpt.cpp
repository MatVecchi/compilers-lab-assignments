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
// MultiInstOptPass implementation
//-----------------------------------------------------------------------------
/*
    Passo che verifica la multi-instruction optimization.
    I seguenti esempi indicano gli scenari in cui può avvenire la trasformazione adottata
    dal passo:

    con valori costanti (vale la proprietà commutativa) (16 è d'esempio):
    (x + 16) - 16 --> x
    (x - 16) + 16 --> x
    (x * 16) / 16 --> x
    (x / 16) * 16 --> x

    con operandi non costanti (si assume che si espanda sempre il primo operando non costante per semplciità):
    (x + y) - y --> x
    (x - y) + y --> x
    (x * y) / y --> x
    (x / y) * y --> x
*/
namespace
{
    //Enumerazione utilizzata per capire quanti operandi non costanti sono stati trovati 
    enum class RegisterCases{
        NEITHER,
        ONE,
        BOTH
    };

    // Serve per poter capire quali sono e quanti sono gli operandi costanti e non costanti
    /*
        Si verifica se è una binary operation, e in tal caso si esegue il controllo 
        con dei dynamic cast si verifica quanti e quali operandi sono costanti e non costanti, 
        facendo ritornare il relativo valore
    */
    RegisterCases registerAndOperand(Instruction &I, Value* &inst, ConstantInt* &constantValue, Value* &inst2){
        auto *BinOp = dyn_cast<BinaryOperator>(&I);
        if (!BinOp || (BinOp->getOpcode() != Instruction::Mul && BinOp->getOpcode() != Instruction::SDiv && BinOp->getOpcode() != Instruction::Add && BinOp->getOpcode() != Instruction::Sub))
            return RegisterCases::NEITHER;

        if ( auto *C = dyn_cast<ConstantInt>(BinOp->getOperand(0)))
        {
            constantValue = C;
            if (dyn_cast<ConstantInt>(BinOp->getOperand(1)))
                return RegisterCases::NEITHER;
            inst = dyn_cast<Value>(BinOp->getOperand(1));
        }
        else if (auto *C = dyn_cast<ConstantInt>(BinOp->getOperand(1))){
            constantValue = C;
            inst = dyn_cast<Value>(BinOp->getOperand(0));
        }
        else{
            inst = dyn_cast<Value>(BinOp->getOperand(0)); // Supponendo per semplicità che l'istruzione da espandere sia sempre la prima
            inst2 = dyn_cast<Value>(BinOp->getOperand(1));
            return RegisterCases::BOTH;
        }
        return RegisterCases::ONE;
    }

    /*
        isOppsite è una funzione che ritorna true o false se l'istruzione passata (e la relativa innestata) sono ottimizzabili
        Funzione che verifica quando le due operazioni (esterna ed interna) sono opposte.
        Si verifica inoltre che i relativi valori utilizzati siano uguali e in tal caso si ritorna true.
        False altrimenti
    */
    bool isOpposite(Instruction* op1, Instruction* op2, ConstantInt* val1, ConstantInt* val2){
        if(!(op1->getOpcode() == Instruction::Mul && op2->getOpcode() == Instruction::SDiv || 
        op1->getOpcode() == Instruction::SDiv && op2->getOpcode() == Instruction::Mul || 
        op1->getOpcode() == Instruction::Add && op2->getOpcode() == Instruction::Sub ||
        op1->getOpcode() == Instruction::Sub && op2->getOpcode() == Instruction::Add
        ))
            return false;
        
        return val1->getSExtValue() == val2->getSExtValue(); // Si suppone che sia già stata applicata la Algebric Semplification
    }

    bool isOpposite(Instruction* op1, Instruction* op2, Value* val1, Value* val2){
         if(!(op1->getOpcode() == Instruction::Mul && op2->getOpcode() == Instruction::SDiv || 
        op1->getOpcode() == Instruction::SDiv && op2->getOpcode() == Instruction::Mul || 
        op1->getOpcode() == Instruction::Add && op2->getOpcode() == Instruction::Sub ||
        op1->getOpcode() == Instruction::Sub && op2->getOpcode() == Instruction::Add
        ))
            return false;
        
        return val1 == val2; // Si suppone che sia già stata applicata la Algebric Semplification
    }

    // New PM implementation
    struct MultiInstOptPass : PassInfoMixin<MultiInstOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &)
        {

            // Scorro su tutti i BasciBlock
            for (auto bb_i = F.begin(); bb_i != F.end(); ++bb_i)
            {
                BasicBlock &BB = *bb_i;
                SmallVector<Instruction *, 0> toDelete;

                // Scorro su tutte le istruzioni
                for (auto instr_i = BB.begin(); instr_i != BB.end(); ++instr_i)
                {
                    Instruction &I = *instr_i;

                    // Si analizza prima l'istruzione esterna
                    Value *registerOperand = nullptr;  //oeprando non costante esterno
                    Value *secondRegisterOperand = nullptr; //operando non costante esterno (può rimanere null)
                    ConstantInt *constantValue = nullptr; //operando costante esterno (può rimaenre null)
                    RegisterCases caseResult = registerAndOperand(I, registerOperand, constantValue, secondRegisterOperand);
                    
                    errs() << I << "\n";
                    errs() << registerOperand << "\n";
                    errs() << secondRegisterOperand << "\n";
                    errs() << constantValue << "\n";
                    errs() << static_cast<int>(caseResult) << "\n";

                    //Se sono entrambi costanti --> passo alla prossima istruzione
                    if(caseResult == RegisterCases::NEITHER)
                        continue;

                    //Altrimenti verifico l'istruzione interna (nested) in corrispondenza dell'oeprando non costante
                    // Se sono entrambi non costanti si espande solaemnte il primo (per semplciità)
                    auto *nestedInst = dyn_cast<Instruction>(registerOperand);
                    if(!nestedInst) continue; // Se non è una istruzione allora non si può semplificare

                    if(caseResult == RegisterCases::BOTH){ //Se sono entrambi non costanti espando il primo operando
                        Value *nestedRegisterOperand = nullptr; //primo operando nested non costante
                        Value *nestedSecondRegisterOperand = nullptr; //secondo operando nested non costante (possibilmente nullo)
                        ConstantInt *nestedConstantValue = nullptr; // secondo oeprando nested costante (possibilmente nullo)
                        RegisterCases nestedCaseResult = registerAndOperand(*nestedInst, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);
                        
                        errs() << nestedRegisterOperand << "\n";
                        errs() << nestedConstantValue << "\n";
                        errs() << nestedSecondRegisterOperand << "\n";
                        
                        // In questo caso anche nella nested Instruction devono essere entrambi non costanti per essere ottimizzati
                        if(nestedCaseResult != RegisterCases::BOTH)
                            continue;

                        //Verifico la possibilità di ottimizzazione
                        if(!isOpposite(&I, nestedInst, secondRegisterOperand, nestedSecondRegisterOperand))
                            continue;

                        // rimpiazzio gli uses della istruzione con il primo operando della nested instruction
                        I.replaceAllUsesWith(nestedRegisterOperand);
                        toDelete.push_back(&I);

                    } else { //Se solo uno è non costante --> lo esapndo
                        Value *nestedRegisterOperand = nullptr; // primo operando nested non costante 
                        Value *nestedSecondRegisterOperand = nullptr; //secondo operando nested non costante (può essere null)
                        ConstantInt *nestedConstantValue = nullptr; // secondo operando nested costante (può essere null)
                        RegisterCases nestedCaseResult = registerAndOperand(*nestedInst, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);
                        
                        errs() << nestedRegisterOperand << "\n";
                        errs() << nestedConstantValue << "\n";
                        errs() << nestedSecondRegisterOperand << "\n";
                        
                        if(nestedCaseResult == RegisterCases::NEITHER)
                            continue;

                        if(nestedCaseResult == RegisterCases::BOTH){
                            continue;
                        } else { //è possibile ottimizzare solo se anche nella nested c'è solamente un operando non costante
                            if(!isOpposite(&I, nestedInst, constantValue, nestedConstantValue)) //verifico la possibilità di ottimizzare
                                continue;

                            //rimpiazzio gli uses di BinOp con il primo operando nested
                            I.replaceAllUsesWith(nestedRegisterOperand);

                            //marco l'istruzione da eliminare
                            toDelete.push_back(&I);
                        }
                    }

                    errs() << "\n\n";
                }

                for (auto *I : toDelete)
                    I->eraseFromParent();
            }

            return PreservedAnalyses::all();
        };

        // Without isRequired returning true, this pass will be skipped for functions
        // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
        // all functions with optnone.
        static bool isRequired() { return true; }
    };
} 
// namespace

    //-----------------------------------------------------------------------------
    // New PM Registration
    //-----------------------------------------------------------------------------
    llvm::PassPluginLibraryInfo getMultiInstOptPassPluginInfo()
    {
        return {LLVM_PLUGIN_API_VERSION, "MultiInstOptPass", LLVM_VERSION_STRING,
                [](PassBuilder &PB)
                {
                    PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                           ArrayRef<PassBuilder::PipelineElement>)
                        {
                            if (Name == "multi-inst-opt")
                            {
                                FPM.addPass(MultiInstOptPass());
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
    llvmGetPassPluginInfo()
    {
        return getMultiInstOptPassPluginInfo();
    }

