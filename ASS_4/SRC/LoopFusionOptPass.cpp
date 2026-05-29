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
#include "llvm/Analysis/DependenceAnalysis.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;
namespace
{
    
    bool areNoPreHeaderInstruction(Loop* loop){
        BasicBlock *preHeader = loop->getLoopPreheader();
        Instruction &firstPreHInstruction = preHeader->front();

        if( auto* branchToHeader = dyn_cast<BranchInst>(&firstPreHInstruction)){
            if (branchToHeader->isUnconditional()) {
                BasicBlock *possibleHeader = branchToHeader->getSuccessor(0);
                if(possibleHeader == loop->getHeader())
                    return true;
            }
        }
        return false;
    }

    bool verifyAdjacentLoops(Loop* first, Loop* second, LoopInfo &LI){
        if(auto *firstGuarded = first->getLoopGuardBranch()){
            errs() << "First loop is guarded\n";
            if(BranchInst *firstGuardedBranch = dyn_cast<BranchInst>(firstGuarded)){
                BasicBlock *exitFirstGuardedBlock = (firstGuardedBranch->getSuccessor(0) == first->getLoopPreheader())
                ? firstGuardedBranch->getSuccessor(1)
                :firstGuardedBranch->getSuccessor(0);
                
                if(auto *secondLoopGuarded = second->getLoopGuardBranch()){
                    if(BranchInst *secondLoopGuardedBranch = dyn_cast<BranchInst>(secondLoopGuarded)){
                        BasicBlock *secondLoopGuardedBlock = secondLoopGuardedBranch->getParent();
                        if(secondLoopGuardedBlock != exitFirstGuardedBlock)
                            return false;
                        
                        return areNoPreHeaderInstruction(second);
                    }
                }
            }
            return false;
        }else{
            BasicBlock *exitLoopBlock = first->getExitBlock();

            BasicBlock *secondLoopPreHeader = second->getLoopPreheader();
            if( secondLoopPreHeader != exitLoopBlock)
                return false;
            
            return areNoPreHeaderInstruction(second);
        }
    }


    bool verifyControlFlowEquivalence(Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE){
        bool guarded = first->isGuarded() && second->isGuarded();
        
        if(guarded){
            // verifico che la veridicità della prima istruzione implica la veridicità della seconda
            // ex: se la prima è n>10 e la seconda n>5 --> allora è true.
            BasicBlock *firstGuard = first->getLoopGuardBranch()->getParent();
            BasicBlock *secondGuard = second->getLoopGuardBranch()->getParent();

            if(!DT.dominates(firstGuard, secondGuard) ||  !PDT.dominates(secondGuard, firstGuard) )
                return false;
                
            Value* firstGuardedBranchCondition = first->getLoopGuardBranch()->getCondition();
            Value* secondGuardedBranchCondition = second->getLoopGuardBranch()->getCondition(); 

            auto *firstCompareInstruction = dyn_cast<ICmpInst>(firstGuardedBranchCondition);
            auto *secondCompareInstruction = dyn_cast<ICmpInst>(secondGuardedBranchCondition);

            if(!firstCompareInstruction || !secondCompareInstruction)
                return false;

            
            auto secondPred = secondCompareInstruction->getPredicate();
            Value *secondLeftHandSide = secondCompareInstruction->getOperand(0); 
            Value *secondRightHandSide = secondCompareInstruction->getOperand(1);

            const SCEV *SCEVsecondLeftHandSide = SE.getSCEV(secondLeftHandSide);
            const SCEV *SCEVsecondRightHandSide = SE.getSCEV(secondRightHandSide);

            /* 
            auto firstPred = firstCompareInstruction->getPredicate();
            Value *firstLeftHandSide = firstCompareInstruction->getOperand(0); 
            Value *firstRightHandSide = firstCompareInstruction->getOperand(1);

            const SCEV *SCEVfirstLeftHandSide = SE.getSCEV(firstLeftHandSide);
            const SCEV *SCEVfirstRightHandSide = SE.getSCEV(firstRightHandSide);

            const SCEV* firstOperandDIFF = SE.getMinusSCEV(
                SCEVfirstLeftHandSide,
                SCEVfirstRightHandSide
            );

            const SCEV* secondOperandDIFF = SE.getMinusSCEV(
                SCEVsecondLeftHandSide,
                SCEVsecondRightHandSide
            );

            if(firstOperandDIFF == secondOperandDIFF && firstPred == secondPred)
                return true;

            */

            /**
             * Il contesto è un blocco che si prende in considerazione per la verifica della implicazione della 
             * prima guardia sulla seconda
             * Per ipotesi si suppone che si passi dal contesto, se si passa dal contesto si passa anche da tutti i suoi dominatori.
             * se tra i dominatori compare una condizione, allora si sa che quella condizione era per forza vera o per forza falsa (dipende dal punto del contesto).
             * In questo modo si considera quindi l'ipotesi iniziale su cui si vuole verificare l'implicazione.
             * --> il contesto mi capisce quale condizione considerare (risale il dom tree) e la sua condizione iniziale su cui si vuole verificare 
             * l'inferenza 
             * 
             * poi il metodo is knwon predicate at verifica l'effettiva implicazione tra le due codnizioni identificate
             */
            Instruction *CtxI = &first->getLoopPreheader()->front();
            
            errs() << SE.isKnownPredicateAt(secondPred, SCEVsecondLeftHandSide, SCEVsecondRightHandSide, CtxI) <<"\n";
            if (!SE.isKnownPredicateAt(secondPred, SCEVsecondLeftHandSide, SCEVsecondRightHandSide, CtxI))
                return false;
            return true;
             
        }
        return DT.dominates(first->getHeader(), second->getHeader()) && PDT.dominates(second->getHeader(), first->getHeader());
    }



    bool verifySameTripCount(Loop* first, Loop* second, ScalarEvolution &SE){
        // Da chiedere a lezione per la gestione degli overflow
        return SE.getBackedgeTakenCount(first) == SE.getBackedgeTakenCount(second);
    }

    SmallVector<Instruction *, 10> getLoadStore(Loop* LL){
        SmallVector<Instruction *, 10> loadStore;
        for(auto *BB: LL->getBlocks()){
            for(auto &I: *BB){
                if(auto *loadInstr = dyn_cast<LoadInst>(&I))
                    loadStore.push_back(loadInstr);
                if(auto *storeInstr = dyn_cast<StoreInst>(&I))
                    loadStore.push_back(storeInstr);
            }
        }
        return loadStore;
    }

    const SCEV* getBaseIteration(const SCEV *S){
        if(auto *pointerIterationRange = dyn_cast<SCEVAddRecExpr>(S))
            return pointerIterationRange->getStart();
        return S;
    }

    bool verifyDependencies(Loop* first, Loop* second, DependenceInfo &DI, ScalarEvolution &SE){
        SmallVector<Instruction *, 10> firstLS = getLoadStore(first);
        SmallVector<Instruction *, 10> secondLS = getLoadStore(second);

        unsigned currLevel = first->getLoopDepth();
        for(Instruction *firstInst: firstLS){
            for(Instruction* secondInst: secondLS){

                // Verifico dipendenza
                auto dep = DI.depends(firstInst, secondInst, true);
                if(!dep) continue;

                // Verifico impossibilità di analisi (confused)
                if(dep->isConfused()) return false;

                // read e read sono sempre safe
                if(isa<LoadInst>(firstInst) && isa<LoadInst>(secondInst))
                    continue;
                

                // estraggo come il primo e il secondo ciclo accedono alla struttura dati utilizzando gli address e gli offset
                // prendo la prima istruzione (base o start come riferimento per il calcolo)
                const SCEV *firstLoopIteration = getBaseIteration( SE.getSCEV(getPointerOperand(firstInst)));
                const SCEV *secondLoopIteration = getBaseIteration( SE.getSCEV(getPointerOperand(secondInst)));

                // calcolo il delta come secondo - primo --> se questo è positivo significa che è una backward direction
                // offset della seconda > della prima
                const SCEV *delta = SE.getMinusSCEV(secondLoopIteration, firstLoopIteration);

                errs() << *firstInst <<" --> loop data space pattern: " << *firstLoopIteration << "\n";
                errs() << *secondInst << " --> loop data space pattern: " << *secondLoopIteration << "\n";

                errs() << "Delta: " << *delta << "\n";
                if(SE.isKnownPositive(delta))
                    return false;

                /*
                if(!dep->isConfused()){
                    unsigned Dir = dep->getDirection(currLevel, true);
                    if(Dir & Dependence::DVEntry::GT)
                        return false;
                    continue;
                }
                */

            }
        }
        return true;
    }



    void LoopFusion(LoopInfo &LI, SmallVector<Loop* , 10> siblingLoops, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, DependenceInfo &DI){
        
        for(auto i=0; i<siblingLoops.size()-1; i++){
            Loop* first = siblingLoops[i];
            Loop* second = siblingLoops[i+1];

            if(!verifyAdjacentLoops(first, second, LI))
                continue;
            errs() << first->getName() << " and " << second->getName() << " are adjacent loops\n";

            if(!verifyControlFlowEquivalence(first, second, DT, PDT, SE))
                continue;
            errs() << first->getName() << " and " << second->getName() << " are control flow equivalent \n";
            
            if(!verifySameTripCount(first, second, SE))
                continue;
            errs() << first->getName() << " and " << second->getName() << " have the same trip count \n";
            
            if(!verifyDependencies(first, second, DI, SE))
                continue;
            errs() << first->getName() << " and " << second->getName() << " can be fused \n";
            
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
            DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

            DenseMap<Loop*, SmallVector<Loop*, 10>> LoopSiblingsMap;
            
            for (auto *LL : LI.getLoopsInPreorder()){
                LoopSiblingsMap[LL->getParentLoop()].push_back(LL);
            }

            
            for(auto &[fatherLoop, LoopSiblings]: LoopSiblingsMap){
                if(fatherLoop)
                    errs() << "Father loop: "<<fatherLoop->getName()<<"\n";
                else
                    errs() << "Top level loop \n";
                LoopFusion(LI, LoopSiblings, DT, PDT, SE, DI);
            }
         
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
