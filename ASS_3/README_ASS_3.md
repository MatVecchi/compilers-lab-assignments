# Implementazione della Loop Invariant Code Motion del Terzo Assignment

## 1. Implementazione della Loop Invariant
Per la definizione di Loop Invariant si definisce invariante un'istruzione interna a un ciclo il cui risultato rimane costante durante tutte le iterazioni, permettendone lo spostamento all'esterno (code motion) per ottimizzare le prestazioni.

​Per identificarla formalmente si utilizza l'analisi delle reaching definitions, valutando ogni istruzione nella forma A = B + C. Un'istruzione viene marcata come loop-invariant se, per ciascuno dei suoi operandi (B e C), si verifica almeno una delle seguenti tre condizioni:


- Tutte le definizioni dell'operando che raggiungono l'istruzione si trovano fuori dal ciclo.
- L'operando è una costante letterale, il cui valore è intrinsecamente immutabile.
- Esiste una sola definizione raggiungente interna al ciclo, ed è già stata marcata come invariante.

​Per fare in modo che queste regole vengano rispettate abbiamo implementato i seguenti metodi:
- `isLoopInvariantOperand`: verifica se un operando è Loop Invariant
- `isLoopInvariantInstruction`: verifica se un istruzione è Loop Invariant
- `findLoopInvariant`: trova tutte le istruzioni Loop Invariant


---

## 2. Implementazione della Code Motion
Il fatto che un'istruzione sia loop-invariant è una condizione necessaria ma non sufficiente per poterla spostare. 
Un'istruzione invariante, infatti, non è automaticamente movable (spostabile): il compilatore deve prima applicare l'analisi di **Code Motion** per verificare che lo spostamento sia sicuro (safe) e non alteri il comportamento o la semantica del programma.
​Per poter muovere un'istruzione loop-invariant (es. A = B + C) nel preheader (il blocco che precede il ciclo), devono essere rigorosamente soddisfatte tre condizioni di sicurezza:



​1. **Dominanza del blocco di uscita:** Il blocco di codice in cui si trova l'istruzione deve dominare tutte le uscite del ciclo.

​2. **Unicità della definizione:** L'istruzione deve essere l'unica definizione all'interno del ciclo per quella specifica variabile (A). Se ci fossero altre istruzioni nel ciclo che assegnano un valore ad A, spostare l'invariante altererebbe l'ordine delle assegnazioni e distruggerebbe la corretta logica dei dati.

​3. **Dominanza dei punti d'uso:** Il blocco dell'istruzione deve dominare tutti gli usi della variabile definita (A) all'interno del ciclo.

Per poter verificare anche queste condizioni è stato necessario implementare i seguenti metodi:

- `verifyDominance`: verifica che il blocco contenente l'istruzione loop-invariant domini tutti i blocchi di uscita del ciclo
- `verifyCodeMotion`: verifica se un istruzione è Loop Invariant
- `findLoopInvariant`: trova tutte le istruzioni Loop Invariant
