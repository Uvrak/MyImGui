# Tueren und Fenster

HouseOpenings verwendet fuer Tueren und Fenster dieselbe OpeningMotion-Zustandsmaschine.
Alle neu geladenen Teile starten offen und unverriegelt. Die historische Startposition in OWOPENINGS 1 wird gelesen, aber nicht mehr angewendet.

Zustaende: Open, Opening, Closed, Closing, Locked. Die Animation dauert bei regulaeren Frames 0,65 Sekunden und verwendet die bestehende geglaettete Scharnierrotation. Bewegungen sind umkehrbar; die Kollisionspruefung stoppt vor dem Charakter und setzt die Bewegung fort, sobald der Weg frei ist.

API auf HouseOpenings:
- toggle(index): Oeffnen/Schliessen; false bei locked.
- setOpen(index, bool): Zielposition setzen; false bei locked.
- setLocked(index, true): nur vollstaendig geschlossen moeglich; sonst false ohne Nebenwirkungen.
- setLocked(index, false): entriegeln, ohne automatisch zu oeffnen.
- state(index), locked(index), progress(index), blocked(index): Zustand abfragen.

Verriegelung ist ein Laufzeitzustand. Schluessel, Speicherung im Spielstand und Schloss-UI sind nicht Bestandteil dieser Aenderung.

Die neuen Wand-PNGs sind Entwurfsbilder und besitzen keine getrennte bewegliche Geometrie. Diese Logik bewegt die existierenden Scharnier-Meshes; sie erzeugt oder platziert keine neuen Gebaeude in Britannia.

Test: tests/opening_motion.cpp, standalone C++20 ohne Grafikabhaengigkeit. Prueft offenen Standard, Zwischenpositionen, Umkehr, Blockierung/Fortsetzung, Verriegelung und ungueltige Zeitschritte. Engine Debug/x64 ebenfalls gebaut.
