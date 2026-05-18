# Implementazione della Loop Invariant Code Motion del Terzo Assignment

## 1. Implementazione della Loop Invariant
Metodi che gestiscono tutta la logica della loop invariant
### `isLoopInvariantOperand`

```cpp
bool isLoopInvariantOperand(Value *operand, Loop *const LL, SmallVector<Instruction *, 10> &loopInvariantInstructions)
```

* **Parametri:**
    * ​**operand**: Il puntatore al valore (Value*) da analizzare.
    * **​LL**: Il ciclo (Loop*) all'interno del quale si sta valutando l'invarianza.
    * **​loopInvariantInstructions:** Un vettore di supporto (SmallVector) utilizzato per tenere traccia delle istruzioni già identificate come invarianti
      
* **Logica di funzionamento:**
​ La funzione determina l'invarianza dell'operando seguendo questi tre criteri sequenziali:
   1. **Costanti e Argomenti:** Se l'operando è una costante (Constant) o un argomento della funzione (Argument), il suo valore è globalmente immutabile nel contesto del ciclo. La funzione ritorna immediatamente true.
   2. **Istruzioni esterne al loop:** Se l'operando è un'istruzione (Instruction) la cui definizione (il blocco genitore getParent()) si trova al di fuori del loop analizzato, il suo valore non può variare all'interno del ciclo. La funzione ritorna true.
   3. **Istruzioni interne al loop:** Se l'operando è un'istruzione definita dentro il loop, la sua invarianza dipende da come è stata generata. La funzione delega quindi il controllo al metodo ausiliario isLoopInvariantInstruction.  

​Se l'operando non soddisfa nessuno dei criteri precedenti, viene considerato non invariante e la funzione ritorna false.
  
### `isLoopInvariantInstruction`

---

## 2. Implementazione della Code Motion
Passo di ottimizzazione che applica la Strength Reduction a moltiplicazioni e divisioni intere positive con un operando costante.
In entrambi i casi deve esistere un operando costante ed un operando non costante.

* **Identità gestite:**
    * MOLTIPLICAZIONE (vale la proprietà commutativa):
    * $x \times 16 \rightarrow x << 4$
    * $x \times 7 \rightarrow (x << 2) + ( x << 1 ) + x$
    * $x \times 1 \rightarrow x$
    * $x \times 0 \rightarrow 0$

    * DIVISIONE (il valore costante deve essere al denominatore):
    * $x / 1 \rightarrow x$
    * $x / 16 \rightarrow x >> 4$
    * $x / 10 \rightarrow (x \times M ) >> k$
    
* **Identificazione delle operazioni binarie**: Per ogni istruzione di ogni Basic Block, viene verificato se si tratta di un'operazione binaria tramite un `dyn_cast<BinaryOperator>`. In caso positivo, viene controllata la tipologia di operazione ( **MUL**, **SDiv**).

* **Classificazione degli Operandi**: Attraverso un `dyn_cast<ConstantInt>`, il passo identifica quale dei due operandi è costante e quale è un registro:
    * L'operando costante viene assegnato a `constantValue`.
    * L'operando variabile viene assegnato a `registerOperand`.
Per l'operazione di divisione non viene considerato il caso in cui l'operando non costante sia al denominatore.
  
* **Ottimizzazione**: Attraverso la funzione `found_pow` si verifica la casistica specifica in cui ci si trova:
   * 0 -> moltiplicazione o divisione per 0
   * 1 -> moltiplicazione o divisione per 1
   * 2 -> moltiplicazione o divisione per potenze di 2
   * 3 -> moltiplicazione o divisione per costante generico
   * -1 -> moltiplicazione o divisione per numero negativo
La funzione calcola inoltre, nel caso di moltiplicazione o divisione per potenza di due, il logaritmo del valore costante, ottenendo così il secondo operando da inserire nella relativa shift.

* Le ottimizzazioni dipendono dal risultato ottenuto:
   * 0 & moltiplicazione -> si sostituisce l'operazione binaria con il valore 0 in tutti i suoi usi
   * 0 & divisione -> non ottimizzabile
   * 1 & (moltiplicazione | divisione ) -> si sostituisce l'operazione binaria con il relativo operando non costante
   * 2 & moltiplicazione -> si sostituisce l'operazione binaria con uno shift a sinistra
   * 2 & divisione -> si sostituisce l'operazione binaria uno shift a destra
   * 3 & moltiplicazione -> si scompone la moltiplicazione come una somma di shift a sinistra con la relativa funzione `sommaShift`
   * 3 & divisione -> applico la magic division con la funzione `magicDiv`
   * -1 & (moltiplicazione | divisione ) -> non gestito
     
* **Rimozione Binary operation**: Tutte le operazioni binarie che sono state ottimizzate vengono inserite all'interno di un vettore `toDelete`.
Alla fine del passo ciascuna di queste istruzioni verrà rimossa con il metodo `eraseFromParent`, applicando così una dead code elimination locale al basic block.
