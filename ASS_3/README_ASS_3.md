# Implementazione della Loop Invariant Code Motion del Terzo Assignment

## 1. Implementazione della Loop Invariant
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

* **Rimozione Binary operation**: Tutte le operazioni binarie che sono state ottimizzate vengono inserite all'interno di un vettore `toDelete`.
Alla fine del passo ciascuna di queste istruzioni verrà rimossa con il metodo `eraseFromParent`, applicando così una dead code elimination locale al basic block.


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
