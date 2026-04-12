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

    RegisterCases registerAndOperand(Instruction &I, Instruction* &inst, ConstantInt* &constantValue, Instruction* &inst2){
        auto *BinOp = dyn_cast<BinaryOperator>(&I);
        if (!BinOp || (BinOp->getOpcode() != Instruction::Mul && BinOp->getOpcode() != Instruction::SDiv && BinOp->getOpcode() != Instruction::Add && BinOp->getOpcode() != Instruction::Sub))
            return RegisterCases::NEITHER;

        if (constantValue = dyn_cast<ConstantInt>(BinOp->getOperand(0)))
        {
            if (dyn_cast<ConstantInt>(BinOp->getOperand(1)))
                return RegisterCases::NEITHER;
            inst = dyn_cast<Instruction>(BinOp->getOperand(1));
        }
        else if (constantValue = dyn_cast<ConstantInt>(BinOp->getOperand(1)))
            inst = dyn_cast<Instruction>(BinOp->getOperand(0));
        else
            inst = dyn_cast<Instruction>(BinOp->getOperand(0)); // Supponendo per semplicità che l'istruzione da espandere sia sempre la prima
            inst2 = dyn_cast<Instruction>(BinOp->getOperand(1));
            return RegisterCases::BOTH;
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

    bool isOpposite(Instruction* op1, Instruction* op2, Instruction* val1, Instruction* val2){
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

                    Instruction *registerOperand;
                    Instruction *secondRegisterOperand;
                    ConstantInt *constantValue;
                    RegisterCases caseResult = registerAndOperand(I, registerOperand, constantValue, secondRegisterOperand);
                    if(caseResult == RegisterCases::NEITHER)
                        continue;

                    if(caseResult == RegisterCases::BOTH){
                        Instruction *nestedRegisterOperand;
                        Instruction *nestedSecondRegisterOperand;
                        ConstantInt *nestedConstantValue;
                        RegisterCases nestedCaseResult = registerAndOperand(*registerOperand, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);

                        if(nestedCaseResult != RegisterCases::BOTH)
                            continue;

                        if(!isOpposite(&I, registerOperand, secondRegisterOperand, nestedSecondRegisterOperand))
                            continue;

                        I.replaceAllUsesWith(nestedRegisterOperand);
                        toDelete.push_back(&I);
                    } else {
                        Instruction *nestedRegisterOperand;
                        Instruction *nestedSecondRegisterOperand;
                        ConstantInt *nestedConstantValue;
                        RegisterCases nestedCaseResult = registerAndOperand(*registerOperand, nestedRegisterOperand, nestedConstantValue, nestedSecondRegisterOperand);

                        if(nestedCaseResult == RegisterCases::NEITHER)
                            continue;

                        if(nestedCaseResult == RegisterCases::BOTH){
                            continue;
                        } else {
                            if(!isOpposite(&I, registerOperand, constantValue, nestedConstantValue))
                                continue;

                            I.replaceAllUsesWith(nestedConstantValue);
                            toDelete.push_back(&I);
                        }
                    }

                    
                }

                for (auto *I : toDelete)
                        I->removeFromParent();
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

