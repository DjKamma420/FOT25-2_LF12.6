# Rechentrainer in C++

Schulprojekt der Klasse FOT25-2. Wir entwickeln ein Lernprogramm, mit dem Grundschulkinder selbstständig Plus- und Minusaufgaben üben können.

## Geplante Funktionen

- Zufällige Aufgaben in den Zahlenräumen bis 10, bis 100 und bis 1000
- Direkte Rückmeldung zur eingegebenen Antwort
- Weitere Aufgaben, bis die Übung beendet wird
- Als Bonus ein Rekord für richtige Antworten in Folge, jeweils pro Zahlenraum und Rechenart

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

Die Webapp und die Konsolenversion verwenden dieselbe Rechenlogik in C++. Für die Webapp ist ein WebAssembly-Build mit Emscripten und die Bereitstellung über GitHub Pages geplant. Die Konsolenversion ist für OnlineGDB vorgesehen.
