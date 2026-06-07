# Implementazione della Loop Fusion (Quarto Assignment)

---

## Introduzione
La **Loop Fusion** è una tecnica di ottimizzazione del codice che consiste nell'unire i corpi di due cicli distinti e consecutivi in un unico ciclo. Questa trasformazione mira a:
* Ridurre l'overhead associato alle istruzioni del *control flow*.
* Migliorare la località dei dati nella cache, riutilizzando immediatamente i valori calcolati.

---

## 1. Verifica delle Condizioni di Adiacenza

La Loop Fusion richiede che i due cicli candidati siano adiacenti all'interno del flusso di esecuzione, ovvero che non vi siano operazioni rilevanti eseguite tra la fine del primo e l'inizio del secondo.

### Metodi Implementati
Per verificarne l'adiacenza, sono stati sviluppati i seguenti metodi:
* `verifyAdjacentLoops`: verifica l'adiacenza tra due cicli consecutivi. 
  * Per i cicli *non guarded*, l'exit block del primo deve coincidere con il preheader del secondo.
  * Per i cicli *guarded*, l'uscita della prima guardia deve portare direttamente al blocco della seconda guardia.
* `areNoPreHeaderInstruction`: controlla che nel preheader del secondo ciclo non siano presenti istruzioni aggiuntive oltre al branch incondizionato verso l'header.
* `getManualLoopGuard`: analizza manualmente il CFG per estrarre la branch instruction condizionale della guardia.
* `getExitFromGuard`: identifica l'effettivo blocco di uscita associato alla guardia di un ciclo protetto.

---

## 2. Analisi delle Iterazioni

Prima di fondere due cicli, il compilatore deve assicurarsi che entrambi eseguano lo stesso identico numero di iterazioni, detto anche **trip count**. Se i cicli avessero un trip count differente, la fusione eseguirebbe il corpo di uno dei due cicli troppe o troppe poche volte.

### Metodi Implementati
Per analizzare questo aspetto, sono state implementate le seguenti funzioni:
* `verifySameTripCount`: effettua l'analisi `ScalarEvolution` di LLVM per verificare che il contatore delle iterazioni coincida matematicamente per entrambi i cicli.
* `isForStructure`: controlla che i cicli seguano la struttura di un ciclo `for` (verificando che il latch contenga esattamente due istruzioni) e scarta la fusion in presenza di strutture `while` generiche, le quali renderebbero insicuro il calcolo statico delle iterazioni.

---

## 3. Verifica della Control Flow Equivalence

La **Control Flow Equivalence** garantisce che, se si entra nel primo ciclo, si entrerà sicuramente anche nel secondo ciclo, e viceversa. Questo impedisce di fondere un ciclo che viene sempre eseguito con uno che è subordinato a una condizione dinamica.

### Metodi Implementati
Per garantire la control flow equivalence viene utilizzato il metodo:
* `verifyControlFlowEquivalence`: utilizza l'analisi di dominanza (`DominatorTree`) e post-dominanza (`PostDominatorTree`). 
  > 💡 *Caso cicli protetti:* Nel caso di cicli protetti da guardie, sfrutta il metodo `isKnownPredicateAt` di `ScalarEvolution` per verificare se la condizione della prima guardia implichi logicamente la veridicità della seconda.

---

## 4. Analisi delle Dipendenze

Anche se due cicli sono adiacenti ed equivalenti nel flusso, la fusione potrebbe essere illegale se l'ordine degli accessi alla memoria viene invertito in modo non sicuro. Non devono esistere **backward dependencies**, ovvero scenari in cui il secondo ciclo legge o scrive dati che il primo ciclo modificherà solo in iterazioni successive.

### Metodi Implementati
Per analizzare la sicurezza dei dati sono stati integrati i seguenti metodi:
* `verifyDependencies`: verifica le dipendenze utilizzando `DependenceInfo`. Gli accessi vengono calcolati misurando il delta dei rispettivi puntatori tramite gli offset di `ScalarEvolution`. Se il delta è positivo (direzione *backward*) o se l'analisi è confusa/ambigua, la fusion viene bloccata.
* `getLoadStore`: scansiona i blocchi del ciclo per estrarre tutte le istruzioni di `Load` e `Store` necessarie al controllo delle dipendenze.

---

## 5. Fusione dei Cicli e Unificazione delle Variabili d'Induzione

Una volta superate con successo tutte le verifiche di legalità, il passo esegue la trasformazione effettiva unendo i corpi dei cicli e rimappando le variabili di induzione in modo che utilizzino un unico contatore condiviso.

### Metodi Implementati
Per gestire la trasformazione fisica del codice sono state create le seguenti funzioni:
* `LoopFusion`: il core del pass, che invoca i metodi di analisi e le fasi di verifica precedenti, avviando la fusione reale dei due cicli se tutte le condizioni sono soddisfatte.
* `getLoopInductionVariable` e `getInductionVariableDifference`: individuano le variabili d'induzione (`PHINode`) dei cicli e calcolano la differenza costante iniziale (delta) tra i loro punti di partenza.
* `inductionVariableFusion`: unifica le due variabili d'induzione creando una nuova istruzione di `add` basata sull'indice del primo ciclo e sostituendo tutti gli usi della variabile del secondo ciclo.
* `bodyConnect`: unisce i corpi dei due cicli, modificando il terminatore del primo corpo per farlo puntare all'inizio del secondo corpo.
* `headerExitConnect`: aggiorna i successori dell'header del primo ciclo per gestire correttamente l'uscita verso i blocchi esterni della nuova struttura fusa.
* `secondLoopHeaderToLatch` e `bypassSecondGuard`: isolano e collegano correttamente le vecchie strutture di controllo del secondo ciclo ormai integrato, eliminando le istruzioni ridondanti e ottimizzando il CFG finale tramite `EliminateUnreachableBlocks`.
