# Rechentrainer in C++

**[Webapp öffnen](https://djkamma420.github.io/FOT25-2_LF12.6/)**

Schulprojekt der Klasse FOT25-2. Wir entwickeln ein Lernprogramm, mit dem Grundschulkinder selbstständig Plus- und Minusaufgaben üben können.

## Team

Marcel, Andreas, Mika und Anton

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

Die Rechenlogik der Webapp wird in C++ geschrieben. Für den Browser ist ein WebAssembly-Build mit Emscripten und die Bereitstellung über GitHub Pages geplant.

## Web-Gerüst lokal starten

Voraussetzungen sind das aktivierte Emscripten SDK (`em++`) und Python 3. Die Befehle laufen in einer Bash im Hauptordner des Repositorys.

```bash
web_build=$(mktemp -d)
em++ -std=c++17 web/browser.cpp src/*.cpp -lembind -o "$web_build/rechentrainer.js"
cp web/index.html web/style.css "$web_build/"
python3 -m http.server 8000 --bind 127.0.0.1 --directory "$web_build"
```

Im Browser [localhost:8000](http://localhost:8000) öffnen und auf **C++ testen** klicken. Es erscheint **C++ läuft im Browser.**
