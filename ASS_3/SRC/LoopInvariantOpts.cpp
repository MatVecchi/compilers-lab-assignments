#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;
namespace
{
    void analyzeLoop(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip);
    bool isLoopInvariant(Value *operand, Loop *const LL, SmallVector<Instruction *, 10> loopInvariantInstructions){
        if(dyn_cast<Constant>(operand) || dyn_cast<Argument>(operand))
            return true;

        if (Instruction *I = dyn_cast<Instruction>(operand)) {
            // Da chiedere a lezione: dominance tree su istruzioni.
            if (!LL->contains(I->getParent()))
                return true;

            if(dyn_cast<PHINode>(operand))
                return false; // Da chiedere a lezione.
            
            for (Instruction *InvInst : loopInvariantInstructions)
                if (InvInst == I)
                    return true;
        }

        return false;
    }

    void findLoopInvariant(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip){
        for(Loop *const SubLL:LL->getSubLoops())
            analyzeLoop(SubLL, toSkip);

        SmallVector<Instruction *, 10> loopInvariantInstructions;
        for(BasicBlock *const BB:LL->getBlocks()){
            if(toSkip.count(BB) != 0) continue;

            for(auto i = BB->begin(); i != BB->end(); ++i){
                Instruction &I = *i;

                bool isInvariant = true;
                for(Value* op : I.operands())
                    if(!isLoopInvariant(op, LL, loopInvariantInstructions)){
                        isInvariant = false;
                        break;
                    }
                
                
                if(isInvariant){
                    loopInvariantInstructions.push_back(&I);
                    errs() << "Is invariant\n";
                }
            }
        }
    }

    void codeMotion(Loop *const LL){

    }

    void analyzeLoop(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip){
        findLoopInvariant(LL, toSkip);
        codeMotion(LL);

        for(BasicBlock *const BB:LL->getBlocks())
            toSkip.insert(BB);
        
    }

    // New PM implementation
    struct LoopInvariantOptPass : PassInfoMixin<LoopInvariantOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM)
        {
            LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
            for(auto &LL:LI){
                SmallPtrSet<BasicBlock *, 10> vec;
                analyzeLoop(LL, vec);
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
llvm::PassPluginLibraryInfo getLoopInvariantOptPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "LoopInvariantOptPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "loop-invariant-opt")
                        {
                            FPM.addPass(LoopInvariantOptPass());
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
    return getLoopInvariantOptPassPluginInfo();
}
