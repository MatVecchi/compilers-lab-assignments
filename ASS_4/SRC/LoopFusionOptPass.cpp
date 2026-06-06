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
    /**
     * Funzione che verifica se nel preheader del loop passato come argomento sono presenti altre istruzioni oltre alla
     * unconditioned branch instruction che porta all'header del loop.
     * Ritorna true nel caso in cui non ci sono altre istruzioni, false altrimenti.
     * 
     * Per poter verificare la condizione si verifica che l'istruzione terminatore del preheader sia un
     * branch incondizionato verso l'header del loop e e che il terminatore sia anche la prima istruzione (front)
     * dell preheader
     */
    bool areNoPreHeaderInstruction(Loop* loop){
        BasicBlock *preHeader = loop->getLoopPreheader();
        if (!preHeader) return false;

        // Ottengo il terminatore
        Instruction *terminator = preHeader->getTerminator();
        if (auto *branchToHeader = dyn_cast<BranchInst>(terminator)) {
            if (branchToHeader->isUnconditional()) {
                // Verifica che salti all'header del loop
                if (branchToHeader->getSuccessor(0) == loop->getHeader()) {
                    
                    // verifico se il terminatore è anche la prima istruzione del preheader
                    if (&preHeader->front() == terminator) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * Funzione helper che, preso un guarded branch e il loop da esso protetto, ritorna il blocco di uscita della guardia.
     * Per poter ottenere il blocco di uscita, verifica quale ramo della branch instruction (successore)
     * porta all'header del loop e prende il successore opposto ad esso.
     * 
     * ex: se il successore 0 porta all'header del loop --> ritorna 1 
     *     se il successore 1 porta all'header del loop --> ritorna 0
     */
    BasicBlock *getExitFromGuard(BranchInst *guardedBranch, Loop *loop){
        BasicBlock *exitGuardedBlock = (guardedBranch->getSuccessor(0) == loop->getLoopPreheader())
                ? guardedBranch->getSuccessor(1)
                : guardedBranch->getSuccessor(0);
        return exitGuardedBlock;
    }

    /**
     * Funzione helper che preso un loop come argomento ritorna la branch instruction relativa alla guardia.
     * (Se esiste)
     *
     * Nota: è stato necessario creare una funzione helper manuale e non utilizzando l'API di LLVM, poichè 
     * LLVM riconosce le guardie solo ed unicamente nei loop ROTATED (dove la condizione di uscita è posta nel latch e 
     * non nell'header).
     * Non avendo trattato i ROTATED loops si è preferito creare una funzione manuale che ricava la guardia da qualsiasi loop 
     * (anche non ruotato).
     * 
     * 
     * Per poter ricavare la guardia si verifica se esiste un unico predecessore del preheader del loop e si cerca di ricavarne una branch instruction 
     * condizionale.
     * L'unico predecessore ci garantisce che tale predecessore (possibile guardia) domini il preheader.
     * Se ci fossero più predecessori si riuscirebbe ad arrivare al preheader da blocchi diversi --> non è protetto direttamente
     * da una guardia.
     * 
     * L'unico predecessore non deve essere a sua volta un header di un altro loop (nel caso di loop innestati c'è la possibilità
     * che la branch instruction dell'header del loop venga erroneamente riconosciuta come guardia e questo va evitato).
     * 
     * Una volta ottenuto l'unico predecessore del preheader del loop (che non è a sua volta un header di un altro loop padre)
     * si prende l'istruzione terminatore di tale predecessore e si verifica se è una conditional branch instruction.
     * Se è un branch condizionale --> si è trovata la guarded branch instruction (guardia)
     */
    BranchInst *getManualLoopGuard(Loop *L, LoopInfo &LI) {
        // estraggo il preheader del loop
        BasicBlock *preheader = L->getLoopPreheader();
        if (!preheader)
            return nullptr;

        // verifico se ha un unico predecessore (la guardia deve dominare il preheader)
        BasicBlock *pred = preheader->getSinglePredecessor();
        if (!pred)
            return nullptr;

        /**
         * Il predecessore deve avere una conditional branch instruction come terminatore, ma questa caratteristica è
         * presente anche negli header dei loop.
         * Quindi se si sta analizzando nested loop, c'è la posssibilità che il predecessore del preheader del nested loop sia 
         * l'header del loop padere, il quale verrebbe erroneamente riconosciuto come guardia.
         * 
         * ex:
         * for(int i=0; i<N; i++){
         *      for(int j=0; j<N; j++){
         *          ...
         *      }
         * }
         * 
         * Il preheader del nested loop ha come unico predecessore l'header del padre, che ha come terminatore una
         * branch instruction condizionale (potrebbe essere erroneamente scambiata come guardia).
         * 
         * Per evitare questa situazione si verifica che il predecessore non sia a sua volta l'header del loop padre.
         */
        if (Loop *PredLoop = LI.getLoopFor(pred)) {
            if (PredLoop->getHeader() == pred)
                return nullptr;
        }

        // ricavo la branch instruction dal terminatore del predecessore
        auto *BI = dyn_cast<BranchInst>(pred->getTerminator());
        if (!BI || !BI->isConditional())
            return nullptr;


        // verifico se tale conditional branch instruction porta effettivamente al preheader del loop (controllo ausiliare)
        if (BI->getSuccessor(0) != preheader &&
            BI->getSuccessor(1) != preheader)
            return nullptr;

        return BI;
    }



    /**
     * Funzione che verifica l'adiacenza fra una coppia di loop presi come argomento.
     * Le condizioni di adiacenza sono le seguenti:
     * 
     * PER LOOP NON GUARDED
     *  - l'exit block del primo loop deve coincidere con il preheader del secondo loop
     * 
     *  - il preheader del secondo loop non deve avere istruzioni aggiuntive oltre che alla
     *      branch incondizionata verso l'header del loop.
     * 
     * 
     * PER LOOP GUARDED
     *  - l'exit block della prima guardia deve coincidere esattamente con la seconda guardia.
     * 
     *  - Nella seconda guardia non ci devono essere istruzioni aggiuntive oltre che ad una compare ed una
     *      conditional branch instruction (che porta all'exit del loop o al suo preheader)
     * 
     *  - il preheader del secondo loop non deve avere istruzioni aggiuntive oltre che alla
     *      branch incondizionata verso l'header del loop.
     * 
     */
    bool verifyAdjacentLoops(Loop* first, Loop* second, LoopInfo &LI){

        // provo ad estrarre la guardia del primo loop
        if(auto *firstGuarded = getManualLoopGuard(first, LI)){

            // il primo loop è guarded e ne ricavo la branch instruction con un dynamic cast
            errs() << "First loop is guarded\n";
            if(BranchInst *firstGuardedBranch = dyn_cast<BranchInst>(firstGuarded)){
                BasicBlock *exitFirstGuardedBlock = getExitFromGuard(firstGuardedBranch, first);
                
                // verifico se anche il secondo loop è guarded 
                if(auto *secondLoopGuarded = getManualLoopGuard(second, LI)){
                    if(BranchInst *secondLoopGuardedBranch = dyn_cast<BranchInst>(secondLoopGuarded)){

                        // il secondo loop è guarded e ne ho ricavato la branch instruction con un dynamic cast
                        errs() << "Second loop is guarded" << "\n";

                        // ricavo il blocco in cui è contenuta la guarded branch del secondo loop
                        BasicBlock *secondLoopGuardedBlock = secondLoopGuardedBranch->getParent();

                        // ricavo la prima sitruzione del blocco di guardia del secondo loop
                        Instruction *firstSecondLoopGuardInstruction = &secondLoopGuardedBlock->front();
                        
                        // valore booleano che verifica se il blocco di guardia del secondo loop non contiene istruzioni aggiuntive
                        // ovvero contiene solo un conditional branch o sia cun compare che un conditional branch
                        bool compareAndBranch = ( isa<CmpInst>(firstSecondLoopGuardInstruction) && (firstSecondLoopGuardInstruction->getNextNode() == secondLoopGuardedBranch) );
                        bool directBranch = ( secondLoopGuardedBranch == firstSecondLoopGuardInstruction );

                        if( !compareAndBranch && !directBranch)
                            return false;

                        if(secondLoopGuardedBlock != exitFirstGuardedBlock)
                            return false;
                        
                        // la seconda guardia non ha istruzioni aggiuntive e le due guardie sono correttamente conllegate 
                        errs() << "Guard connected !\n";

                        // verifico se ci sono istruzioni ausiliari nel preheader e ritorno il risultato
                        return areNoPreHeaderInstruction(second);
                    }
                }
            }
            return false;
        }else{

            // il primo loop non è guarded

            BasicBlock *exitLoopBlock = first->getExitBlock();

            BasicBlock *secondLoopPreHeader = second->getLoopPreheader();
            if( secondLoopPreHeader != exitLoopBlock)
                return false;
            
            return areNoPreHeaderInstruction(second);
        }
    }


    bool verifyControlFlowEquivalence(Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, LoopInfo &LI){
        BranchInst* firstGuardBranch = getManualLoopGuard(first, LI);
        BranchInst* secondGuardBranch = getManualLoopGuard(second, LI);
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

    bool bypassSecondGuard(Loop* first, Loop* second, LoopInfo &LI){
        BranchInst *firstGuardedBranch = getManualLoopGuard(first, LI);
        BranchInst *secondGuardedBranch = getManualLoopGuard(second, LI);

        if(!firstGuardedBranch || !secondGuardedBranch)
            return true;

        
        BasicBlock *exitSecondGuardedBlock = getExitFromGuard(secondGuardedBranch, second);
        unsigned int firstExitSuccessor = firstGuardedBranch->getSuccessor(0) == first->getLoopPreheader()? 1:0;
        firstGuardedBranch->setSuccessor(firstExitSuccessor, exitSecondGuardedBlock);
        return true;
    }


    bool LoopFusion(LoopInfo &LI, Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, DependenceInfo &DI, Function &F){
        if(!verifyAdjacentLoops(first, second, LI)){
            errs() << first->getName() << " and " << second->getName() << " are NOT adjacent loops\n";
            return false;
        }    
        errs() << first->getName() << " and " << second->getName() << " are adjacent loops\n";

        if(!verifyControlFlowEquivalence(first, second, DT, PDT, SE, LI)){
            errs() << first->getName() << " and " << second->getName() << " are NOT control flow equivalent \n";
            return false;
        }   
        errs() << first->getName() << " and " << second->getName() << " are control flow equivalent \n";  

        if(!verifySameTripCount(first, second, SE)){
            errs() << first->getName() << " and " << second->getName() << " DON'T have the same trip count \n";
            return false;
        }   
        errs() << first->getName() << " and " << second->getName() << " have the same trip count \n";
        
        if(!verifyDependencies(first, second, DI, SE)){
            errs() << first->getName() << " and " << second->getName() << " have at least one backward dependence\n";
            return false;
        }
        errs() << first->getName() << " and " << second->getName() << " can be fused \n";


        PHINode* firstIV = getLoopInductionVariable(first, SE);
        PHINode* secondIV = getLoopInductionVariable(second, SE);

        if(!bodyConnect(first, second)) return false;
        if(!headerExitConnect(first, second)) return false;
        if(!secondLoopHeaderToLatch(second)) return false;
        if(!bypassSecondGuard(first, second, LI)) return false;

        inductionVariableFusion(firstIV, secondIV, SE);

        EliminateUnreachableBlocks(F);

        return true;
    }
//-----------------------------------------------------------------------------
// Loop Fusion (LICM) pass

// --> è un passo di trasformazione utilizzato per la loop fusion
//-----------------------------------------------------------------------------
    struct LoopFusionOptPass : PassInfoMixin<LoopFusionOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM){
            bool fused = true;

            while (fused) {
                LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
                DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
                PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
                ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
                DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

                bool fused = false;

                DenseMap<Loop*, SmallVector<Loop*, 10>> LoopSiblingsMap;

                for (Loop *LL : LI.getLoopsInPreorder()) {
                    LoopSiblingsMap[LL->getParentLoop()].push_back(LL);
                }

                for (auto &[FatherLoop, LoopSiblings] : LoopSiblingsMap) {
                    if (fused)
                        break;

                    for (unsigned i = 0; i + 1 < LoopSiblings.size(); ++i) {
                        Loop *First = LoopSiblings[i];
                        Loop *Second = LoopSiblings[i + 1];

                        if (LoopFusion(LI, First, Second, DT, PDT, SE, DI, F)) {
                            fused = true;
                            break;
                        }
                    }
                }

                if (!fused)
                    break;

                EliminateUnreachableBlocks(F);

                AM.invalidate(F, PreservedAnalyses::none());
            }


            return PreservedAnalyses::all();
        }

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
