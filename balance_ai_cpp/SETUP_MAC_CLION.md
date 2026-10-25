# Mac + CLion: kurze Einrichtung

Die genaue Schritt-für-Schritt-Einrichtung kann direkt in ChatGPT begleitet werden. Für die spätere Verwendung sind diese Schritte vorgesehen:

1. Ordner `balance_ai_cpp` in CLion mit **Open** öffnen.
2. CLion soll Apple Clang / clang++ als Toolchain verwenden.
3. CMake laden lassen.
4. Target `gba_selftest` bauen und starten.
5. Danach Target `game_balance_ai` bauen.
6. Im Java-Projekt die drei Bridge-Dateien unter `src/ch/alexb/tower/balancebridge/` verwenden.
7. Java exportiert `balance_data/tower_defense.json`.
8. C++ schreibt seine Resultate nach `general_balance_runs/...`.

Falls Apple Command Line Tools fehlen:

```bash
xcode-select --install
```
