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
        if( first->isRotatedForm() && second->isRotatedForm() ){
            if(auto *firstGuarded = first->getLoopGuardBranch()){

                // il primo loop è guarded e ne ricavo la branch instruction con un dynamic cast
                errs() << first->getName() << " loop is guarded\n";
                if(BranchInst *firstGuardedBranch = dyn_cast<BranchInst>(firstGuarded)){
                    BasicBlock *exitFirstGuardedBlock = getExitFromGuard(firstGuardedBranch, first);
                    
                    // verifico se anche il secondo loop è guarded 
                    if(auto *secondLoopGuarded = second->getLoopGuardBranch()){
                        if(BranchInst *secondLoopGuardedBranch = dyn_cast<BranchInst>(secondLoopGuarded)){

                            // il secondo loop è guarded e ne ho ricavato la branch instruction con un dynamic cast
                            errs() << second->getName() <<" loop is guarded" << "\n";

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
                            // verifico se ci sono istruzioni ausiliari nel preheader e ritorno il risultato
                            bool noPreheaderInst = areNoPreHeaderInstruction(second);
                            
                            // nel caso dei guarded l'uscita del primo loop non coincide esattamente con l'uscita della guardia
                            // va quindi verificato che l'uscita del primo loop contenga solo una istruzione, che è quella che 
                            // permette di fare un unconditional branch verso l'uscita della guard.
                            // L'unconditional branch viene sempre messo da LLVM, se oltre ad essa è presente anche almeno un'altra
                            // istruzione --> si ritorna false per non adiacenza
                            BasicBlock *firstExitBB = first->getUniqueExitBlock();
                            if(!firstExitBB) return false;
                            bool noExitFromFirstLoopInst = firstExitBB->size() == 1;
                            return noExitFromFirstLoopInst && noPreheaderInst;
                        }
                    }
                }
                return false;
            }
            return false;
        }else{

            // il primo loop non è guarded
            // estraggo l'exit block del primo loop e verifico che questo sia il preheader del secondo loop
            BasicBlock *exitLoopBlock = first->getExitBlock();

            BasicBlock *secondLoopPreHeader = second->getLoopPreheader();
            if( secondLoopPreHeader != exitLoopBlock)
                return false;
            
            // verifico che non ci siano altre istruzioni nel pre-header del secondo loop
            return areNoPreHeaderInstruction(second);
        }
    }

    /*
        Funzione che verifica la Control Flow Equivalence su due loop:

        CASO LOOP GUARDED
        Tramite DominatorTree e PostDominatorTree bisogna controllare che la prima guardia domini la seconda e allo stesso tempo la seconda postdomini la prima.
        Inoltre bisogna verificare che la condizione della prima guardia implichi la condizione della seconda.
        
        CASO LOOP NON GUARDED
        Tramite DominatorTree e PostDominatorTree bisogna controllare che il primo loop domini il secondo e allo stesso tempo il secondo postdomini il primo
    */
    bool verifyControlFlowEquivalence(Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, LoopInfo &LI){
        BranchInst* firstGuardBranch = first->getLoopGuardBranch();
        BranchInst* secondGuardBranch = second->getLoopGuardBranch();
        bool guarded = (firstGuardBranch && secondGuardBranch);

        if(guarded){
            
            // verifico che la veridicità della prima istruzione implica la veridicità della seconda
            // ex: se la prima è n>10 e la seconda n>5 --> allora è true.
            BasicBlock *firstGuard = firstGuardBranch->getParent();
            BasicBlock *secondGuard = secondGuardBranch->getParent();

            // verifico le proprietà di dominanza e post-dominanza
            // la prima guardia deve dominare la seconda e la seconda deve post-dominare la prima
            if(!DT.dominates(firstGuard, secondGuard) ||  !PDT.dominates(secondGuard, firstGuard) )
                return false;
            
            // verifico che la prima guardia implichi la seconda, per prima cosa ricavando le due condizioni
            Value* firstGuardedBranchCondition = firstGuardBranch->getCondition();
            Value* secondGuardedBranchCondition = secondGuardBranch->getCondition(); 

            auto *firstCompareInstruction = dyn_cast<ICmpInst>(firstGuardedBranchCondition);
            auto *secondCompareInstruction = dyn_cast<ICmpInst>(secondGuardedBranchCondition);

            if(!firstCompareInstruction || !secondCompareInstruction)
                return false;

            // estraggo il predicato e il RHV e il LHV della seconda condizione e ne ottengo lo SCEV
            auto secondPred = secondCompareInstruction->getPredicate();
            Value *secondLeftHandSide = secondCompareInstruction->getOperand(0); 
            Value *secondRightHandSide = secondCompareInstruction->getOperand(1);

            const SCEV *SCEVsecondLeftHandSide = SE.getSCEV(secondLeftHandSide);
            const SCEV *SCEVsecondRightHandSide = SE.getSCEV(secondRightHandSide);


            /**
             * Il contesto è una istruzione che si prende in considerazione per la verifica della implicazione della 
             * prima guardia sulla seconda.
             * Il contesto corrisponde allo scope in cui si vuole verificare la veridicità della seconda condizione rispetto alla prima.
             * La funzione isKnownPredicateAt prende in input il contesto e un predicato e verifica se tale predicato è
             * verificato nel contesto passato come argomento.
             */
            Instruction *CtxI = firstGuardBranch;
            
            //errs() << SE.isKnownPredicateAt(secondPred, SCEVsecondLeftHandSide, SCEVsecondRightHandSide, CtxI) <<"\n";
            if (!SE.isKnownPredicateAt(secondPred, SCEVsecondLeftHandSide, SCEVsecondRightHandSide, CtxI))
                return false;
            return true;
             
        }
        
        // verifico che il primo loop domini il secondo e che il secondo loop post-domini il primo
        return DT.dominates(first->getHeader(), second->getHeader()) && PDT.dominates(second->getHeader(), first->getHeader());
    }


    /*
        Funzione per la verifica del Trip Count:
        
        A prescindere dal fatto che i loop siano guarded o meno devono avere lo stesso Trip Count. Per farlo si ottiene il Backedge Taken Count, 
        che nel contesto del Control Flow Graph consiste nel sapere quante volte viene percorso l'arco che dal Latch riporta all'Header.
    */
    bool verifySameTripCount(Loop* first, Loop* second, ScalarEvolution &SE){
        // ottengo i trip count dei due loop
        const SCEV *firstTP = SE.getBackedgeTakenCount(first);
        const SCEV *secondTP = SE.getBackedgeTakenCount(second);

        errs() << "First trip count: " << *firstTP << "\n";
        errs() << "Second trip count: " << *secondTP << "\n";
        // se non sono calcolabili ritorno false
        if(isa<SCEVCouldNotCompute>(firstTP) || isa<SCEVCouldNotCompute>(secondTP))
            return false;
            
        // altrimenti verifico se sono uguali
        return  (firstTP == secondTP || SE.isKnownPredicate(ICmpInst::ICMP_EQ, firstTP, secondTP));
    }

    // Funzone helper che viene utilizzata per ricavare le operazioni di STORE (ed eventualmente anche LOAD) dato un loop preso come argomento
    // getLoad è un flag che indica se includere anche le load.
    SmallVector<Instruction *, 10> getLoadStore(Loop* LL, bool getLoad){
        // creo il vettore di load e store
        SmallVector<Instruction *, 10> loadStore;
        
        // scorro i blocchi e le istruzioni del loop
        for(auto *BB: LL->getBlocks()){
            for(auto &I: *BB){
                
                // aggiungo al vettore le load se richieste
                if(getLoad){
                    if(auto *loadInstr = dyn_cast<LoadInst>(&I))
                        loadStore.push_back(loadInstr);
                }
                
                // aggiungo al vettore le store
                if(auto *storeInstr = dyn_cast<StoreInst>(&I))
                    loadStore.push_back(storeInstr);
            }
        }
        return loadStore;
    }

    
    /*
        Funzione che verifica le dipendenze tra le istruzioni di due loop:

        Tralasciando il caso particolare che si ha confrontando due LOAD, le altre possibili combinazioni di LOAD e STORE possono portare il programma ad uno stato di inconsistenza se confrontato con la sua versione precedente non ottimizzata.
        Per questo motivo si ottengono tutte le STORE del primo loop e le LOAD e le STORE del secondo loop e tramite un Brute Force in cui si escludono i casi in cui le istruzioni non dipendono tra loro, 
        per quelli rimanenti tramite la Scalar Evolution si effettua un controllo tra i Pointer Operand delle due istruzioni per determinare se queste hanno una backwards dependency e quindi rinunciare alla Loop Fusion
    */
    bool verifyDependencies(Loop* first, Loop* second, DependenceInfo &DI, ScalarEvolution &SE){
        // ricavo le store del primo loop e le load & store del secondo loop
        SmallVector<Instruction *, 10> firstLS = getLoadStore(first, false);
        SmallVector<Instruction *, 10> secondLS = getLoadStore(second, true);
        
        // veifico le dipendenze fra le store del primo loop e le load&store del secondo
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
                
                // se il delta è positivo è una backward dependence
                if(SE.isKnownPositive(delta))
                    return false;


            }
        }
        return true;
    }

    /**
     * Funzione helper che ricava la induction variable da un loop.
     * 
     * Nota: è stato necessario creare una funzione manuale per ricavare la induction variable di un Loop perchè 
     * l'API di LLVM getInductionVariable funziona solo per i loop rotated.
     * In alternativa ritorna nullptr.
     * 
     * Prendo il phi node corrispondente alla condizione di ripetizione del loop
     */
    PHINode* getLoopInductionVariable(Loop *loop, ScalarEvolution &SE) {
        BasicBlock *header = loop->getHeader();
        auto *BI = dyn_cast<BranchInst>(header->getTerminator());
        if (!BI || !BI->isConditional())
            return nullptr;

        auto *cmp = dyn_cast<ICmpInst>(BI->getCondition());
        if (!cmp)
            return nullptr;

        for (Value *Op : cmp->operands()) {
            if (auto *PHI = dyn_cast<PHINode>(Op)) {
                if (PHI->getParent() == header){
                    
                    const SCEV* IVRange = SE.getSCEV(PHI);
                    if( auto *range = dyn_cast<SCEVAddRecExpr>(IVRange))
                        if(range->getLoop() == loop)
                            return PHI;
                }
            }
        }

        return nullptr;
    }

    /*
        Funzione helper che ricava la differenza tra due Induction Variable:

        Tramite la Scalar Evolution si effettua una differenza tra due Induction Variable per capire di quanto la seconda Induction Variable si discosta dalla prima.
        Questo ci è particolarmente utile quando bisogna "inserire" la seconda Induction Variable nel primo loop, utilizzando la prima Induction Variable come riferimento.
    */
    const SCEV* getInductionVariableDifference(PHINode* firstInductionVariable, PHINode* secondInductionVariable, ScalarEvolution &SE){
        
        // calcolo i range di valori delle due induction variable esprssi come SCEVAddRecExpr
        const SCEVAddRecExpr *firstIVSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(firstInductionVariable));
        const SCEVAddRecExpr *secondIVSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(secondInductionVariable));

        errs() << "First Induction variable SCEV" << *firstIVSCEV << "\n";
        errs() << "Second Induction variable SCEV" << *secondIVSCEV << "\n";
        
        // calcolo la differenza fra gli "start" (valori iniziali) dei due range ottenuti
        const SCEV *inductionDifference = SE.getMinusSCEV(secondIVSCEV->getStart(), firstIVSCEV->getStart());
        errs() << "Induction variable difference SCEV: " << *inductionDifference << "\n";
    
        return inductionDifference;
    }

    /*
        Funzione che effettua l'inserimento della seconda Induction Variable nel primo loop:
        
        Si inserisce nell'Header del primo loop, esattamente dopo la prima Induction Variable, un operazione di Add tra la prima Induction Variable e il valore calcolato come differenza tra le due Induction Variable
    */
    void inductionVariableFusion(PHINode *firstInductionVariable,PHINode *secondInductionVariable, ScalarEvolution &SE){
        if(!firstInductionVariable || !secondInductionVariable)
            return;
        
        // ottengo la differenza fra le due induction variable con l'omonima funzione
        const SCEV *delta = getInductionVariableDifference(firstInductionVariable, secondInductionVariable, SE);
        
        // estraggo il valore costante
        const SCEVConstant *constant_delta = dyn_cast<SCEVConstant>(delta);
        if(!constant_delta)
            return;
        
        
        // inserisco una operazione di ADD come somma della prima induction variable e il valore costante del delta.
        // il nome della nuova variabile è "fused.iv" e viene posizionata dopo la posizione della first induction variable
        Instruction *newInduction = BinaryOperator::CreateAdd(
            firstInductionVariable, 
            constant_delta->getValue(), 
            "fused.iv", 
            firstInductionVariable->getParent()->getFirstNonPHI()
        );
        
        // rimpiazzio gli usi della seconda induction variable con la nuova variabile inserita
        secondInductionVariable->replaceAllUsesWith(newInduction);
    }

    /*
        Funzione helper che dato uno SmallVector di BasicBlock e un BasicBlock controlla se tale BasicBlock è contenuto nello SmallVector
    */
    bool contains(SmallVector<BasicBlock *, 10> vec, BasicBlock *find){
        for(auto *BB: vec)
            if(find == BB)
                return true;
        return false;
    }

    /*
        Funzione helper che dato un loop ne ricava il suo primo blocco Body
    */
    BasicBlock *getBodyStart(Loop* loop){
        if(loop->isRotatedForm())
            return loop->getHeader();

        // Il primo blocco Body del loop è il blocco successore all'Header
        BasicBlock *header = loop->getHeader();
        BranchInst *headerBranch = dyn_cast<BranchInst>(header->getTerminator());
        if(!headerBranch)
            return nullptr;
        
        // Siccome l'Header ha per forza due successori, l'Exit Block e il Body, se il primo successore è l'Exit Block allora il secondo è il Body e viceversa
        SmallVector<BasicBlock *, 10> loopExits;
        loop->getExitBlocks(loopExits);
        if(contains(loopExits, headerBranch->getSuccessor(0)))
            return headerBranch->getSuccessor(1);
        return headerBranch->getSuccessor(0);
    }

    // funzione helper che ritorna la branch instruction dell'ultimo blocco del body (escluso il LATCH). ovvero la branch
    // instruction che porta al blocco di latch
    Instruction *getTerminatorBodyInstruction(Loop* loop){
        // si ottiene il latch
        BasicBlock *latchBB = loop->getLoopLatch();
        if(!latchBB)
            return nullptr;
        
        // si prende il predecessore del latch 
        BasicBlock *boodyLastBB = latchBB->getUniquePredecessor();
        if(!boodyLastBB)
            return nullptr;
        
        // si prende l'ultima istruzione del predecessore del latch (e quindi la branch instruction verso il latch)
        Instruction *bodyTerminatorInstr = boodyLastBB->getTerminator();
        return bodyTerminatorInstr;
    }

    /*
        Funzione che si occupa di incorporare il Body del secondo loop nel Body del primo:

        Requisiti:
        - Istruzione terminatrice dell'ultimo blocco del Body (non Latch) del primo loop
        - Istruzione terminatrice dell'ultimo blocco del Body (non Latch) del secondo loop
        - Blocco Latch del primo loop
        
        Si imposta come successore del terminator dell'ultimo blocco Body del primo loop il primo blocco del Body del secondo loop,
        successivamente si imposta come successore del terminator dell'ultimo blocco Body del secondo loop il blocco Latch del primo loop.
    */    
    bool bodyConnect(Loop *first, Loop *second){
        // si ricava il branch del body che porta al latch del primo loop
        Instruction *firstBodyTerminatorInstruction = getTerminatorBodyInstruction(first);
        
        BranchInst *firstTerminatorBodyBranch = dyn_cast<BranchInst>(firstBodyTerminatorInstruction);
        if(!firstTerminatorBodyBranch)
            return false;
        
        // si ottiene il primo blocco del body del secondo loop
        BasicBlock *secondBody = getBodyStart(second);
        
        // modifichiamo la branch instruction alla fine del body del primo loop facendo in modo che punti al primo basic
        // block del body del secondo loop
        firstTerminatorBodyBranch->setSuccessor(0, secondBody);

        // si modifica la branch instruction alla fine del body (non latch) del secondo loop facendoin mod che punto al 
        // latch del primo loop
        Instruction *secondBodyTerminatorInstruction = getTerminatorBodyInstruction(second);
        BranchInst *secondTerminatorBodyBranch = dyn_cast<BranchInst>(secondBodyTerminatorInstruction);
        BasicBlock *firstLatch = first->getLoopLatch();


        // modifico la branch instruction terminatrice del secondo body facendo in modo che punti al latch del 
        // primo loop
        secondTerminatorBodyBranch->setSuccessor(0, firstLatch);
        
        return true;
    }

    
    bool loopRedirectToExit(Loop* second, BasicBlock* source, BasicBlock* internalPoint){
        if(!source || !internalPoint)
            return false;

        BranchInst* terminatorBranch = dyn_cast<BranchInst>(source->getTerminator());
        if(!terminatorBranch || !terminatorBranch->isConditional())
            return false;

        unsigned exitBranchIndex = terminatorBranch->getSuccessor(0) == internalPoint? 1:0;
        SmallVector<BasicBlock*, 10> secondExits;
        second->getExitBlocks(secondExits);

        if(secondExits.empty())
            return false;

        terminatorBranch->setSuccessor(exitBranchIndex, secondExits[0]);
        return true;
    }

    /**
     * Funzione che si occupa di collegare l'uscita dell'header del primo loop con l'exit block del secondo loop.
     * 
     * Requisiti:
     *  - header del primo loop
     *  - primo blocco del body del primo loop
     *  - exit block del secondo loop
     */
    bool headerExitConnect(Loop *first, Loop *second){
        return loopRedirectToExit(second, first->getHeader(), getBodyStart(first));
    }


    bool LatchExitConnect(Loop* first, Loop* second){
        return loopRedirectToExit(second, first->getLoopLatch(), first->getHeader());
    }

    /*
        Funzione che si occupa di collegare l'Header del secondo loop con il suo Latch:

        Requisiti:
        - Istruzione terminatrice dell'header 
        - Blocco Latch

        Inoltre si cambia l'istruzione di salto condizionato da Header al Latch in un istruzione di salto incondizionato.
        
        Nota: questa operazione viene svolta per fare in modo che LLVM capisca che il seconndo loop è vuoto e lo consideri come 
        Dead code
    */
    bool secondLoopHeaderToLatch(Loop *loop) {
        BasicBlock *header = loop->getHeader();
        BasicBlock *latch = loop->getLoopLatch();

        // si ottiene la branch instruction alla fine dell'header
        BranchInst *headerBranch =
            dyn_cast_or_null<BranchInst>(header->getTerminator());

        if (!headerBranch)
            return false;

        unsigned bodyStartIndex = 0;

        if(!loop->isRotatedForm())
            bodyStartIndex = headerBranch->getSuccessor(0) == getBodyStart(loop)? 0:1;
        headerBranch->setSuccessor(bodyStartIndex, latch);

        return true;
    }

    /**
     * Funzione che collega l'exit della prima guardia con l'exit della seconda guardia
     * Requisiti
     *  - preheader del primo loop
     *  - uscita della seconda guardia
     * 
     * Questa funzione viene eseguita solo per i loop guarded, se un loop non è guarded viene fatto ritornare direttamente true
     */
    bool bypassSecondGuard(Loop* first, Loop* second, LoopInfo &LI){
        BranchInst *firstGuardedBranch = first->getLoopGuardBranch();
        BranchInst *secondGuardedBranch = second->getLoopGuardBranch();

        if(!firstGuardedBranch || !secondGuardedBranch)
            return true;

        // si estrae l'exit block della seconda guardia
        BasicBlock *exitSecondGuardedBlock = getExitFromGuard(secondGuardedBranch, second);

        // si verifica quale successore della prima guardia non porta al pre-header del loop
        unsigned int firstExitSuccessor = firstGuardedBranch->getSuccessor(0) == first->getLoopPreheader()? 1:0;

        // si cambia il successore che porta all'uscita del primo loop facendo in modo che punti all'uscita del secondo loop
        firstGuardedBranch->setSuccessor(firstExitSuccessor, exitSecondGuardedBlock);
        return true;
    }

    /*
        Funzione helper che controlla se un loop ha la struttura di un loop di tipo For:

        Si può riconoscere un ciclo For dal suo blocco Latch, se questo contiene solamente due istruzioni, una di incremento del contatore e una di salto incondizionato all'Header, allora il loop è un For
        NOTA: Questo ci serve per escludere altri loop con strutture diverse che non sono stati trattati a lezione    
    */
    bool isForStructure(Loop* loop){
        return loop->getLoopLatch()->size() == 2;
    }

    bool fuseRotatedLoops(Loop* first, Loop* second, ScalarEvolution &SE, LoopInfo &LI){
        // posso usare direttamente le API di LLVM essendo loop rotated
        PHINode* firstIV = first->getInductionVariable(SE);
        PHINode* secondIV = second->getInductionVariable(SE);
        
        // l'ordine rispetto ai non rotated loops cambia, perchè quando cerco di modificare le guardie se ho già spostato i 
        // body il controllo su getGuardedBranch fallisce
        if(!bypassSecondGuard(first, second, LI)) return false;
        
        if(!LatchExitConnect(first, second)) return false;
        
        if(!bodyConnect(first, second)) return false;
        
        if(!secondLoopHeaderToLatch(second)) return false;
        
        inductionVariableFusion(firstIV, secondIV, SE);

        return true;
    }

    bool fuseNonRotatedLoops(Loop* first, Loop* second, ScalarEvolution &SE, LoopInfo &LI){
        // Devono essere entrambi due cicli For
        if( !isForStructure(first) || !isForStructure(second)){
            errs() << "One loop is in while-form. fuse refused\n";
            return false;
        }

        // Si ottengono le Induction Variable di entrambi i loop prima di effettuare la Fusion, questo perchè in questo modo evitiamo che i puntatori delle IV si invalidino a seguito proprio di alcune operazioni della Fusion
        PHINode* firstIV = getLoopInductionVariable(first, SE);
        PHINode* secondIV = getLoopInductionVariable(second, SE);
        
        // Si inserisce il Body del secondo loop nel Body del primo, se per un qualche motivo questa operazione fallisce si abortisce la fusione di questi due cicli
        if(!bodyConnect(first, second)) return false;
        
        // Si collega l'Exit Block del secondo loop all'Header del primo, se per un qualche motivo questa operazione fallisce si abortisce la fusione di questi due cicli
        if(!headerExitConnect(first, second)) return false;

        // Si collega l'Header del secondo loop con il suo Latch, se per un qualche motivo questa operazione fallisce si abortisce la fusione di questi due cicli
        if(!secondLoopHeaderToLatch(second)) return false;
        
        // Si collega l'exit della prima guardia con l'exit della seconda, se per un qualche motivo questa operazione fallisce si abortisce la fusione di questi due cicli
        if(!bypassSecondGuard(first, second, LI)) return false;

        // Si inserisce l'Induction Variable del secondo loop subito dopo l'induction variable del primo loop
        inductionVariableFusion(firstIV, secondIV, SE);

        return true;
    }

    /*
        Funzione che realizza la Loop Fusion di due loop:

        Requisiti:
        - Devono essere soddisfatte le condizioni di adiacenza
        - Deve essere soddisfatta la Control Flow Equivalence
        - Entrambi i loop devono avere lo stesso Trip Count
        - Non ci devono essere Backwards Dependency
        - Devono essere entrambi due cicli For
    */
    bool LoopFusion(LoopInfo &LI, Loop* first, Loop* second, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, DependenceInfo &DI, Function &F){
        errs() << "Analyzing " << first->getName() << " and " << second->getName() << "\n\n";
        
        // Devono essere soddisfatte le condizioni di adiacenza
        if(!verifyAdjacentLoops(first, second, LI)){
            errs() << first->getName() << " and " << second->getName() << " are NOT adjacent loops\n";
            return false;
        }    
        errs() << first->getName() << " and " << second->getName() << " are adjacent loops\n";
        
        // Deve essere soddisfatta la Control Flow Equivalence
        if(!verifyControlFlowEquivalence(first, second, DT, PDT, SE, LI)){
            errs() << first->getName() << " and " << second->getName() << " are NOT control flow equivalent \n";
            return false;
        }   
        errs() << first->getName() << " and " << second->getName() << " are control flow equivalent \n";  

        // Entrambi i loop devono avere lo stesso Trip Count
        if(!verifySameTripCount(first, second, SE)){
            errs() << first->getName() << " and " << second->getName() << " DON'T have the same trip count \n";
            return false;
        }   
        errs() << first->getName() << " and " << second->getName() << " have the same trip count \n";
        
        // Non ci devono essere Backwards Dependency
        if(!verifyDependencies(first, second, DI, SE)){
            errs() << first->getName() << " and " << second->getName() << " have at least one backward dependence\n";
            return false;
        }
        errs() << first->getName() << " and " << second->getName() << " can be fused \n";

        bool correctFusion = false;
        if(first->isRotatedForm() && second->isRotatedForm())
            correctFusion = fuseRotatedLoops(first, second, SE, LI);
        else
            correctFusion = fuseNonRotatedLoops(first, second, SE, LI);

        if(!correctFusion){
            errs() << "Fusion error !\n";
            return false;
        }

        // Si ripulisce il codice del programma dal secondo loop ormai vuoto
        EliminateUnreachableBlocks(F);
        errs() << "Loop Fused correctly ! \n";
        
        return true;
    }
//-----------------------------------------------------------------------------
// Loop Fusion  pass

// --> è un passo di trasformazione utilizzato per la loop fusion
//-----------------------------------------------------------------------------
    struct LoopFusionOptPass : PassInfoMixin<LoopFusionOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM){
            bool fused = true;
            bool result = false;
            
            // Itero l'intera fase di fusione fino a che non ci sono più fusioni da eseguire
            while (fused) {
                /**
                 * Ogni volta che eseguo una funzione devo poter ricalcolare completamente gli oggetti forniti dall'analysis manager
                 * Questo perchè, cambiando il CFG della funzione i vari puntatori ai loop vengono invalidati, lo schema delle dominanzecambia
                 * e le relazione tra i vari loop cambiano.
                 * è quindi necessario svolgere una ri-analisi per ottenere gli oggetti aggiornati e che descrivano il CFG in maniera corretta anche dopo la fusione
                 */
                LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
                DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
                PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
                ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
                DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

                bool fused = false;
                
                // si crea una mappa che ha come chiave il puntatore al loop padre (eventualmente nullptr per i top-level loops) e come valore
                // una lista di tutti quanti i loop figli di tale padre (tra loro fratelli) inseriti nell'array in ordine di programam
                /*
                    Nota: ogni volta che si esegua una fusione è necessario dover rivalutare anche tutta la mappa.
                    Questo perchè invalidando LoopInfo si invalidano consequenzialmente anche tutti i puntatori dei loop della mappa
                    (loop info viene ri-valutato grazie ad una funzione "realease memory" che invalida tali puntatori).
                */
                DenseMap<Loop*, SmallVector<Loop*, 10>> LoopSiblingsMap;

                for (Loop *LL : LI.getLoopsInPreorder()) {
                    LoopSiblingsMap[LL->getParentLoop()].push_back(LL);
                }

                // grazie al fatto che i loop fratelli sono in ordine di programma, posso prenderli a coppie di 2 e verificarne ed effettuarne la fusione
                for (auto &[FatherLoop, LoopSiblings] : LoopSiblingsMap) {
                    if (fused)
                        break;

                    // scorro i loop nell'array a coppie per verificare la fusione
                    for (unsigned i = 0; i + 1 < LoopSiblings.size(); ++i) {
                        Loop *First = LoopSiblings[i];
                        Loop *Second = LoopSiblings[i + 1];

                        errs() << "\n=========================================\n";
                        result = LoopFusion(LI, First, Second, DT, PDT, SE, DI, F);
                        errs() << "=========================================\n\n";

                        if (result) {
                            fused = true;
                            break;
                        }
                    }
                }

                if (!fused)
                    break;

                
                // se i loop si fondono correttamente il flag fused viene settato a true e si invalidano le analisi per il ricalcolo
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
