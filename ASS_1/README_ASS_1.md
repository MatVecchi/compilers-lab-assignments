# Implementazione dei 3 passi LLVM del Primo Assignment

## 1. Algebric Identity Pass
AlgebricIdOptsPass implementation è un passo che verifica l'identità algebrica. Di seguito sono mostrati gli scenari ottimizzabili e la relativa trasformazione.

* **Identità gestite:**
    * $x + 0 == 0 + x \rightarrow x$
    * $x \times 1 == 1 \times x \rightarrow x$
    * $x - 0 \rightarrow x$
    * $x / 1 \rightarrow x$
      
* **Identificazione delle operazioni binarie**: Per ogni istruzione di ogni Basic Block, viene verificato se si tratta di un'operazione binaria tramite un `dyn_cast<BinaryOperator>`. In caso positivo, viene controllata la tipologia di operazione ( **ADD**, **SUB**, **MUL**, **SDiv**).
  
* **Classificazione degli Operandi**: Attraverso un `dyn_cast<ConstantInt>`, il passo identifica quale dei due operandi è costante e quale è un registro:
    * L'operando costante viene assegnato a `constantValue`.
    * L'operando variabile viene assegnato a `registerOperand`.
    
* **Gestione della Commutatività**: Viene impostato il flag booleano `firstOperandRegister` che identifica se il parametro non costante è il primo o il secondo operando.
Questo parametro permette inoltre di poter identificare quando l'ottimizzazione non è possibile a causa della mancanza della proprietà commutativa della sottrazione e della divisione.

* **Ottimizzazione**: Ogni volta che si incontra una identità algebrica, la si sostituisce direttamente con il parametro non costante.
Per fare ciò si utilizza il metodo `replaceAllUsesWith` sull’operazione binaria da ottimizzare.

* **Dead Code elimination**: Tutte le operazioni binarie che sono state ottimizzate vengono inserite all'interno di un vettore `toDelete`.
Alla fine del passo ciascuna di queste istruzioni verrà rimossa con il metodo `eraseFromParent`, applicando così una dead code elimination locale al basic block.


---

## 2. Strength Reduction Pass
è un passo che implementa l'ottimizzazione di strength reduction sia su moltiplicazioni positive con qualsiasi operando costante e che permette di poter ottimizzare tutte quante le divisioni positive con potenze di due.
In entrambi i casi deve esistere un operando costante ed un operando non costante.

* **Identità gestite:**
    * MOLTIPLICAZIONE (vale la proprietà commutativa):
    * $x * 16 --> x << 4
    * $x * 7 --> (x << 2) + ( x << 1 ) + x
    * $x * 1 --> x
    * $x * 0 --> 0

    * DIVISIONE (il valore costante deve essere al denominatore):
    * $x / 1 --> x
    * $x / 16 --> x >> 4
    
* **Identificazione delle operazioni binarie**: Per ogni istruzione di ogni Basic Block, viene verificato se si tratta di un'operazione binaria tramite un `dyn_cast<BinaryOperator>`. In caso positivo, viene controllata la tipologia di operazione ( **MUL**, **SDiv**).

* **Classificazione degli Operandi**: Attraverso un `dyn_cast<ConstantInt>`, il passo identifica quale dei due operandi è costante e quale è un registro:
    * L'operando costante viene assegnato a `constantValue`.
    * L'operando variabile viene assegnato a `registerOperand`.
Per l'operazione di divisione non viene considerato il caso in cui l'operando non costante sia al denominatore.
  
* **Ottimizzazione**: Attraverso la funzione `found_pow` si verifica la casistica specifica in cui ci si trova:
   * 0 --> moltiplicazione o divisione per 0
   * 1 --> moltiplicazione o divisione per 1
   * 2 --> moltiplicazione o divisione per potenze di 2
   * 3 --> moltiplicazione o divisione per costante generico
   * -1 --> moltiplicazione o divisione per numero negativo
La funzione calcola inoltre, nel caso di moltiplicazione o divisione per potenza di due, il logaritmo del valore costante, ottenendo così il secondo operando da inserire nella relativa shift.
Le ottimizzazioni dipendono dal risultato ottenuto:
   * 0 & moltiplicazione --> si sostituisce l'operazione binaria con il valore 0 in tutti i suoi usi
   * 0 & divisione --> non ottimizzabile
   * 1 & (moltiplicazione | divisione ) --> si sostituisce l'operazione binaria con il relativo operando non costante
   * 2 & moltiplicazione --> si sostituisce l'operazione binaria con uno shift a sinistra
   * 2 & divisione --> si sostituisce l'operazione binaria uno shift a destra
   * 3 & moltiplicazione --> si scompone la moltiplicazione come una somma di shift a sinistra con la relativa funzione `sommaShift`
   * 3 & divisione --> non gestito
   * -1 & (moltiplicazione | divisione ) --> non gestito
     
* **Dead Code elimination**: Tutte le operazioni binarie che sono state ottimizzate vengono inserite all'interno di un vettore `toDelete`.
Alla fine del passo ciascuna di queste istruzioni verrà rimossa con il metodo `eraseFromParent`, applicando così una dead code elimination locale al basic block.

---

## 3. Multi-Instruction Optimization Pass
Passo che verifica la multi-instruction optimization su somme, moltiplicazioni, divisioni e sottrazioni.
Nel momento in cui viene trovata una operazione ottimizzabile, viene sostituita con il registro non costante

* **Identità gestite:**
  * Scenari con valori costanti (vale la proprietà commutativa) (16 è d'esempio):
  * ($x + 16) - 16 --> x
  * ($x - 16) + 16 --> x
  * ($x * 16) / 16 --> x
  * ($x / 16) * 16 --> x

  * Scenari con operandi non costanti (si assume che si espanda sempre il primo operando non costante per semplicità):
  * ($x + $y) - y --> x
  * ($x - $y) + y --> x
  * ($x * $y) / y --> x
  * ($x / $y) * y --> x

* **Identificazione delle operazioni binarie**: Per ogni istruzione di ogni Basic Block, viene verificato se si tratta di un'operazione binaria tramite un `dyn_cast<BinaryOperator>`. In caso positivo, viene controllata la tipologia di operazione ( **MUL**, **ADD**, **SUB**, **SDiv**).

* **Classificazione degli Operandi**: Attraverso un `dyn_cast<ConstantInt>`, il passo identifica quale dei due operandi è costante e quale è un registro:
    * L'operando costante viene assegnato a `constantValue`.
    * L'operando variabile viene assegnato a `registerOperand`.
    * in caso di due operandi non costanti il secondo operando è assegnato a `secondregisterOperand`
Per poter eseguire l'identificazione viene richiamata la funzione `secondregisterValue` che ritorna inoltre il numero di operandi non costanti (espresso come un elemento di una enumerazione).
 
* **Ottimizzazione**: si espande l'operando non costante con la relativa istruzione (se possibile) e si verifica che l'istruzione nidificata sia con lo stesso valore costante, ma con operatore opposto, rispetto alla istruzione esterna.
Nel caso di due operandi non costanti si è deciso di espandere solamente il primo.
La casistica di ottimizzazione è gestita dalla funzione `isOpposite`.
In caso di ottimizzazione, si sostituisce a tutti gli uses dell'operazione binaria il registro non costante nidificato.

* **Dead Code elimination**: Tutte le operazioni binarie che sono state ottimizzate vengono inserite all'interno di un vettore `toDelete`.
Alla fine del passo ciascuna di queste istruzioni verrà rimossa con il metodo `eraseFromParent`, applicando così una dead code elimination locale al basic block.
