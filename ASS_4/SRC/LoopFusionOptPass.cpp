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
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;
namespace
{
    
    Loop* getAdjacentLoop(Loop* LL, LoopInfo &LI, SmallVector<Loop* , 10> levelLoops){
        if(auto *guardedBranch = dyn_cast<CondBrInst>(LL->getLoopGuardBranch())){
            BasicBlock exitGuardedBlock = guardedBranch->getSuccessor(0) == LL->getLoopPreheader()? guardedBranch->getSuccessor(1):guardedBranch->getSuccessor(0);
            for(Loop* otherLoop: levelLoops){
                if(LL == otherLoop)
                    continue;
                
                if(auto *otherLoopGuardedBranch = dyn_cast<CondBrInst>(otherLoop->getLoopGuardBranch())){
                    BasicBlock otherLoopGuardedBlock = otherLoop->getParent();
                    if(otherLoopGuardedBlock == exitGuardedBlock)
                        return otherLoop;
                }
            }
            return nullptr;
        }else{
            BasicBlock exitLoopBlock = LL->getExitBlock();

            for(Loop* otherLoop: levelLoops){
                if(LL == otherLoop)
                    continue;

                BasicBlock otherLoopPreHeader = otherLoop->getLoopPreheader();
                if( otherLoopPreHeader != exitLoopBlock)
                    continue;
                
                Instruction* firstInstruction = otherLoopPreHeader.front();
                if( auto* possibleBranchToHeader = dyn_cast<UncondBrInst>(firstInstruction)){
                    BasicBlock possibleHeader = possibleBranchToHeader->getSuccessor();
                    if(possibleHeader == otherLoop->getHeader())
                        return otherLoop;
                    return nullptr;
                }else
                    return nullptr;
                
            }
            return nullptr;
        }
    }


    bool verifyControlFlowEquivalence(Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE){
        bool guarded = first->isGuarded() && second->isGuarded();
        
        if(guarded){
            // verifico che la veridicità della prima istruzione implica la veridicità della seconda
            // ex: se la prima è n>10 e la seconda n>5 --> allora è true.

            BranchInst* firstGuardedBranchCondition = first->getLoopGuardBranch(); // FoundCondValue
            Value* secondGuardedBranchCondition = second->getLoopGuardBranch()->getCondition();

            if (auto *secondCompareInstruction = dyn_cast<ICmpInst>(secondGuardedBranchCondition)) {
                auto* pred = cmpInst->getPredicate();
                Value *leftHandSide = cmpInst->getOperand(0); 
                Value *rightHandSide = cmpInst->getOperand(1);

                const SCEV *SCEVleftHandSide = SE->getSCEV(leftHandSide);
                const SCEV *SCEVrightHandSide = SE->getSCEV(rightHandSide);

                // SE->isImpliedCond(ICmpInst::Predicate Pred, const SCEV *LHS, const SCEV *RHS, const Value *FoundCondValue, bool Inverse = false, const Instruction *Context = nullptr)
                if( !SE->isImpliedCond(pred, SCEVleftHandSide, SCEVrightHandSide ,firstGuardedBranchCondition, false) )
                    return false;
            }
            else return false;
        }

        return DT.dominates(first->getHeader(), second->getHeader()) && PDT->dominates(second->getHeader(), first->getHeader());
    }



    void LoopFusion(LoopInfo &LI, SmallVector<Loop* , 10> levelLoops, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE){
        // verifica la loop fusion su Loops
        for(Loop* LL: levelLoops){
            Loop *adjacentLoop = getAdjacentLoop(LL, LI, levelLoops);
            if(!adjacentLoop)
                continue;
            if(!verifyControlFlowEquivalence(LL, adjacentLoop, DT, PDT, SE))
                continue;
            
        }

        /*for(auto* Loop: worklist){
            LoopFusion(LI, Loop->getSubLoops());
        }*/
    }
//-----------------------------------------------------------------------------
// Loop Fusion (LICM) pass

// --> è un passo di trasformazione utilizzato per la loop fusion
//-----------------------------------------------------------------------------
    struct LoopFusionOptPass : PassInfoMixin<LoopFusionOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM)
        {   
            LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
            DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
            PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
            ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

            for (auto *L : LI)
         
            return PreservedAnalyses::all();
        };

        // Without isRequired returning true, this pass will be skipped for functions
        // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
        // all functions with optnone.
        static bool isRequired() { return true; }
    };
}


// registrazione del passo nel Pass Builder
llvm::PassPluginLibraryInfo getLoopFusionOptPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "LoopFusionOptPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "loop-fusion-opt")
                        {
                            FPM.addPass(LoopFusionOptPass());
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
    return getLoopFusionOptPassPluginInfo();
}
