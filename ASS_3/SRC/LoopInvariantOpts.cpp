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
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;
namespace
{
    void analyzeLoop(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip, DominatorTree &DT);
    bool isLoopInvariantInstruction(Instruction *I, Loop *LL,  SmallVector<Instruction *, 10> &loopInvariantInstructions);

    bool isLoopInvariantOperand(Value *operand, Loop *const LL, SmallVector<Instruction *, 10> &loopInvariantInstructions){
        if(dyn_cast<Constant>(operand) || dyn_cast<Argument>(operand))
            return true;

        if (Instruction *I = dyn_cast<Instruction>(operand)) {
            // Da chiedere a lezione: dominance tree su istruzioni.
            if (!LL->contains(I->getParent()))
                return true;

            if(dyn_cast<PHINode>(operand))
                return false; // Da chiedere a lezione.
            
            return isLoopInvariantInstruction(I, LL, loopInvariantInstructions);
        }

        return false;
    }

    // dividi la isLoopInvariant da funzione ad operando, una richiama l'altra

    bool isLoopInvariantInstruction(Instruction *I, Loop *LL,  SmallVector<Instruction *, 10> &loopInvariantInstructions){
        for (Instruction *InvInst : loopInvariantInstructions)
                if (InvInst == I)
                    return true;

        for(Value* op : I->operands())
            if(!isLoopInvariantOperand(op, LL, loopInvariantInstructions)){
                return false;
            }
        return true;
    }

    void findLoopInvariant(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip,  SmallVector<Instruction *, 10> &loopInvariantInstructions, DominatorTree &DT){
       
        for(BasicBlock *const BB:LL->blocks()){
            if(toSkip.count(BB) != 0) continue;

            for(auto i = BB->begin(); i != BB->end(); ++i){
                Instruction &I = *i;

                if(isLoopInvariantInstruction(&I, LL, loopInvariantInstructions)){
                    loopInvariantInstructions.push_back(&I);
                    errs() << I << " Is invariant\n";
                }
            }
        }
    }







    bool contains(SmallVector<Instruction *, 10> vec, Instruction *I){
        for(Instruction *i: vec)
            if(I == i)
                return true;
        return false;
    }

    bool hasDependencies(Instruction* LIInstr, SmallVector<Instruction *, 10> codeMotionInstructions){
        for(Value* operand: LIInstr->operands()){
            if( auto I = dyn_cast<Instruction>(operand))
                    if(contains(codeMotionInstructions, I))
                        return true;
        }
        return false;
    }

    bool verifyDominance(Instruction* LIInstr, SmallVector<BasicBlock *, 10> successorsBlocks, DominatorTree &DT){
        BasicBlock *LIIBasicBlock = LIInstr->getParent();
        for(BasicBlock *exitBlock: successorsBlocks){
            if(!DT.dominates(LIIBasicBlock, exitBlock ) ){
                return false;
            }
        }
        return true;
    }


    bool verifyDeadCode(Instruction* LIInstr, DominatorTree &DT, SmallVector<BasicBlock *, 10> successorsBlocks){
        for (auto &U : LIInstr->uses()) {
            User *user = U.getUser(); 
            
            if (Instruction *I = dyn_cast<Instruction>(user)) {
                for(BasicBlock *exitBlock: successorsBlocks){
                    if(DT.dominates(exitBlock, I->getParent()))
                        return false;                    
                }
            }else
                return false; 
        }
        return true;
    }


    void verifyCodeMotion(Loop *const LL,  SmallVector<Instruction *, 10> &loopInvariantInstructions, DominatorTree &DT, SmallVector<Instruction *, 10> &codeMotionInstructions){
        SmallVector<BasicBlock *, 10> successorsBlocks;
        LL->getExitBlocks(successorsBlocks);

        for(Instruction *LIInstr: loopInvariantInstructions ){

            if(verifyDominance(LIInstr, successorsBlocks, DT)){
                codeMotionInstructions.push_back(LIInstr);
                continue;
            }
            
            if(verifyDeadCode(LIInstr, DT, successorsBlocks))
                codeMotionInstructions.push_back(LIInstr);
        }
    }


    void moveInstructions(SmallVector<Instruction *, 10> &codeMotionInstructions, Loop *LL){

        Instruction *preHeaderTerminator = (LL->getLoopPreheader())->getTerminator();

        while(!codeMotionInstructions.empty()){
            for(Instruction* moveableInstruction: codeMotionInstructions){
                if(!hasDependencies(moveableInstruction,  codeMotionInstructions)){
                    moveableInstruction->removeFromParent();
                    moveableInstruction->insertBefore(preHeaderTerminator);
                }
            }
        } 
    }

    void analyzeLoop(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip, DominatorTree &DT){
        SmallVector<Instruction *, 10> loopInvariantInstructions;
        SmallVector<Instruction *, 10> codeMotionInstructions;


        // Analisi dei loop interni in post order
        for(Loop *const SubLL:LL->getSubLoops())
            analyzeLoop(SubLL, toSkip, DT);

        findLoopInvariant(LL, toSkip, loopInvariantInstructions, DT);
        verifyCodeMotion(LL, loopInvariantInstructions, DT, codeMotionInstructions);

        for(Instruction *i: codeMotionInstructions)
            errs() << *i << " is Movable\n";

        moveInstructions(codeMotionInstructions, LL);

    
        for(BasicBlock *const BB:LL->blocks())
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
            DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

            for(auto &LL:LI){
                SmallPtrSet<BasicBlock *, 10> vec;
                analyzeLoop(LL, vec, DT);
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
