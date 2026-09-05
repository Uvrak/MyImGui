# Kommunikation

- Antworte dem Benutzer immer auf Deutsch.
- Erklärungen, Rückfragen und Statusmeldungen müssen auf Deutsch sein.
- Bezeichner, API-Namen und Quellcode bleiben auf Englisch.

# Arbeitsweise

- Untersuche zuerst den vorhandenen Code, bevor du Änderungen vorschlägst.
- Erfinde keine Klassen, Funktionen, Dateien oder APIs, ohne vorher zu prüfen,
  ob sie im Repository existieren.
- Ändere nur das, was für die aktuelle Aufgabe notwendig ist.
- Keine großflächigen Refactorings ohne ausdrückliche Aufforderung.
- Bestehende Architekturentscheidungen sollen beibehalten werden.

# Build und Fehlerbehebung

- Nach Codeänderungen muss das betroffene Projekt oder die Solution gebaut werden.
- Bei Compiler- oder Linkerfehlern nicht aufhören.
- Lies die vollständige Fehlermeldung und ermittle zuerst die Ursache.
- Repariere Fehler iterativ und baue danach erneut.
- Entferne keinen bestehenden Code nur deshalb, um einen Buildfehler zu umgehen.
- Behaupte erst, dass eine Aufgabe abgeschlossen ist, wenn der Build erfolgreich ist,
  sofern ein Build möglich ist.

# Projektorganisation

- Neue eigenständige Funktionalität bekommt grundsätzlich eine eigene .h/.cpp-Datei.
- Neue Funktionalität nicht unnötig in bestehende, fachfremde Klassen einbauen.
- Generische Komponenten dürfen keine spielspezifische MM3-Logik enthalten.

# Architektur

- GridBuilderHost bleibt möglichst spielunabhängig.
- Might-and-Magic-III-spezifische Logik gehört in MightAndMagic3ReverseEngineering.
- MM3-Speicheradressen gehören nicht in generische Host-Komponenten.
- DOSBox-X-spezifische Funktionalität gehört möglichst in DosBoxX.
- Speicheranalyse- und Trace-Funktionalität gehört in DosBoxMemoryTools.

# Änderungssicherheit

- Vor Änderungen prüfen, welche Dateien und Klassen betroffen sind.
- Keine bestehenden Funktionen löschen, wenn ihre Aufgabe nicht vollständig verstanden wurde.
- Keine stillen Architekturänderungen durchführen.
- Wenn mehrere Lösungswege möglich sind, den kleinsten und risikoärmsten bevorzugen.