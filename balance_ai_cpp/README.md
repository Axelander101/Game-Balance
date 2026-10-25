# General Game Balance AI (C++20)

Eine allgemeine, modulare Balance- und Game-Test-KI, entstanden aus dem V5-Tower-Defense-Balancer.

## Unterstützte Spieltypen

- `tower_defense`
- `arena_survival`
- `deckbuilder`
- `roguelike`
- `turn_strategy`
- `auto_battler`

Der Evolutionskern ist für alle Genres derselbe. Jedes Genre liefert nur:

1. seine tunebaren Balance-Parameter,
2. seine Strategie-Gene,
3. seine Simulation,
4. seinen Zielscore.

## Wichtige Funktionen

- Strategy-Genome + Balance-Genome
- Co-Evolution von Spielerstrategie und Spielbalance
- automatische Meta-Anpassung nach Balanceänderungen
- adaptive Mutation
- Diversity Pressure
- Pareto-Front (Leistung + Novelty)
- Hall of Fame
- adversariale Worst-Seed-Validation
- Sensitivity Analysis
- Anti-Meta-Experimente
- Exploit-/Dominanz-Erkennung
- Multi-Thread-Simulation
- Checkpoint + Resume
- Zeitlimit
- JSON Input/Output ohne externe C++-Bibliotheken
- CSV- und Text-Reports

## Ordner

```text
balance_ai_cpp/
├── CMakeLists.txt
├── include/gba/
├── src/
├── config/
├── samples/
├── tests/
└── java_bridge/
```

## Schnelltest in CLion

Öffne diesen Ordner in CLion. CMake sollte automatisch erkannt werden.

Target `gba_selftest` ausführen. Erwartet wird am Ende:

```text
SELFTEST 6/6 + End-to-End OK
```

Danach kann `game_balance_ai` gestartet werden.

Beispiel:

```bash
./game_balance_ai \
  --game samples/tower_defense.json \
  --config config/quick_test.json \
  --out runs/td_test
```

## Tower-Defense-Java-Bridge

Die Dateien unter `java_bridge/ch/alexb/tower/balancebridge/` gehören im bestehenden Java-Projekt nach:

```text
src/ch/alexb/tower/balancebridge/
```

`GeneralBalanceDataExporter` liest die echten Tower- und Gegnerwerte aus den Java-Klassen und erzeugt:

```text
balance_data/tower_defense.json
```

`GeneralBalanceLauncher` findet die in CLion gebaute C++-Executable und startet einen 3h-TD-Lauf.

`GeneralBalancePatch` liest die von C++ erzeugten Balance-Gene wieder ein.

## Reports

Ein Lauf erzeugt unter anderem:

```text
generations.csv
balance_genomes.csv
pareto_front.csv
exploit_candidates.csv
adversarial_seeds.csv
sensitivity.csv
best_strategy.json
current_balance.json
balance_patch.txt
checkpoint.json
summary.txt
```

## Neues Genre hinzufügen

1. neue Klasse von `IGameModule` ableiten,
2. `balanceParameters()` definieren,
3. `strategyParameters()` definieren,
4. `simulate()` implementieren,
5. optional eigenen `evaluateBalance()` und Anti-Meta-Experimente ergänzen,
6. Modul in `createGameModule()` registrieren.

Der Evolutionsalgorithmus muss dafür nicht verändert werden.

## Desktop UI / Balance Studio

Zusätzlich zur Kommandozeile gibt es jetzt die optionale Qt-6-Oberfläche `game_balance_studio`.
Sie startet denselben `game_balance_ai`-Backendprozess und zeigt Logs und Reports direkt an.

Auf macOS siehe `SETUP_UI_MAC.md`.

Wenn Qt 6 nicht installiert ist, baut CMake weiterhin CLI und Selftests. Es geht dadurch keine bestehende Funktion verloren.
