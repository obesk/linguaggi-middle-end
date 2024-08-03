# Assignments linguaggi e compilatori middle-end

Assignment svolti per il corso di linguaggi e compilatori.

## Descrizione dei file

I file sono organizzati in assignmets. Le cartelle chiamate `ASSINGMENT{N}` contengono i sorgenti utilizzati per risolvere l'assignemt.

La cartella `common` contiene i file di llvm per buildare correttamente l'eseguibile con tutti i passi.

La cartella `TEST` contiene i programmi c per testare i passi di ottimizzazione creati. 

## Installazione

Per l'installazione è presente il file `install.sh`, che crearà dei symbolic link nel progetto LLVM in modo da includere i passi.

Procedura di installazione:

1. Posizionare il progetto all'interno della cartella del progetto LLVM
2. Entrare nella cartella del progetto e avviare l'installer:
    ```bash
    ./install.sh
    ```
    Attenzione: questa operazione sostituirà eventuali file omonimi
3. Ricompilare il progetto, posizionandosi nella cartella di build e avviando la compilazione
    ```bash
    cd ../BUILD
    make install -j18
    ```

## Testing dei passi

1. Posizionarsi nella cartella di test:
    ```bash
    cd TEST
    ```
2. Compilare i programmi di test:
    ```bash
    make
    ```

Per ogni programma `*.c` verranno creati 4 file:

- `.ll` la conversione a IR LLVM del programma in C
- `.optimized.ll` l'IR risultante a fine del passo testato
- `.optimized` l'eseguibile compilato partendo dall'IR ottimizzato
- `.original` l'eseguibile compilato partendo dal programma in C originale

