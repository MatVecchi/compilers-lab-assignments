# Implementazione della Loop Fusion del quarto Assignment 

La **Loop Fusion** è una tecnica di ottimizzazione del codice che consiste nell'unire i corpi di due loop distinti e consecutivi in un unico loop. Questa trasformazione mira a ridurre l'overhead associato alle istruzioni del control flow e a migliorare la località dei dati nella cache riutilizzando immediatamente i valori calcolati.

# (extra) Implementazione della Loop Fusion con rotated Loops
Si è incluso anche una versione del passo di Loop Fusion che lavora solo con rotated loops.
Un rotated loop è un loop dove la condizione di uscita/ripetizione è posta in corrispondenza del Latch e non nell'header.
Si è deciso di aggiungere questo passo per approfondimento extra.
Prima di eseguire il passo è necessario chiamare il passo LLVM `loop-rotate` che trasforma ogni loop in rotated loops.

---

## Come eseguire il passo
Per generare i file che sfruttano l'ottimizzazione di questo passo bisogna eseguire i seguenti comandi per i due esempi trattati `loop_fusion` e `loop_complex`:

### Loop fusion
```bash 
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_fusion.c -o loop_fusion.O0.ll && opt -passes=mem2reg -S loop_fusion.O0.ll -o loop_fusion.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopFusionOptPass.so -passes=loop-fusion-opt loop_fusion.O0.ll -o loop_fusion.OPT.ll && opt -passes=dot-cfg -cfg-dot-filename-prefix=loop_fusion loop_fusion.OPT.ll
```

### Loop fusion complesso
```bash 
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_complex.c -o loop_complex.O0.ll && opt -passes=mem2reg -S loop_complex.O0.ll -o loop_complex.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopFusionOptPass.so -passes=loop-fusion-opt loop_complex.O0.ll -o loop_complex.OPT.ll && opt -passes=dot-cfg -cfg-dot-filename-prefix=loop_complex loop_complex.OPT.ll
```

Per la variante del passo di ottimizzazione che sfrutta solamente i loop ruotati i comandi sono i seguenti:

### Loop fusion con loop ruotati
```bash 
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_fusion.c -o loop_fusion_rotated.O0.ll && opt -passes=mem2reg -S loop_fusion_rotated.O0.ll -o loop_fusion_rotated.O0.ll && opt -passes=loop-rotate -S loop_fusion_rotated.O0.ll -o loop_fusion_rotated.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopFusionRotatedOptPass.so -passes=loop-fusion-r-opt loop_fusion_rotated.O0.ll -o loop_fusion_rotated.OPT.ll && opt -passes=dot-cfg -cfg-dot-filename-prefix=loop_fusion_rotated loop_fusion_rotated.OPT.ll
```

### Loop fusion complesso con loop ruotati
```bash 
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -fno-discard-value-names -S -c loop_complex.c -o loop_complex_rotated.O0.ll && opt -passes=mem2reg -S loop_complex_rotated.O0.ll -o loop_complex_rotated.O0.ll && opt -passes=loop-rotate -S loop_complex_rotated.O0.ll -o loop_complex_rotated.O0.ll && opt -S --load-pass-plugin ../BUILD/libLoopFusionRotatedOptPass.so -passes=loop-fusion-r-opt loop_complex_rotated.O0.ll -o loop_complex_rotated.OPT.ll && opt -passes=dot-cfg -cfg-dot-filename-prefix=loop_complex_rotated loop_complex_rotated.OPT.ll
```

questo genera anche un file `.dot` per ogni caso di ottimizzazione che permette di visualizzare il CFG in maniera grafica.

## 1. Verifica delle Condizioni di Adiacenza 

La Loop Fusion richiede che i due loop candidati siano adiacenti all'interno del flusso di esecuzione, ovvero che non vi siano operazioni rilevanti eseguite tra la fine del primo e l'inizio del secondo.
Per verificarne l'adiacenza, abbiamo implementato i seguenti metodi: 

* `verifyAdjacentLoops`: verifica l'adiacenza tra due loop consecutivi. Per i loop non guarded, l'exit block del primo deve coincidere con il preheader del secondo; 
* per i loop guarded, l'uscita della prima guardia deve portare direttamente al blocco della seconda guardia.
* `areNoPreHeaderInstruction`: controlla che nel preheader del secondo loop non siano presenti istruzioni aggiuntive oltre al branch incondizionato verso l'header.
* `getExitFromGuard`: identifica l'effettivo blocco di uscita associato alla guardia di un loop protetto.

---

## 2. Analisi delle Iterazioni 

Prima di fondere due loop, il compilatore deve assicurarsi che entrambi eseguano lo stesso identico numero di iterazioni, detto anche trip count. Se i loop avessero un trip count differente, la fusione eseguirebbe il corpo di uno dei due loop troppe o troppe poche volte. 

Per analizzare questo aspetto, abbiamo implementato le seguenti funzioni:
* `verifySameTripCount`: effettua l'analisi ScalarEvolution di LLVM per verificare che il contatore delle iterazioni coincida matematicamente per entrambi i loop.
* `isForStructure`: controlla che i loop seguano la struttura di un ciclo for (verificando che il latch contenga esattamente due istruzioni), e scarta la fusion in presenza di strutture while generiche che renderebbero insicuro il calcolo statico delle iterazioni.
(e' stato necessario creare questa funzione perchè tra il ciclo while, for ci sono delle leggere differenze in termini di struttura.
Infatti il ciclo while non ha un blocco di latch esplicito come nel for ma lo ha incorporato nell'ultimo blocco del body.)
---

## 3. Verifica della Control Flow Equivalence

La Control Flow Equivalence garantisce che, se si entra nel primo loop, si entrerà sicuramente anche nel secondo loop, e viceversa. Questo impedisce di fondere un loop che viene sempre eseguito con uno che è subordinato a una condizione dinamica. 
Per verificare questa condizione sono state utilizzate le analisi di dominanza e post dominanza.

Per garantire la control flow equivalence abbiamo implementato il metodo: 

* `verifyControlFlowEquivalence`: utilizza l'analisi di dominanza (DominatorTree) e post-dominanza (PostDominatorTree) verificando che il primo loop domini il secondo e che contemporaneamente il secondo loop post-domini il primo.
* Nel caso di loop protetti da guardie, sfrutta il metodo isKnownPredicateAt di ScalarEvolution per verificare se la condizione della prima guardia implichi logicamente la veridicità della seconda.

---

## 4. Analisi delle Dipendenze 

Anche se due loop sono adiacenti ed equivalenti nel flusso, la fusione potrebbe essere illegale se l'ordine degli accessi alla memoria viene invertito in modo non sicuro. Non devono esistere delle backward dependencies in cui il secondo loop legge o scrive dati che il primo loop modificherà solo in iterazioni successive. 

Per analizzare la sicurezza dei dati abbiamo implementato i seguenti metodi:
* `verifyDependencies`: verifica le dipendenza utilizzando la DependenceInfo. Gli accessi vengono calcolati misurando il delta dei rispettivi puntatori tramite gli offset di ScalarEvolution. Se il delta è positivo (direzione backward) o se l'analisi è confusa, la fusion viene bloccata.
* `getLoadStore`: scansiona i blocchi del loop per estrarre tutte le istruzioni di Load e Store necessarie al controllo delle dipendenze.

---

## 5. Fusione dei Loop e Unificazione delle Variabili d'Induzione 

Una volta superate tutte le verifiche di legalità, il passo esegue la trasformazione effettiva unendo i corpi dei loop e rimappando delle variabili di induzione in modo che utilizzino un unico contatore condiviso. 

Per implementare la trasformazione fisica del codice abbiamo creato le seguenti funzioni:
* `LoopFusion`: invoca i metodi di analisi e le fasi di verifica, avviando la fusione dei due loop se tutte le condizioni sono soddisfatte.
* `getLoopInductionVariable` e `getInductionVariableDifference`: individuano le variabili d'induzione (PHINode) dei loop e calcolano la differenza costante iniziale (delta) tra i loro punti di partenza.
* `inductionVariableFusion`: unifica le due variabili d'induzione creando una nuova istruzione di add basata sull'indice del primo loop e sostituendo tutti gli usi della variable del secondo loop.
* `bodyConnect`: unisce i corpi dei due loop, modificando il terminatore del primo corpo per farlo puntare all'inizio del secondo corpo.
* `headerExitConnect`: aggiorna i successori dell'header del primo loop per gestire correttamente l'uscita verso i blocchi esterni della nuova struttura fusa.
* `secondLoopHeaderToLatch` e `bypassSecondGuard`: isolano e collegano correttamente le vecchie strutture di controllo del secondo loop ormai integrato, eliminando le istruzioni ridondanti e ottimizzando il CFG finale tramite EliminateUnreachableBlocks.
