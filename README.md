# Rechentrainer in C++

**[Webapp öffnen](https://djkamma420.github.io/FOT25-2_LF12.6/)**

Schulprojekt der Klasse FOT25-2. Wir entwickeln ein Lernprogramm, mit dem Grundschulkinder selbstständig die vier Grundrechenarten üben können.

## Team

Marcel, Andreas, Mika und Anton

## Spielablauf

- Unter **Einstellungen** lassen sich Plus, Minus, Mal, Geteilt, beide Mischungen oder alle Rechenarten auswählen.
- Beim Spielen stehen die Zahlenräume bis 10, bis 100 und bis 1000 zur Wahl. Geteilt-Aufgaben haben höchstens eine Nachkommastelle.
- **Üben** läuft nach einer falschen Antwort weiter. **Endlos** endet beim ersten Fehler und stellt als jede zehnte Aufgabe eine Killer-Frage mit drei bis fünf Zahlen.
- Im Endlosmodus mit **Alles** kommt Mal ab Serie 8 und Geteilt ab Serie 15 dazu. Einzelne Rechenarten können direkt geübt werden.
- Punkte berücksichtigen Antwortzeit, Aufgabentyp und richtige Antworten in Folge. Punkt- und Serienrekorde werden je Modus, Zahlenraum und Rechenart lokal gespeichert.

## Arbeitspakete

| Gantt | Arbeitspaket |
| --- | --- |
| 2.1 | Hauptmenü |
| 2.2 | Aufgabenauswahl |
| 2.3 | Aufgabengenerierung |
| 2.4 | Eingabe und Rückmeldung |
| 2.5 | Wiederholung |
| 2.6 | Zusatzfunktion für Serienrekorde |

## Umsetzung

Die Rechenlogik ist in C++ geschrieben. GitHub Actions kompiliert sie mit Emscripten zu WebAssembly und veröffentlicht die Webapp auf GitHub Pages. Die veröffentlichte App lässt sich über ein unterstütztes Browsermenü installieren und ist nach dem ersten vollständigen Laden offline spielbar.
