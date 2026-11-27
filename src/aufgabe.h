#ifndef RECHENTRAINER_AUFGABE_H
#define RECHENTRAINER_AUFGABE_H

enum class Rechenart {
    Addition,
    Subtraktion
};

enum class Zahlenraum {
    Bis10 = 10,
    Bis100 = 100,
    Bis1000 = 1000
};

struct Aufgabe {
    int ersteZahl;
    int zweiteZahl;
    Rechenart rechenart;
};

#endif
