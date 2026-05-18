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


    /**
     * Funzione che, dato un operando passato come argomento, verifica se è loop invariant rispetto al loop passato come argomento
     * Un operando è loop invariant se:
     * 
     * - è un argomento o è costante
     * - è definito fuori dal loop
     * - è definito in una istruzione dentro al loop a sua volta loop invariant 
     */
    bool isLoopInvariantOperand(Value *operand, Loop *const LL, SmallVector<Instruction *, 10> &loopInvariantInstructions){
        // se l'operando è costante o è un argomento --> loop invariant
        if(dyn_cast<Constant>(operand) || dyn_cast<Argument>(operand))
            return true;

        // se è una istruzione verifico se è definito fuori dal loop o è loop invariant
        if (Instruction *I = dyn_cast<Instruction>(operand)) {
            
            // se la definizione è definita furoi dal loop --> l'operando è loop invariant 
            if (!LL->contains(I->getParent()))
                return true;
            
            /** 
            * se la definizione è dentro al loop verifico ricorsivamente se è loop invariant.
            * queste chiamate ricorsive permettono di verificare la proprietà di loop invariant indipendentemente dall'ordine di 
            * analisi delle istruzioni dentro al basic block
            */
            return isLoopInvariantInstruction(I, LL, loopInvariantInstructions);
        }

        return false;
    }

    
    /**
     * Funzione che, data una specifica istruzione, verifica se è loop invariant rispetto al loop passato come argomento.
     * Per essere loop invariant instruction devono essere rispettate le seguenti proprietà sugli operandi:
     * 
     * - Devono essere definiti fuori dal loop
     * oppure
     * - Devono essere definit da una istruzione dentro il loop che è a sua volta loop invariant
     * 
     * Per verificare se una di queste codnizioni è verificata si richiama ricorsivamente la funzione isLoopInvariantOperand
     * - Se almeno un operando non è loop invariant --> l'intera istruzione non è loop invariant
     * - Se tutti gli operandi sono loop invariant --> viene fatto ritornare true
     */
    bool isLoopInvariantInstruction(Instruction *I, Loop *LL,  SmallVector<Instruction *, 10> &loopInvariantInstructions){
        // verifico se l'istruzione passata come argomento è già marcata come loop invariant
        for (Instruction *InvInst : loopInvariantInstructions)
                if (InvInst == I)
                    return true;

        // se è un phi node ritorno false --> non considero le multiple reaching definitions
        if(dyn_cast<PHINode>(I))
                return false;

        // per ogni operando richiamo la funzione isLoopInvariantOperand e ritorno true solo se tutti gli operandi sono loop invariant
        for(Value* op : I->operands())
            if(!isLoopInvariantOperand(op, LL, loopInvariantInstructions)){
                return false;
            }
        return true;
    }

    /**
     * Funzione che verifica quali istruzioni sono loop invariant, 
     * inserendo quelle trovate all'interno dell'apposito array loopInvariantInstructions.
     * 
     * Per ciascun blocco del loop (saltando quelli già precedentemente analizzati dai sub-loops --> toSkip)
     * verifico con l'apposita funzione se l'istruzione è loop invariant.
     *  
     * - Se il controllo è positivo la aggiungo all'array 
     * - altrimenti non la aggiungo e passo alla istruzione successiva 
     */
    void findLoopInvariant(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip,  SmallVector<Instruction *, 10> &loopInvariantInstructions, DominatorTree &DT){
       
        // scorro su tutti i basic block del loop
        for(BasicBlock *const BB:LL->blocks()){
            // salto quelli già analizzati dai subloops
            if(toSkip.count(BB) != 0) continue;

            // scorro tutte le istruzioni del loop
            for(auto i = BB->begin(); i != BB->end(); ++i){
                Instruction &I = *i;

                // verifico se la istruzione è loop invariant con la funzione isLoopInvariantInstruction e se il controllo è positivo la aggiungo all'array
                if(isLoopInvariantInstruction(&I, LL, loopInvariantInstructions)){
                    loopInvariantInstructions.push_back(&I);
                    errs() << I << " Is invariant\n";
                }
            }
        }
    }


    /**
     * Funzione che, data una istruzione Loop invariant verifica se domina tutte le uscite del loop (passate come argomento)
     * 
     * Per farlo si scorrono tutti i blocchi successivi al loop (successorsBlocks) e si verifica la proprietà di dominanza
     * fra l'istruzione data fornita come argomento e ciascuno dei blocchi successivi.
     * - Se il controllo è positivo su tutti i blocchi --> domina tutte le uscite --> ritorna true
     * - Se il controllo fallisce almeno una volta --> non domina tutte le uscite --> ritorna false
     */
    bool verifyDominance(Instruction* LIInstr, SmallVector<BasicBlock *, 10> successorsBlocks, DominatorTree &DT){
        BasicBlock *LIIBasicBlock = LIInstr->getParent();

        // Per ciascuna uscita del loop
        for(BasicBlock *exitBlock: successorsBlocks){
            // verifico la proprietà di dominanza fra il blocco della istruzione passata come argomento e la relativa uscita considerata
            if(!DT.dominates(LIIBasicBlock, exitBlock ) ){
                //errs() << LIIBasicBlock->getName() << " non domina " << exitBlock->getName() << "\n";
                return false;
            }
        }
        return true;
    }


    /**
     * Prende in input una istruzione e verifica se viene utilizzata fuori da un loop o in un phi node.
     * 
     * Se l'istruzione viene usata in un phi node nell'header (e tale phi usato fuori dal loop) 
     * significa che l'itruzione in se ha un uso (indiretto tramite phi) fuori dal loop e non è dead code
     * 
     * Se l'istruzione viene usata in un phi node non nell'header:
     * si verifica se è dentro o fuori il loop
     * - se è dentro il loop viene ignorato (phi normale di utilizzo dentro il loop)
     * - se è fuori dal loop (dominato dall'uscita) significa che un generico uso al di fuori del loop
     *   e la realtiva istruzione non è dead code.
     * 
     * Se l'istruzione viene usata in un'altra istruzione fuori dal loop significa che non è dead code
     */
    bool verifyDeadCode(Instruction* LIInstr, DominatorTree &DT, Loop *LL, SmallVector<BasicBlock *, 10> successorsBlocks){
        for(auto const &user: LIInstr->users()){
            auto *userInstruction = dyn_cast<Instruction>(user);
            
            if(!userInstruction)
                return false;

            if(auto *phiNodeInstruction = dyn_cast<PHINode>(userInstruction)){

                /**
                Si può semplificare con:

                if(LL->getHeader() == phiNodeInstruction->getParent()){
                    errs() << "%"<<(*LIInstr).getName()<< " Ha il phi node "<< "%"<<(*phiNodeInstruction).getName() <<" nell header, ovvero possibili multiple reaching def\n";
                    return false
                }

                */

                // verifico se l'user è il phi node nell'heder che introduce LLVM per gestire gli utilizzi esterni al loop
                if(LL->getHeader() == phiNodeInstruction->getParent()){
                    errs() << "%"<<(*LIInstr).getName()<< " Ha il phi node "<< "%"<<(*phiNodeInstruction).getName() <<" nell header, ovvero possibili multiple reaching def\n";
                    if(!verifyDeadCode(phiNodeInstruction, DT, LL, successorsBlocks))
                        return false;
                    continue;
                }
                    
                // verifico se lo user, anche se phi, è dentro o fuori al loop
                for(BasicBlock *exitBlock: successorsBlocks){
                    if(DT.dominates(exitBlock, phiNodeInstruction->getParent())){
                        errs() << "%"<<(*LIInstr).getName() << " è usata fuori dal loop nel phi:"<< *phiNodeInstruction <<"\n";
                        return false;                    
                    }
                }
            }else{

                // verifico se lo user normale (non phi) è dentro o fuori al loop
                for(BasicBlock *exitBlock: successorsBlocks){
                    if(DT.dominates(exitBlock, userInstruction->getParent())){
                        errs() << "%"<<(*LIInstr).getName() << " è usata fuori dal loop\n";
                        return false;                    
                    }
                }
            }     
        }
        return true; 
    }


    /**
     * Funzione che verifica quali istruzioni loop invariant passate come argomento sono movable e quali no.
     * Data una istruzione loop invariant questa è movable se:
     * 
     * - Domina tutte le uscite del loop.
     * - Non ha usi al di fuori del loop.
     * - La variabile non viene ridefinita dentro il loop (implicitamente verificato da SSA)
     * - la varaibile domina tutti i suoi usi dentro al loop (implicitamente verificatwo da SSA)
     * 
     * Tutte le istruzioni movable sono inserite dentro l'apposito set: codeMotionInstructions
     */
    void verifyCodeMotion(Loop *const LL,  SmallVector<Instruction *, 10> &loopInvariantInstructions, DominatorTree &DT, SmallPtrSet<Instruction *, 10> &codeMotionInstructions){
        // ottengo la referenza ai blocchi successivi (exit blocks) del loop (uscite edl loop)
        SmallVector<BasicBlock *, 10> successorsBlocks;
        LL->getExitBlocks(successorsBlocks);

        // per ogni instruction loop invariant verifico le proprietà e in tal caso la aggiungo al movable instruction set
        for(Instruction *LIInstr: loopInvariantInstructions ){

            //verifico che domini tutte le uscite con la funzione verifyDominance
            if(verifyDominance(LIInstr, successorsBlocks, DT)){
                codeMotionInstructions.insert(LIInstr);
                continue;
            }
            
            // verifico che non abbia usi al di fuori del loop on verifyDeadCode
            if(verifyDeadCode(LIInstr, DT, LL, successorsBlocks))
                codeMotionInstructions.insert(LIInstr);
        }
    }

    /**
     * Funzione che verifica se una specifica funzione ha dipendenze rispetto ad una qualsiasi altra istruzione presente nel set
     * passato come argomento.
     * 
     * Dipendenza: una istruzione A è dipendente da una istruzione B nel caso in cui l'istruzione B venga usata come operando 
     * dall'istruzione A. (entrambe devono essere dentro il set delle movable instruction)
     * 
     * Nel caso dipendenza ritorna true, altrimenti false
     */
    bool hasDependencies(Instruction* LIInstr, SmallPtrSet<Instruction *, 10> codeMotionInstructions){
        for(Value* operand: LIInstr->operands()){
            if( auto I = dyn_cast<Instruction>(operand))
            // se un qualsiasi operando della istruzione è contenuto dentro codeMotionInstruction --> c'è una dipendenza 
                if(codeMotionInstructions.contains(I)){
                    errs() << "Ha una dipendenza \n ";
                    return true;
                }
        }
        return false;
    }

    /**
     * Funzione che, dato un insieme di istruzioni movable le muove dalla loro posizione all blocco pre-header del relativo loop.
     * Riceve in input il loop e l'insieme delle istruzioni movable.
     * 
     * Per ciascuna istruzione verifica se questa ha delle dipendenze (deve essere successiva) ad un'altra istruzione movable
     * del set.
     * - Se l'istruzione non ha dipendenze la sposta nel pre-heder
     * - Se l'istruzione ha dipendenze la ignora temporaneamente e prosegue con l'analisi delle successive.
     *  (Aspetta che la istruzione che crea la dipendenza venga mossa)
     * 
     * L'intera funzione ripete fino a che tutte le istruzioni sono state spostate nel pre-heder, garantendo così il corretto
     * ordine delle dipendenze
     */
    void moveInstructions(SmallPtrSet<Instruction *, 10> &codeMotionInstructions, Loop *LL){
        // referenza al blocco pre-header del loop
        Instruction *preHeaderTerminator = (LL->getLoopPreheader())->getTerminator();

        // itero fino a che non ho spostato tutte le istruzioni
        while(!codeMotionInstructions.empty()){
            for(Instruction* moveableInstruction: codeMotionInstructions){

                // se una istruzione ha dipendenze rispetto ad un'altra istruzione movable la ignoro temporaneamente 
                if(!hasDependencies(moveableInstruction,  codeMotionInstructions)){
                    // se non ha dipendenze la sposto nel blocco pre-header e la rimuovo dal set
                    moveableInstruction->removeFromParent();
                    moveableInstruction->insertBefore(preHeaderTerminator);
                    codeMotionInstructions.erase(moveableInstruction);
                }
            }
        } 
    }


    /**
     * Funzione che analizza l'intero loop e i suoi subloops.
     * Per ciascuno dei subloops si richiama la funzione ricorsivamenete.
     * 
     * Per ciascun loop si richiama :
     * - findLoopInvariant che trova le istruzioni loop invariant e le inserisce nell'omonimo array
     * - verifyCodeMotion che verifica quali fra le istruzioni loop invariant precedentemente trovare sono movable
     * - move instruction che muove le istruzioni definite come movable dalla funzione precedente
     * 
     * Una volta analizzato ciascun loop inserisce i propri blocchi dentro toSkip per fare in modo che i loop
     * padre non ri-analizzino il subloops
     */
    void analyzeLoop(Loop *const LL, SmallPtrSet<BasicBlock *, 10> &toSkip, DominatorTree &DT){
        SmallVector<Instruction *, 10> loopInvariantInstructions; // vettore che contiene le istruzioni loop invariant
        SmallPtrSet<Instruction *, 10> codeMotionInstructions; // insieme che contiene tutte le istruzioni candidate per il move


        // Analisi dei loop interni in maniera ricorsiva
        for(Loop *const SubLL:LL->getSubLoops())
            analyzeLoop(SubLL, toSkip, DT);

        errs() << "\n---------------------------\n";
        errs() <<"\nLoop invariant instructions: \n";

        // trovo le funzioni loop invariant con la relativa funzione
        findLoopInvariant(LL, toSkip, loopInvariantInstructions, DT);
        errs() << "\n\nVerifica delle movable instructions:\n";

        // verifico quali istruzioni loop invariant sono movable
        verifyCodeMotion(LL, loopInvariantInstructions, DT, codeMotionInstructions);

        errs() << "\n\nMovable (code motion) instructions: \n";
        for(Instruction *i: codeMotionInstructions)
            errs() << *i << " is Movable\n";

        // muovo le istruzioni con la relativa funzione
        moveInstructions(codeMotionInstructions, LL);

        errs() << "\n---------------------------\n";
        

        // Inserisco i blocchi analizzati dentro toSkip per saltarli nel loop padre --> evito ridondanza 
        for(BasicBlock *const BB:LL->blocks())
            toSkip.insert(BB);
    }

//-----------------------------------------------------------------------------
// Loop Invariant Code Motion (LICM) pass

// --> è un passo di trasformazione utilizzato per la loop invariant code motion
//-----------------------------------------------------------------------------
    struct LoopInvariantOptPass : PassInfoMixin<LoopInvariantOptPass>
    {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM)
        {   
            // ottengo le referenze del LoopInfo e del dominator tree per le analisi dei loop e delel proprietà di dominanza
            LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
            DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

            // analizzo singolarmente ogni loop più esterno con la relativa funzione analyzeLoop
            // Ogni subloop viene analizzato ricorsivamente dalla funzione analyzeLoop
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


// registrazione del passo nel Pass Builder
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
