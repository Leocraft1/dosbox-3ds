# Migrazione dosbox-3ds → dosbox-staging — piano e stato (Fase 0)

Documento di lavoro. Riassume quanto emerso analizzando sia il fork
abbandonato (`pre.zip`, stato attuale di Leocraft1/dosbox-3ds) sia il
repository ufficiale `dosbox-staging/dosbox-staging` (clonato e ispezionato
direttamente, non a memoria), e definisce il piano a fasi per la migrazione.

## 0. Decisioni già prese

- **Due binari separati**, build automatizzata via CI:
  - `dosbox-3ds-legacy` → Old 3DS/2DS + New 3DS (bonus clock/RAM quando
    disponibile). `SystemMode=80MB`, `SystemModeExt=124MB`.
  - `dosbox-3ds-n3ds` → New 3DS/2DS esclusivo. `SystemModeExt=178MB`
    (il massimo assoluto, non 174 — refuso da correggere nei tuoi appunti).
  - Unica differenza funzionale tra i due profili: la riga `SystemModeExt`
    nel rispettivo `.rsf`. Tutto il resto (clock 804MHz, L2 cache,
    accesso al secondo core) è identico in entrambi.
- Gestione di RAM/CPU per i modelli "inferiori" (Old 2DS/3DS con clock più
  basso) **rimandata**, come concordato — i due profili sopra bastano per
  ora e non precludono nulla in futuro.
- Mouse via Circle Pad/C-Stick: **già implementato** nel fork abbandonato
  (`ctr_input.cpp`, `axis_event()`), da riportare come riferimento diretto
  quando si affronterà quel punto, non da riprogettare da zero.
- UI a pannelli su schermo inferiore (tastiera/mouse/mapper/sistema): la
  struttura attuale va presa come riferimento di partenza; il gesto
  "tieni premuto ~0.5s al centro per attivare" va rivalutato in fase di
  redesign (non necessariamente eliminato, ma reso scopribile — es.
  un'icona o un prompt visivo al primo avvio).

## 1. Verifica ambiente di test (New 3DS XL)

- **Luma3DS**: SELECT all'avvio → menu di configurazione (mostra la
  versione), oppure in-game `L+Giù+Select` → Rosalina, versione+commit in
  basso a sinistra. Ultima release ufficiale verificata ora:
  **Luma3DS v13.5** (LumaTeam, 2 novembre 2026).
- **Firmware Nintendo**: System Settings → Other Settings, versione in
  basso a destra sullo schermo superiore. Il firmware 3DS non riceve più
  aggiornamenti da Nintendo: l'ultima versione mai pubblicata è
  **11.17.0-50** (maggio 2022) — se il sistema la mostra, è già definitivo.
- Annotare qui la build di Luma3DS/firmware usata per i primi test riusciti,
  come riferimento noto-funzionante nel caso l'hack SVC (sezione 4) smetta
  di funzionare dopo un futuro aggiornamento di Luma3DS.

## 2. Packaging: nota .3dsx vs .cia

Importante per non fraintendere i risultati dei test tra una build e
l'altra: **solo il `.cia`/`.cci` rispetta il manifest `.rsf`** (System
Mode, SystemModeExt, SystemCallAccess, ecc.), perché solo lì `makerom`
genera un vero ExHeader che il kernel applica alla creazione del processo.
Un `.3dsx` lanciato dall'Homebrew Launcher **non ha ExHeader**: la sua
memoria/priorità dipende dai default dell'Homebrew Launcher stesso (patchati
da Luma3DS), non da `dosbox-3ds.legacy.rsf`/`dosbox-3ds.n3ds.rsf`. Quindi:

- Per iterare rapidamente → `.3dsx`, ma la differenza 124MB/178MB tra i due
  profili **non sarà osservabile** in questa modalità.
- Per validare davvero i due profili di memoria → serve il `.cia`
  installato via FBI o simile.

## 3. Licenza

GPL-2.0-or-later (dosbox-staging) è compatibile con una distribuzione sotto
GPLv3, esercitando esplicitamente la clausola "or later" — nessun conflitto
legale. Resta da sistemare, quando riscriverai il `README.md`/i file di
testo: il `LICENSE` in root del fork attuale è testo integrale GPLv3,
mentre gli header nei singoli sorgenti dicono ancora "version 2 ... or any
later version". Rendilo esplicito da qualche parte (es. una riga nel
README) così non sembra un refuso a chi guarda la repo.

## 4. La scoperta architetturale che ridefinisce il piano

Dal clone diretto di `dosbox-staging/dosbox-staging` (non dal fork):

| Aspetto | Valore verificato oggi in dosbox-staging |
|---|---|
| Standard C++ | C++23 |
| Build system | CMake ≥3.25 + vcpkg (niente Meson, niente autotools) |
| Libreria finestre/input | **SDL3** (non SDL2) |
| Dipendenze vcpkg | asio, gtest, iir1, libmt32emu, libpng, opusfile, fluidsynth, sdl3 |
| Backend dynrec ARM disponibili | **solo `risc_armv8le.h` (AArch64)** e `risc_x64.h` |

Il punto critico è l'ultimo: **non esiste più, nel codice upstream, un
backend del dynamic recompiler per ARM a 32 bit** (il vecchio
`risc_armv4le.h` usato dal fork per l'ARMv6K del 3DS è stato rimosso dal
progetto). L'ARM11 MPCore del 3DS è strettamente a 32 bit: non può eseguire
le istruzioni AArch64 che `risc_armv8le.h` genera. Risultato: **oggi
dosbox-staging non ha alcun modo di fare JIT sulla CPU del 3DS.**

Lato positivo: il punto di estensione per impostare i permessi di memoria
del JIT è oggi molto più pulito che nel DOSBox classico. In
`src/cpu/dyn_cache.h` c'è un'unica funzione `dyn_mem_set_access(ptr, size,
execute)` con rami `#elif defined(HAVE_MPROTECT)` / `#elif defined(WIN32)`
— un ramo `#elif defined(__3DS__)` lì dentro, che chiami
`_SetMemoryPermission()` da `platform/ctr/src/ctr_memory.c` (vedi sezione
5), è l'unico punto da toccare per ricollegare l'hack, una volta scritto un
backend. Non ci sono più le due chiamate sparse e inconsistenti che c'erano
nel fork originale.

Anche `OPT_FORCE_DYNREC` ("Force DYNREC dynamic recompiler, not x86-64
optimized") esiste ancora come opzione CMake: il progetto prevede
concettualmente target non-x86-64, semplicemente nessuno ha scritto il
file per ARM a 32 bit nella base attuale. È quindi un componente mancante
ben definito da reinserire, non una riprogettazione del subsystem CPU.

## 5. Cosa consegna questa Fase 0 (file pronti nel repo)

- `toolchain/devkitarm-3ds.cmake` — toolchain file CMake per devkitARM
  (ARMv6K/MPCore). **Non testato contro un'installazione reale di
  devkitARM** (non disponibile in questo ambiente): verificare i path
  (`DEVKITPRO`/`DEVKITARM`, portlibs) e la compatibilità dei flag prima di
  fidarsene in CI.
- `.github/workflows/build-3ds.yml` — pipeline CI a matrice che produce i
  due binari (`dosbox-3ds-legacy`, `dosbox-3ds-n3ds`) via il container
  Docker ufficiale `devkitpro/devkitarm`. **Fallirà di proposito** finché
  non esiste un target CMake reale da compilare (Fase 1) — documenta la
  forma della pipeline, non è ancora operativa.
- `platform/ctr/rsf/dosbox-3ds.legacy.rsf` e `.../dosbox-3ds.n3ds.rsf` — i
  due manifest, identici tranne `SystemModeExt` (124MB vs 178MB) e
  `UniqueId` (diversi apposta, così i due CIA possono coesistere installati
  insieme; cambialo se preferisci che l'uno sostituisca l'altro).
- `platform/ctr/src/ctr_memory.c`/`.h` — versione corretta dell'hack SVC
  per ottenere memoria RWX. Rispetto all'originale: valori di ritorno
  propagati invece di un semplice `printf`, risultato cachato (non rilancia
  `svcBackdoor` ad ogni allocazione), e una `ctr_memory_jit_available()`
  interrogabile per decidere *prima* di eseguire codice JIT se è il caso di
  usarlo o ripiegare su un core interprete. **Non corregge** la fragilità
  intrinseca dell'offset kernel hardcoded in `ctrEnableAllServices()` —
  quello resta un rischio noto, non eliminabile senza re-reversare l'offset
  per ogni versione di kernel/Luma3DS. È pronto per essere agganciato al
  `dyn_mem_set_access()` di dosbox-staging in Fase 2, ma utilizzabile anche
  prima per qualunque altra necessità di memoria eseguibile.

## 6. Roadmap

### Fase 1 — bring-up funzionale (core_normal, nessun JIT)

Obiettivo: boot corretto e testabile sul New 3DS XL, senza velocità come
priorità. Lavoro principale: il layer `ctr_*` da riscrivere contro la
struttura più modulare di dosbox-staging (oggi niente più un unico
`sdlmain.cpp` monolitico da sostituire — ci sono `src/gui/mapper.cpp`,
`src/gui/render/`, `src/gui/clipboard.cpp`, `src/gui/titlebar.cpp`,
`src/gui/window_geometry.cpp` separati). Mappatura indicativa
responsabilità vecchio → nuovo (da raffinare quando si entra nel codice):

| File del fork abbandonato | Responsabilità | Area corrispondente in dosbox-staging |
|---|---|---|
| `ctr_main.cpp` | loop principale, init SDL/libctru | da sostituire con un ingresso dedicato, non più uno "sdlmain.cpp" unico |
| `ctr_gfx.h`/`.cpp` | asset e stato pannelli schermo inferiore | nuovo codice, nessun equivalente diretto in staging (era già tutto custom) |
| `ctr_input.cpp` | touch, tasti, Circle Pad/C-Stick → mouse/joystick | da agganciare a `src/gui/mapper.cpp` e all'input DOS (`KEYBOARD_AddKey`, `Mouse_CursorMoved`, `JOYSTICK_Move_*` — verificarne i nomi attuali in staging, possono essere stati rinominati nel refactor) |
| `ctr_memory.c`/`.h` | permesso RWX per il JIT | vedi sezione 4/5, aggancio a `dyn_mem_set_access()` |
| `Makefile.ctr` | build | sostituito da `toolchain/devkitarm-3ds.cmake` + target CMake dedicato |
| `ctr/template.rsf` | manifest CIA | `platform/ctr/rsf/*.rsf` (sezione 5) |

In questa fase: `cpu_core=normal` (o `simple`), nessun lavoro sul dynrec.

### Fase 2 — prestazioni (dynrec ARM a 32 bit)

Reinserire un backend per `core_dynrec` equivalente a `risc_armv4le.h` (e
companion `-common.h`/`-o3.h`/`-thumb*.h`) adattato alle interfacce attuali
di `decoder.h`/`decoder_basic.h`/`decoder_opcodes.h` di dosbox-staging (da
verificare quanto sono cambiate rispetto al DOSBox classico/al fork — non
assumere che siano identiche). Aggancio della gestione memoria tramite il
ramo `#elif defined(__3DS__)` in `dyn_mem_set_access()`, usando
`ctr_memory.c` corretto di questa Fase 0. Solo dopo che la Fase 1 produce
una baseline corretta e testata, per non confondere bug di correttezza con
bug di generazione di codice JIT.

### Non ancora pianificato (esplicitamente rimandato)

- Supporto a modelli Old 2DS/Old 3DS con profilo CPU/RAM dedicato oltre ai
  due profili attuali.
- Approfondimento mouse-su-joystick oltre a quanto già presente nel fork.
- Redesign del gesto di attivazione pannelli sullo schermo inferiore.
