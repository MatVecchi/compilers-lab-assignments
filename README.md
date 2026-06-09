# Get Started
Per ogni assignment bisogna generare la build di ogni passo di ottimizzazione. In ogni directory degli assignment eseguire questi comandi:

```bash
mkdir BUILD
cd BUILD

cmake -DLT_LLVM_INSTALL_DIR=<bin LLVM> ../SRC
make
```

Se è la prima volta che si esegue LLVM bisogna inserire la directory bin nel PATH:
```bash
export PATH=~<bin LLVM>:$PATH
```