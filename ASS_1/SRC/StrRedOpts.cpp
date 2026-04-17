#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/DivisionByConstantInfo.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace llvm;

//-----------------------------------------------------------------------------
// Strenght Reduction opt Pass implementation
//-----------------------------------------------------------------------------
/*
   è un passo che permette di poter implementare l'ottimizzazione di strength reduction sia su moltiplicazioni positive di qualsiasi tipo ( non solo potenze di 2)
   e che permette di poter ottimizzare tutte quante le divisioni unsigned utilizzando la tecnica dei Magic number.
   In entrambi i casi deve esistere un operando costante ed un operando non costante.
   le casistiche coperte sono le seguenti:


    MOLTIPLICAZIONE (vale la proprietà commutativa):
    x * 16 --> x << 4
    x * 7 --> (x << 2) + ( x << 1 ) + x
    x * 1 --> x
    x * 0 --> 0

    DIVISIONE (il valore costante deve essere al denominatore):
    x / 1 --> x
    x / 16 --> x >> 4
    x / 15 --> ( x * M ) >> k

*/
namespace
{

    /*
        found_pow: è una funzione che calcola il valore del logaritmo in base 2 di un numero dato in input e lo inserisce dentro log_value.
       è utile per poter calcolare l'esponente di un eventuale potenza di due, per poterlo indicare come secondo operando di una shift.
       Inoltre ritorna:
       1 --> se il numero n = 1
       0 --> se il numero n = 0
       -1 --> se il numero n è negativo
       2 --> se il numero è una potenza di 2
       3 --> se il numero non è una potenza di due

    */
    int found_pow(int n, int &log_value)
    {
        if (n == 1)
            return 1;
        if (n == 0)
            return 0;
        if (n < 0)
            return -1;

        // calcolo il logaritmo di n con la relativa funzione
        double val = std::log2(n);
        if (std::fmod(val, 1) == 0)
        {
            log_value = val;
            return 2;
        }

        return 3;
    }

    /*
        Magic division è una divisione che viene utilizzata per divisioni non con potenze di 2.
       L'idea della divisione è poter utilizzare un valore costante M tale per cui trasformare la divisone in:
       quoziente =  (x * M) / 2^k


       in realtà per ottimizzare i cicli si utilizza:
       quoziente = ( x * M ) >> k


       dove M è un valore costante che viene calcolato sulla base del divisore e del valore del dividendo
       mentre k corrisponde al numero di bit di grandezza del divisore (e dividendo).


       è importante sottolineare che il valore M viene calcolato attraverso l'utilizzo di un apposito metodo get della classe
       UnsignedDivisionByConstantInfo che permette di poter gestire inoltre particolari scenari di correzione:


       - PRE-SHIFT: scenario che si verifica per semplificare il calcolo del Magic number se il divisore è pari o,
       generalmente, si può scrivere come prodotto di una potenza di due (ex: 12 = 3 * 2^2).
       Il pre shift permette di poter direttamente inserire una shift relativa all'esponente del termine potenza di 2.
       In questo modo si semplifica l'intera divisione futura, prendendo in considerazione solamente la parte restante del divisore.
       Il numero di bit del pre-shift è già calcolato dall'oggetto Magic, potendo quindi direttamente creare la shift utilizzando tale
       numero di bit


       - ISADD: è uno scenario che si verifica per numeri dove il calcolo con il semplice Magic Number non basta per poter ottenere il
       risultato corretto.
       In questo caso, si calcola la differenza fra il valore da dividere e il quoziente ottenuto tramite il processo di magic number:
       (x*M)>>k.
       la differenza viene divisa per due e sommata al quoziente finale.
       Queste operazioni servono principalmente per poter fare in modo che il quoziente finale così ottenuto sia preciso.
       Questo scenario si verifica quando il Magic Number ideale ha un numero di bit maggiore rispetto a quello messo a disposizione.


       - POST-SHIFT: è lo scenario che si verifica nel momento in cui lo shift a destra di k (numero di bit - 32 nel nostro caso) non è sufficiente.
       è quindi necessario dover utilizzare un ulteriore shift right, di cui il numero di bit è espressamente definito dentro l'oggetto Magic.

    */
    void magicDiv(BinaryOperator *BinOp, ConstantInt *constantValue, Value *registerOperand)
    {
        // prendo i riferimenti necessari per calcolare la divisione con magic number

        Type *registerType = registerOperand->getType();        // tipo del registro
        unsigned BitWidth = registerType->getIntegerBitWidth(); // numero di bit del registro
        LLVMContext &Ctx = registerType->getContext();          // contesto: oggetto in cui sono salvati i tipi LLVM

        // estendo il valore del divisore a 64 bit per aumentare la precisione (estendo con zeri in testa: unsigned)
        auto dValue = constantValue->getZExtValue();
        // Creo l'apposito oggetto Divisor, utilizzato per calcolare il Magic number
        APInt Divisor(BitWidth, dValue);

        // Cacolo il Magic number con l'apposito oggetto
        UnsignedDivisionByConstantInfo Magici = UnsignedDivisionByConstantInfo::get(Divisor);

        Value *Val = registerOperand;

        // Verifico la necessità di un pre shift
        if (Magici.PreShift > 0)
        {
            // nel caso di preshift lo inserisco come shift di registerOperand per un valore PreC che corrisponde a quanto dover shiftare
            Value *PreC = ConstantInt::get(registerType, Magici.PreShift);
            Instruction *PreShiftInst = BinaryOperator::CreateLShr(Val, PreC);
            PreShiftInst->insertBefore(BinOp);
            Val = PreShiftInst;
        }

        // Inserisco delle operazioni di estensione afino a 64 bit per poter evitare problemi di overflow
        // dovuti al Magic Number molto grande a seguito della moltiplicazione
        Type *WideTy = IntegerType::get(Ctx, BitWidth * 2);
        Value *MagicC = ConstantInt::get(registerType, Magici.Magic);

        Instruction *XWide = new ZExtInst(Val, WideTy);
        XWide->insertBefore(BinOp);

        Instruction *MagicWide = new ZExtInst(MagicC, WideTy);
        MagicWide->insertBefore(BinOp);

        // Moltiplicazione fra il magic number e il register operand (entrambi estesi) --> X * M
        Instruction *MulWide = BinaryOperator::CreateMul(XWide, MagicWide);
        MulWide->insertBefore(BinOp);

        // Shift a sinistra di k bit (x * M ) >> k
        Value *ShiftByBitWidthC = ConstantInt::get(WideTy, BitWidth);
        Instruction *HighPart = BinaryOperator::CreateLShr(MulWide, ShiftByBitWidthC);
        HighPart->insertBefore(BinOp);

        // riporto il valore calcolato del quoziente a 32 bit, in linea con il resto del programma
        Instruction *Res = new TruncInst(HighPart, registerType);
        Res->insertBefore(BinOp);

        // Verifico la casisitica di isAdd, in cui il Magic num utilizzato è più piccolo rispetto a quello ottimale a causa della sua dimensione in bit
        // applico una serie di somme e sottrazioni per compensare all'errore
        if (Magici.IsAdd)
        {
            // Differenza fra il registerOperand e il quoziente calcolato
            Instruction *Sub = BinaryOperator::CreateSub(Val, Res);
            Sub->insertBefore(BinOp);

            // La differenza di divide per 2
            Value *OneC = ConstantInt::get(registerType, 1);
            Instruction *Corr = BinaryOperator::CreateLShr(Sub, OneC);
            Corr->insertBefore(BinOp);

            // il risultato della divisione per due lo si somma al quoziente per compensare all'errore
            Instruction *AddRes = BinaryOperator::CreateAdd(Res, Corr);
            AddRes->insertBefore(BinOp);
            Res = AddRes;
        }

        // Verifico il post shift nel caso in cui uno shift di k non sia sufficiente a causa della dimensione del magic number
        if (Magici.PostShift > 0)
        {
            // ottengo il numero di bit del post shift direttamente da Magic
            Value *PostC = ConstantInt::get(registerType, Magici.PostShift);

            // creo lo shift a destra
            Instruction *PostShiftInst = BinaryOperator::CreateLShr(Res, PostC);
            PostShiftInst->insertBefore(BinOp);
            Res = PostShiftInst;
        }

        // Rimpiazzo gli uses della binOp originale, con l'ultima istruzione eseguita (calcolo del quoziente finale)
        BinOp->replaceAllUsesWith(Res);
    }

    /*
    sommaShift è la funzione generica che permette di poter scrivere una moltiplicazione tra register operand e constantValue
    come una somma di prodotti per potenze di due.
    nel caso in cui infatti il valore costante della moltiplicazione non sia una potenza di 2, operazione sostituibile direttamente con una shift,
    posso scomporre il valore costante come somma di potenze di due e applicare la proprietà distributiva della moltiplicazione.
    Ex:

     x * 5 = x * ( 2^2 + 2^0 ) = x * (4 + 1) = (x * 4) + (x * 1) = ( x << 2 ) + x

     in questo modo il prodotto di può scrivere come somma di operazioni shift right
     */
    void sommaShift(BinaryOperator *BinOp, ConstantInt *constantValue, Value *registerOperand)
    {
        // ricavo il valore unsigned del numero costante e inizializzo gli oggetto che verranno successivamente utilizzati.
        auto number = constantValue->getZExtValue();
        Instruction *newShift = nullptr;
        Instruction *newAdd = nullptr;
        Value *valI = nullptr;
        Instruction *incSum = nullptr;

        // per ogni bit a partire da quello in posizione 1 (la posizione 0 verrà valutata a parte) controllo se è 1 o 0.
        for (unsigned i = 1; i < constantValue->getType()->getIntegerBitWidth(); ++i)
        {
            if ((number >> i) & 1) // il controllo è fare l'and logico fra l'iesima cifra del numero e il valore 1
            {
                // se il bit è 1 allora creo la shift a destra di i posizioni
                valI = ConstantInt::get(registerOperand->getType(), i);
                newShift = BinaryOperator::CreateShl(registerOperand, valI);
                newShift->insertBefore(BinOp);

                // e la sommo incrementalmente alla somma incrementale delle altre shift già precedentemente create e sommate
                if (incSum == nullptr) // nel caso in cui sia la prima shift, allora la considero già come il primo operando della somma, ma aspetto una seconda shift per crere l'add
                    incSum = newShift;
                else
                { // se invece esiste già una shift creo l'operazione di add incrementale
                    incSum = BinaryOperator::CreateAdd(newShift, incSum);
                    incSum->insertBefore(BinOp);
                }
            }
        }

        // gestisco il primo bit separatamente, in cui aggiungo una add direttamente utilizzante registerOperand come operando (evito la shift per 0)
        if (number & 1)
        {
            incSum = BinaryOperator::CreateAdd(registerOperand, incSum);
            incSum->insertBefore(BinOp);
        }

        // rimpiazzio gli usi del prodotto con l'ultima istruzione creata.
        BinOp->replaceAllUsesWith(incSum);
    }

    // New PM implementation
    struct StrRedOptsPass : PassInfoMixin<StrRedOptsPass>
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

                    // Verifico se è una operazione binaria con un dyn_cast a BinaryOperator
                    if (auto *BinOp = dyn_cast<BinaryOperator>(&I))
                    {
                        /* Verifico se l'istruzione è una moltiplicazione o una divisione verificando se il suo
                        OP code (identificatore univoco del tipo di istruzione) è uguale a quello di una Mul o di una Div */

                        if (BinOp->getOpcode() == Instruction::Mul || BinOp->getOpcode() == Instruction::SDiv)
                        {

                            Value *registerOperand;
                            ConstantInt *constantValue;

                            // estraggo dalla operazione binaria l'operando costante e quello non costante
                            // per farlo utilizzo delle dynamic cast
                            if (auto *CI = dyn_cast<ConstantInt>(BinOp->getOperand(0)))
                            {
                                constantValue = CI;

                                if (BinOp->getOpcode() == Instruction::SDiv) // in caso di primo operando costante, la divisione non viene valutata
                                    continue;
                                if (dyn_cast<ConstantInt>(BinOp->getOperand(1)))
                                    continue;
                                registerOperand = BinOp->getOperand(1);
                            }
                            else if (auto *CI = dyn_cast<ConstantInt>(BinOp->getOperand(1)))
                            {
                                constantValue = CI;
                                registerOperand = BinOp->getOperand(0);
                            }
                            else // non valuto le istruzioni con entrambi gli operandi non costanti
                                continue;

                            // richiamo la funzione found_pow per poter definire la situazione attuale e un eventuale valore di shift
                            int log_value = 0;
                            int result = found_pow(constantValue->getSExtValue(), log_value);
                            Value *val = ConstantInt::get(registerOperand->getType(), log_value);

                            // gestisco le casistiche
                            if (result == -1) // salto alla prossima istruzione se il numero è negativo
                                continue;
                            if (result == 0) // sostituisco tutti gli uses di BinOp con 0 se il numero è 0
                            {
                                if (BinOp->getOpcode() == Instruction::SDiv) // in caso di divisione salto direttamente alla prossima istruzione (non si può dividere per 0)
                                    continue;
                                BinOp->replaceAllUsesWith(ConstantInt::get(registerOperand->getType(), 0));
                                toDelete.push_back(BinOp);
                            }
                            else if (result == 1) // se il numero costante è 1 sostituisco direttamente con registerOperand tutti gli uses della operazione binaria
                            {
                                BinOp->replaceAllUsesWith(registerOperand);
                                toDelete.push_back(BinOp);
                            }
                            else if (result == 2) // se il numero è una potenza di due allora creo una shift left (moltiplicazione ) o right( divisione), utilizzando log_value come valore costante
                            {
                                Instruction *NewInst = BinOp->getOpcode() == Instruction::SDiv ? BinaryOperator::CreateAShr(registerOperand, val) : BinaryOperator::CreateShl(registerOperand, val);
                                NewInst->insertBefore(BinOp);
                                BinOp->replaceAllUsesWith(NewInst);
                                toDelete.push_back(BinOp);
                            }
                            else // se invece non è una potenza di due gestisco separataemnte i casi in base al tipo di operazione
                            {
                                if (BinOp->getOpcode() == Instruction::SDiv) // per la divisione creo il calcolo con il magic number
                                    magicDiv(BinOp, constantValue, registerOperand);
                                else // per la moltiplicazione calcolo la somma di shift
                                    sommaShift(BinOp, constantValue, registerOperand);
                                toDelete.push_back(BinOp);
                            }
                        }
                    }
                }

                // elimino tutte le istruzioni BinOp marcate
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
llvm::PassPluginLibraryInfo getStrRedOptsPassPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "StrRedOptsPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "str-red-opt")
                        {
                            FPM.addPass(StrRedOptsPass());
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
    return getStrRedOptsPassPluginInfo();
}
