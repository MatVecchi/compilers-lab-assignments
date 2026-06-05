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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;
namespace
{
    
    bool areNoPreHeaderInstruction(Loop* loop){
        BasicBlock *preHeader = loop->getLoopPreheader();
        if (!preHeader) return false;

        // 1. Prendi il terminatore del blocco (il branch finale)
        Instruction *terminator = preHeader->getTerminator();
        if (auto *branchToHeader = dyn_cast<BranchInst>(terminator)) {
            if (branchToHeader->isUnconditional()) {
                // Verifica che salti all'header del loop
                if (branchToHeader->getSuccessor(0) == loop->getHeader()) {
                    
                    // 2. Controlla che il blocco sia effettivamente vuoto.
                    // Un blocco è "vuoto" se la sua prima istruzione coincide con il terminatore
                    // (ignorando le istruzioni di debug intrinseche se presenti).
                    if (&preHeader->front() == terminator) {
                        return true;
                    }
                }
            }
        }
        return false;
    }


    BasicBlock *getExitFromGuard(BranchInst *guardedBranch, Loop *loop){
        BasicBlock *exitGuardedBlock = (guardedBranch->getSuccessor(0) == loop->getLoopPreheader())
                ? guardedBranch->getSuccessor(1)
                : guardedBranch->getSuccessor(0);
        return exitGuardedBlock;
    }

    BranchInst *getManualLoopGuard(Loop *L) {
        BasicBlock *preheader = L->getLoopPreheader();
        if (!preheader) return nullptr;

        // Il blocco immediatamente prima del preheader
        BasicBlock *pred = preheader->getSinglePredecessor();
        if (!pred) return nullptr;

        // Controlliamo se questo blocco finisce con un branch condizionale (il nostro if)
        if (BranchInst *bi = dyn_cast<BranchInst>(pred->getTerminator())) {
            if (bi->isConditional()) {
                return bi; // Trovato! Questa è la nostra guardia manuale
            }
        }
        return nullptr;
    }

    bool verifyAdjacentLoops(Loop* first, Loop* second, LoopInfo &LI){
        if(auto *firstGuarded = getManualLoopGuard(first)){
            errs() << "First loop is guarded\n";
            if(BranchInst *firstGuardedBranch = dyn_cast<BranchInst>(firstGuarded)){
                BasicBlock *exitFirstGuardedBlock = getExitFromGuard(firstGuardedBranch, first);
                
                if(auto *secondLoopGuarded = getManualLoopGuard(second)){
                    if(BranchInst *secondLoopGuardedBranch = dyn_cast<BranchInst>(secondLoopGuarded)){
                        errs() << "Second loop is guarded" << "\n";
                        BasicBlock *secondLoopGuardedBlock = secondLoopGuardedBranch->getParent();
                        Instruction *firstSecondLoopGuardInstruction = &secondLoopGuardedBlock->front();
                        
                        bool compareAndBranch = ( isa<CmpInst>(firstSecondLoopGuardInstruction) && (firstSecondLoopGuardInstruction->getNextNode() == secondLoopGuardedBranch) );
                        bool directBranch = ( secondLoopGuardedBranch == firstSecondLoopGuardInstruction );

                        if( !compareAndBranch && !directBranch)
                            return false;


                        if(secondLoopGuardedBlock != exitFirstGuardedBlock)
                            return false;
                        
                        errs() << "Guard connected !\n";
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
        BranchInst* firstGuardBranch = getManualLoopGuard(first);
        BranchInst* secondGuardBranch = getManualLoopGuard(second);
        bool guarded = (firstGuardBranch && secondGuardBranch);

        if(guarded){
            errs() << "are guarded\n";
            // verifico che la veridicità della prima istruzione implica la veridicità della seconda
            // ex: se la prima è n>10 e la seconda n>5 --> allora è true.
            BasicBlock *firstGuard = firstGuardBranch->getParent();
            BasicBlock *secondGuard = secondGuardBranch->getParent();

            if(!DT.dominates(firstGuard, secondGuard) ||  !PDT.dominates(secondGuard, firstGuard) )
                return false;
                
            Value* firstGuardedBranchCondition = firstGuardBranch->getCondition();
            Value* secondGuardedBranchCondition = secondGuardBranch->getCondition(); 

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
            Instruction *CtxI = firstGuardBranch;
            
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

    SmallVector<Instruction *, 10> getLoadStore(Loop* LL, bool getLoad){
        SmallVector<Instruction *, 10> loadStore;
        for(auto *BB: LL->getBlocks()){
            for(auto &I: *BB){
                if(getLoad){
                    if(auto *loadInstr = dyn_cast<LoadInst>(&I))
                        loadStore.push_back(loadInstr);
                }
                if(auto *storeInstr = dyn_cast<StoreInst>(&I))
                    loadStore.push_back(storeInstr);
            }
        }
        return loadStore;
    }

    

    bool verifyDependencies(Loop* first, Loop* second, DependenceInfo &DI, ScalarEvolution &SE){
        SmallVector<Instruction *, 10> firstLS = getLoadStore(first, false);
        SmallVector<Instruction *, 10> secondLS = getLoadStore(second, true);

        
        for(Instruction *firstInst: firstLS){
            for(Instruction* secondInst: secondLS){

                // Verifico dipendenza
                auto dep = DI.depends(firstInst, secondInst, true);
                if(!dep) continue;

                // Verifico impossibilità di analisi (confused)
                if(dep->isConfused()) return false;


                // estraggo come il primo e il secondo ciclo accedono alla struttura dati utilizzando gli address e gli offset
                // prendo la prima istruzione (base o start come riferimento per il calcolo)
                const SCEV *firstLoopIteration = SE.getSCEVAtScope(getPointerOperand(firstInst), first->getParentLoop());
                const SCEV *secondLoopIteration =  SE.getSCEVAtScope(getPointerOperand(secondInst), second->getParentLoop());

                // calcolo il delta come secondo - primo --> se questo è positivo significa che è una backward direction
                // offset della seconda > della prima
                const SCEV *delta = SE.getMinusSCEV(secondLoopIteration, firstLoopIteration);
                //errs() << "First pointer: "<< *getPointerOperand(firstInst) << "\n";
                //errs() << "Second pointer: "<< *getPointerOperand(secondInst) << "\n";
/*                 errs() << *firstInst <<" --> loop data space pattern: " << *firstLoopIteration << "\n";
                errs() << *secondInst << " --> loop data space pattern: " << *secondLoopIteration << "\n";

                errs() << "Delta: " << *delta << "\n\n"; */
                if(SE.isKnownPositive(delta))
                    return false;


            }
        }
        return true;
    }

    

    PHINode* getLoopInductionVariable(Loop *loop, ScalarEvolution &SE) {
        BasicBlock *header = loop->getHeader();

        for (auto &instr : *header) {
            if (auto *phiInstr = dyn_cast<PHINode>(&instr)) {
                const SCEV *scev = SE.getSCEV(phiInstr);

                if (auto *addRec = dyn_cast<SCEVAddRecExpr>(scev)) {
                    if (addRec->getLoop() == loop) {
                        return phiInstr; 
                    }
                }
            }
        }
        return nullptr;
    }

    const SCEV* getInductionVariableDifference(PHINode* firstInductionVariable, PHINode* secondInductionVariable, ScalarEvolution &SE){
        const SCEVAddRecExpr *firstIVSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(firstInductionVariable));
        const SCEVAddRecExpr *secondIVSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(secondInductionVariable));

        errs() << "First Induction variable SCEV" << *firstIVSCEV << "\n";
        errs() << "Second Induction variable SCEV" << *secondIVSCEV << "\n";

        const SCEV *inductionDifference = SE.getMinusSCEV(secondIVSCEV->getStart(), firstIVSCEV->getStart());
        errs() << "Induction variable difference SCEV: " << *inductionDifference << "\n";
    
        return inductionDifference;
    }

    void inductionVariableFusion(PHINode *firstInductionVariable,PHINode *secondInductionVariable, ScalarEvolution &SE){
        if(!firstInductionVariable || !secondInductionVariable)
            return;

        const SCEV *delta = getInductionVariableDifference(firstInductionVariable, secondInductionVariable, SE);
        const SCEVConstant *constant_delta = dyn_cast<SCEVConstant>(delta);
        if(!constant_delta)
            return;
        

        
        Instruction *newInduction = BinaryOperator::CreateAdd(
            firstInductionVariable, 
            constant_delta->getValue(), 
            "fused.iv", 
            firstInductionVariable->getParent()->getFirstNonPHI()
        );
        
        //newInduction->insertAfter(firstInductionVariable);
        secondInductionVariable->replaceAllUsesWith(newInduction);
    }

    bool contains(SmallVector<BasicBlock *, 10> vec, BasicBlock *find){
        for(auto *BB: vec)
            if(find == BB)
                return true;
        return false;
    }


    BasicBlock *getBodyStart(Loop* loop){
        BasicBlock *header = loop->getHeader();
        BranchInst *headerBranch = dyn_cast<BranchInst>(header->getTerminator());
        if(!headerBranch)
            return nullptr;
            
        SmallVector<BasicBlock *, 10> loopExits;
        loop->getExitBlocks(loopExits);
        if(contains(loopExits, headerBranch->getSuccessor(0)))
            return headerBranch->getSuccessor(1);
        return headerBranch->getSuccessor(0);
    }


    Instruction *getTerminatorBodyInstruction(Loop* loop){
        BasicBlock *latchBB = loop->getLoopLatch();
        if(!latchBB)
            return nullptr;
        
        // da sistemare 
        BasicBlock *loopBody = getBodyStart(loop);
        if(latchBB == loopBody)
            return loopBody->getTerminator();

        BasicBlock *boodyLastBB = latchBB->getUniquePredecessor();
        if(!boodyLastBB)
            return nullptr;
        
        Instruction *bodyTerminatorInstr = boodyLastBB->getTerminator();
        return bodyTerminatorInstr;
    }


    bool bodyConnect(Loop *first, Loop *second){
        Instruction *firstBodyTerminatorInstruction = getTerminatorBodyInstruction(first);
        BranchInst *firstTerminatorBodyBranch = dyn_cast<BranchInst>(firstBodyTerminatorInstruction);
        if(!firstTerminatorBodyBranch)
            return false;
        
        BasicBlock *secondBody = getBodyStart(second);
        
        // modifichiamo la branch instruction alla fine del body del primo loop facendo in modo che punti al primo basic
        // block del body del secondo loop
        firstTerminatorBodyBranch->setSuccessor(0, secondBody);

        
        Instruction *secondBodyTerminatorInstruction = getTerminatorBodyInstruction(second);
        BranchInst *secondTerminatorBodyBranch = dyn_cast<BranchInst>(secondBodyTerminatorInstruction);
        BasicBlock *firstLatch = first->getLoopLatch();

        

        // modifico la branch instruction terminatrice del primo body facendo in modo che punti al latch del 
        // primo loop
        secondTerminatorBodyBranch->setSuccessor(0, firstLatch);
        
        return true;
    }

    bool headerExitConnect(Loop *first, Loop *second){
        BasicBlock *firstBody = getBodyStart(first);
        if(!firstBody)
            return false;
        
        BasicBlock *firstHeader = first->getHeader();
        BranchInst *firstHeaderBranch = dyn_cast<BranchInst>(firstHeader->getTerminator());

        unsigned int firstHeaderExitSuccessorIndex = firstHeaderBranch->getSuccessor(0) == firstBody ? 1 : 0;
        SmallVector<BasicBlock *, 10> secondLoopExits;
        // c'è solo un exit block perchè se ce ne fossere altri si formerebbe un problema di dominanza e post-dominanza
        // e il trip count non si riuscirebbe a calcolare
        second->getExitBlocks(secondLoopExits);

        // il successore non body dell'header del primo loop diventa il primo (ed unico) successor del secondo loop.
        firstHeaderBranch->setSuccessor(firstHeaderExitSuccessorIndex, secondLoopExits[0] );

        return true;
    }

    bool secondLoopHeaderToLatch(Loop *loop) {
        BasicBlock *header = loop->getHeader();
        BasicBlock *latch = loop->getLoopLatch();

        // da sistemare
        if(!latch)
           return true;

        BranchInst *headerBranch =
            dyn_cast_or_null<BranchInst>(header->getTerminator());

        if (!headerBranch)
            return false;

        headerBranch->eraseFromParent();
        BranchInst::Create(latch, header);

        return true;
    }

    bool bypassSecondGuard(Loop* first, Loop* second){
        BranchInst *firstGuardedBranch = getManualLoopGuard(first);
        BranchInst *secondGuardedBranch = getManualLoopGuard(second);

        if(!firstGuardedBranch || !secondGuardedBranch)
            return true;

        
        BasicBlock *exitSecondGuardedBlock = getExitFromGuard(secondGuardedBranch, second);
        unsigned int firstExitSuccessor = firstGuardedBranch->getSuccessor(0) == first->getLoopPreheader()? 1:0;
        firstGuardedBranch->setSuccessor(firstExitSuccessor, exitSecondGuardedBlock);
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


            PHINode* firstIV = getLoopInductionVariable(first, SE);
            PHINode* secondIV = getLoopInductionVariable(second, SE);

            if(!bodyConnect(first, second))
                continue;
            if(!headerExitConnect(first, second))
                continue;
            
            if(!secondLoopHeaderToLatch(second))
                continue;
            
            if(!bypassSecondGuard(first, second))
                continue;

            
            inductionVariableFusion(firstIV, secondIV, SE);
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
                    errs() << "Not top level \n";
                else
                    errs() << "Top level loop \n";
                LoopFusion(LI, LoopSiblings, DT, PDT, SE, DI);
                EliminateUnreachableBlocks(F);
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
