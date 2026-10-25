# Architektur

```text
                    +---------------------------+
                    |     EvolutionEngine       |
                    | Strategy + Balance Genome |
                    +-------------+-------------+
                                  |
                +-----------------+------------------+
                |                                    |
        Strategy Evolution                   Balance Evolution
        + Diversity/Pareto                  + Meta Adaptation
                |                                    |
                +-----------------+------------------+
                                  |
                          IGameModule
                                  |
     +-------------+--------------+--------------+-------------+
     |             |              |              |             |
 Tower Defense   Arena        Deckbuilder    Roguelike    Strategy/Auto
```

Der Core kennt keine Tower, Karten oder RPG-Items. Er kennt nur `ParameterSpec`, `Genome`, `SimulationResult` und Zielmetriken.
