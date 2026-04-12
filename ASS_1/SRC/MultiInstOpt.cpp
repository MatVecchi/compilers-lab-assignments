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
namespace
{
    enum class RegisterCases{
        NEITHER,
        ONE,
        BOTH
    };

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

                    Value *registerOperand = nullptr;
                    Value *secondRegisterOperand = nullptr;
                    ConstantInt *constantValue = nullptr;
                    RegisterCases caseResult = registerAndOperand(I, registerOperand, constantValue, secondRegisterOperand);
                    
                    errs() << I << "\n";
                    errs() << registerOperand << "\n";
                    errs() << secondRegisterOperand << "\n";
                    errs() << constantValue << "\n";
                    errs() << static_cast<int>(caseResult) << "\n";

                    if(caseResult == RegisterCases::NEITHER)
                        continue;

                    auto *nestedInst = dyn_cast<Instruction>(registerOperand);
                    if(!nestedInst) continue; 

                    if(caseResult == RegisterCases::BOTH){
                        Value *nestedRegisterOperand = nullptr;
                        Value *nestedSecondRegisterOperand = nullptr;
                        ConstantInt *nestedConstantValue = nullptr;
                        RegisterCases nestedCaseResult = registerAndOperand(*nestedInst, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);
                        
                        errs() << nestedRegisterOperand << "\n";
                        errs() << nestedConstantValue << "\n";
                        errs() << nestedSecondRegisterOperand << "\n";
                        
                        if(nestedCaseResult != RegisterCases::BOTH)
                            continue;

                        if(!isOpposite(&I, nestedInst, secondRegisterOperand, nestedSecondRegisterOperand))
                            continue;

                        I.replaceAllUsesWith(nestedRegisterOperand);
                        toDelete.push_back(&I);
                    } else {
                        Value *nestedRegisterOperand = nullptr;
                        Value *nestedSecondRegisterOperand = nullptr;
                        ConstantInt *nestedConstantValue = nullptr;
                        RegisterCases nestedCaseResult = registerAndOperand(*nestedInst, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);
                        
                        errs() << nestedRegisterOperand << "\n";
                        errs() << nestedConstantValue << "\n";
                        errs() << nestedSecondRegisterOperand << "\n";
                        
                        if(nestedCaseResult == RegisterCases::NEITHER)
                            continue;

                        if(nestedCaseResult == RegisterCases::BOTH){
                            continue;
                        } else {
                            if(!isOpposite(&I, nestedInst, constantValue, nestedConstantValue))
                                continue;

                            I.replaceAllUsesWith(nestedRegisterOperand);
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

