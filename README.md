<img src="web/assets/icon-192.png" width="72" align="right" alt="">

# Rechentrainer in C++

Ein Rechenspiel der Klasse FOT25-2, mit dem Grundschulkinder die vier Grundrechenarten selbstständig üben können. Im Modus **Endlos** steigt die Schwierigkeit mit deiner Serie.

**[Rechentrainer im Browser öffnen](https://djkamma420.github.io/FOT25-2_LF12.6/)**

## Hauptmenü

| Auswahl | Funktion |
| --- | --- |
| **Endlos** | Starte im Zahlenraum bis 10 und spiele, bis du einen Fehler machst oder bei einer Killer-Frage die Zeit abläuft. |
| **Üben** | Wähle den Zahlenraum bis 10, 100 oder 1.000 und kombiniere die Rechenarten frei. Für Geteilt sind bis zu drei Nachkommastellen einstellbar. Nach einem Fehler geht die Übung weiter. |
| **Highscore** | Zeigt deine fünf besten abgeschlossenen Endlos-Läufe mit Punkten, Serie, Dauer, höchstem erzieltem Multiplikator und gelösten Killer-Fragen. Bei Punktgleichstand entscheidet die längere Serie. Die Daten bleiben im Browser. |
| **i** | Öffnet die Spielregeln, die Erklärung zur Punktewertung und die Teamübersicht. |

## So läuft Endlos

| Aufgabe | Zahlenraum |
| --- | ---: |
| 1–10 | Bis 10 |
| 11–99 | Bis 100 |
| Ab 100 | Bis 1.000 |

Du beginnst mit Plus und Minus. Mal kommt nach 20 richtigen Antworten in Folge dazu, Geteilt nach 50. Danach ist Geteilt bei normalen Aufgaben halb so wahrscheinlich wie jede andere Rechenart. Divisionen ergeben zunächst ganze Zahlen. Nach 100, 200 und 300 richtigen Antworten in Folge haben ihre Ergebnisse jeweils eine, zwei und drei Nachkommastellen.

Jede zehnte Aufgabe ist eine **Killer-Frage** mit 60 Sekunden Zeit. Sobald die Zahlen einer Rechenart ihre maximale Stellenzahl erreicht haben, werden ihre Aufgaben länger. Ab Serie 300 enthalten normale Aufgaben 14 Zahlen und Killer-Fragen bis zu 17.

### Stellenzahl für Mal und Geteilt

| Serie für Mal | Stellen je Faktor | Serie für Geteilt | Stellen von Dividend und Teiler |
| --- | --- | --- | --- |
| 20 | 1 × 1 | 50 | 2 ÷ 1 |
| 50 | 2 × 1 | 100 | 3 ÷ 1 |
| 100 | 2 × 2 | 125 | 3 ÷ 2 |
|  |  | 150 | 3 ÷ 3 |

### Punkte

Normale Aufgaben bringen 100 Grundpunkte, Killer-Fragen 200. Der angezeigte Multiplikator verbindet den Aufgabenfaktor (×1 oder ×1,50), den Serienfaktor und den Zeitfaktor. Die Grundpunkte werden mit diesem Wert multipliziert.

Der Serienfaktor startet bei ×1 und steigt alle fünf richtigen Antworten um 0,25 bis ×2. Jede gelöste Killer-Frage bringt zusätzlich 0,10. Der Zeitfaktor beginnt bei ×1,60 und sinkt innerhalb von zehn Sekunden gleichmäßig auf ×0. Für die erste richtige Antwort nach fünf Sekunden gibt es zum Beispiel **80 Punkte**. Auch bei null Punkten wächst die Serie weiter, wenn die Antwort richtig ist.

## Team

- Mika Christians
- Marcel Enns
- Lars Anton Roßbach
- Andreas Pizelkin

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
