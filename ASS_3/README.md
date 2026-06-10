# Implementazione della Loop Invariant Code Motion del Terzo Assignment

## Come eseguire il passo

Per generare i file che sfruttano l'ottimizzazione di questo passo bisogna eseguire il seguente comando:

```bash
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_icm.c -o loop_icm.O0.ll && opt -passes=mem2reg -S loop_icm.O0.ll -o loop_icm.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopInvariantOptPass.so -passes=loop-invariant-opt loop_icm.O0.ll -o loop_icm.OPT.ll
```

MAC

```bash
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_icm.c -o loop_icm.O0.ll && opt -passes=mem2reg -S loop_icm.O0.ll -o loop_icm.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopInvariantOptPass.dylib -passes=loop-invariant-opt loop_icm.O0.ll -o loop_icm.OPT.ll
```

## 1. Implementazione della Loop Invariant

Per la definizione di Loop Invariant si definisce invariante un'istruzione, interna a un ciclo, il cui risultato rimane costante durante tutte le iterazioni.

​Per identificarla si utilizza l'analisi delle reaching definitions, valutando ogni istruzione nella forma A = B + C. Un'istruzione viene marcata come loop-invariant se, per ciascuno dei suoi operandi (B e C), si verifica almeno una delle seguenti tre condizioni:

- Tutte le definizioni dell'operando che raggiungono l'istruzione si trovano fuori dal ciclo.
- L'operando è una costante (o un argomento nel caso di richiami a funzioni).
- Esiste una sola definizione raggiungente interna al ciclo, ed è già stata marcata come invariante.

​Per fare in modo che queste regole vengano rispettate abbiamo implementato i seguenti metodi:

- `isLoopInvariantOperand`: verifica se un operando è Loop Invariant
- `isLoopInvariantInstruction`: verifica se un'istruzione è Loop Invariant
- `findLoopInvariant`: trova tutte le istruzioni Loop Invariant

---

## 2. Implementazione della Code Motion

Il fatto che un'istruzione sia loop-invariant è una condizione necessaria ma non sufficiente per poterla spostare.
Un'istruzione invariante, infatti, non è automaticamente movable (spostabile): il compilatore deve prima applicare l'analisi di **Code Motion** per verificare che lo spostamento sia sicuro (safe) e non alteri il comportamento o la semantica del programma.
​Per poter muovere un'istruzione loop-invariant (es. A = B + C) nel preheader (il blocco che precede il ciclo), devono essere soddisfatte tre condizioni di sicurezza:

​1. **Dominanza del blocco di uscita:** l'istruzione deve dominare tutte le uscite del ciclo.
Questa condizione può essere semplificata nella sola verifica degli usi dell'istruzione da spostare.
Se l'istruzione marcata come invariante non viene mai usata fuori dal ciclo, allora il suo spostamento può avvenire anche se non domina tutte le uscite.

​2. **Unicità della definizione:** L'istruzione deve essere l'unica definizione all'interno del ciclo per quella specifica variabile.
Nel caso di molteplici ridefinizioni, lo spostamento dell'istruzione marcata come invariante cambierebbe la semantica del programma.

​3. **Dominanza dei punti d'uso:** Ogni istruzione marcata come invariante deve dominare tutti i suoi usi.

I punti 2 e 3 sono implicitamente verificati quando si lavora nella forma SSA.

Per poter verificare anche queste condizioni è stato necessario implementare i seguenti metodi:

- `verifyDominance`: verifica che il blocco contenente l'istruzione loop-invariant domini tutti i blocchi di uscita del ciclo
- `verifyDeadCode`: verifica se un'istruzione marcata loop invariant viene usata fuori dal loop.

---

## 3. Move

Infine, l'insieme di tutte le istruzioni che sono marcate come movable dalla fase precedente vengono mosse nel pre-header del loop.
Questa operazione avviene seguendo l'ordine di dipendenza delle istruzioni dove, per poter spostare un'istruzione nel pre-header, è necessario che tutte le istruzioni a loro volta marcate come movable da cui essa dipende siano state già spostate.
Una istruzione A è dipendente da una istruzione B nel caso in cui B venga usata dall'istruzione A stessa.
Di conseguenza uno spostamento di A prima di B comporterebbe l'uso di B senza averla prima definita.

Per implementare questa fase abbiamo creato la funzione:

- `moveInstructions` muove le istruzioni marcate come movable
- `hasDependencies` verifica la dipendenza fra due istruzioni
