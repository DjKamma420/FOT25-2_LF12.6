# Rechentrainer in C++

**[Webapp öffnen](https://djkamma420.github.io/FOT25-2_LF12.6/)**

Schulprojekt der Klasse FOT25-2. Wir entwickeln ein Lernprogramm, mit dem Grundschulkinder selbstständig die vier Grundrechenarten üben können.

## Team

Marcel, Andreas, Mika und Anton

## Spielablauf

- **Endlos** startet direkt mit Plus und Minus im Zahlenraum bis 10. Von Aufgabe 11 bis 99 geht der Zahlenraum bis 100. Ab Aufgabe 100 geht er bis 1.000.
- Mal kommt ab Serie 20 dazu, Geteilt ab Serie 50. Danach ist Geteilt bei normalen Aufgaben halb so wahrscheinlich wie jede andere Rechenart.
- Geteilt beginnt mit ganzen Ergebnissen. Ab Serie 100 haben die Ergebnisse eine Nachkommastelle, ab 200 zwei und ab 300 drei.
- Die Aufgaben werden länger, sobald die Stellenzahl ihrer Hauptrechenart ihr Maximum erreicht hat. Ab Serie 300 enthalten normale Aufgaben 14 Zahlen. Killer-Fragen haben bis zu 17 Zahlen.
- Jede zehnte Endlos-Aufgabe ist eine Killer-Frage mit 60 Sekunden Zeit. Ein Fehler oder das Zeitlimit beendet die Runde.
- Unter **Üben** lassen sich der Zahlenraum und beliebige Kombinationen aus Plus, Minus, Mal und Geteilt einstellen. Für Division sind bis zu 3 Nachkommastellen wählbar. Die Übung geht nach Fehlern weiter.
- Die App zeigt den gemeinsamen Multiplikator aus Aufgabentyp, Serie und Zeit. Der Zeitfaktor startet bei ×1,60 und fällt linear auf null bei 10 Sekunden. Eine richtige Antwort zählt weiterhin für die Serie.
- Der **Highscore** speichert die höchste Punktzahl aus Endlos zusammen mit der Serie aus genau dieser Runde. Die Daten liegen im Browser.

### Stellenzahl in Endlos

| Serie für Mal | Stellen je Faktor | Serie für Geteilt | Stellen von Dividend und Teiler |
| --- | --- | --- | --- |
| 20 | 1 × 1 | 50 | 2 ÷ 1 |
| 50 | 2 × 1 | 100 | 3 ÷ 1 |
| 100 | 2 × 2 | 125 | 3 ÷ 2 |
|  |  | 150 | 3 ÷ 3 |

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
