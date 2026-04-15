# Implementazione dei 3 passi LLVM del Primo Assignment

## 1. Algebric Identity
AlgebricIdOptsPass implementation è un passo che verifica l'identita algebrica. Di seguito sono mostrati gli scenari ottimizzabili e la relativa trasformazione.

* **Identità gestite:**
    * $x + 0 == 0 + x \rightarrow x$
    * $x \times 1 == 1 \times x \rightarrow x$
    * $x - 0 \rightarrow x$
    * $x / 1 \rightarrow x$
* **Analisi e Identificazione**: Per ogni istruzione di ogni Basic Block, viene verificato se si tratta di un'operazione binaria tramite un `dyn_cast<BinaryOperator>`. In caso positivo, viene controllata la tipologia di operazione (es. **ADD**, **MUL**, **SDiv**).
* **Classificazione degli Operandi**: Attraverso un `dyn_cast<ConstantInt>`, il passo identifica quale dei due operandi è costante e quale è un registro:
    * L'operando costante viene assegnato a `constantValue`.
    * L'operando variabile viene assegnato a `registerValue`.
* **Gestione della Commutatività**: Viene impostato il flag booleano `firstOperandRegister` in base alla posizione dell'operando non costante. Questo parametro è fondamentale per estendere l'ottimizzazione a operazioni non commutative, come sottrazioni e divisioni, garantendo la correttezza della trasformazione.
* **Sostituzione degli Usi**: Se l'istruzione è ottimizzabile, viene invocato il metodo `replaceAllUsesWith` per rimpiazzare ogni riferimento alla `BinOp` originale con il relativo registro o con la nuova sequenza di istruzioni generata.
* **Rimozione Differita (Safe Deletion)**: Tutte le istruzioni destinate all'eliminazione (come le *Algebric Identities*) vengono inserite in un vettore `toDelete`. La rimozione effettiva tramite `eraseFromParent()` avviene solo al termine dell'analisi del Basic Block; questo evita di modificare la struttura del codice mentre gli iteratori sono ancora in uso, scongiurando errori di segmentazione o comportamenti indefiniti.


---

## 2. Strength Reduction
Questa è la sezione dedicata alla sostituzione di operazioni pesanti con alternative più leggere (quella che abbiamo visto nel codice precedente).

* **Logica applicata:** Trasformazione di moltiplicazioni e divisioni in shift (`shl`, `ashr`).
* **Casi supportati:**
    * Moltiplicazione per potenze di 2 ($2^k \rightarrow \ll k$).
    * Moltiplicazione per $2^k \pm 1$ (utilizzando shift + add/sub).
    * Divisione per potenze di 2 (usando `ashr`).
* **Dettagli Tecnici:** Menziona l'uso della funzione `found_pow` (o equivalenti APInt) per l'analisi delle costanti.

---

## 3. Multi-Instruction Optimization
Qui descrivi le ottimizzazioni che analizzano più istruzioni concatenate (peephole optimization o pattern matching più complessi).

* **Esempi tipici:**
    * **Combinazione di shift:** `(x << 2) << 3` diventa `x << 5`.
    * **Semplificazione Add/Sub:** `(x + a) - a` diventa `x`.
    * **Moltiplicazioni consecutive:** `(x * 2) * 4` diventa `x * 8` (e poi ridotta a shift).
* **Approccio:** Spiega se hai usato un approccio iterativo o se hai analizzato le catene di `User` delle istruzioni.
