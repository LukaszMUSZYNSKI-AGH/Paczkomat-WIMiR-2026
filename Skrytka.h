#pragma once
#include <string>
#include <optional>
#include "Paczka.h"

class Skrytka {
private:
    int numer;
    char rozmiar;
    bool Zajeta;
    bool Zamknieta;
    std::optional<Paczka> paczka;

public:
    Skrytka(int n, char r);

    void otworz();
    void zamknij();

    void wkladaniePaczki(const Paczka& p);
    void wyjmowaniePaczki();

    bool czyZajeta() const;
    bool czyZamknieta() const;
    char getRozmiarSkrytki() const;
    int  getNumer() const;

    const std::optional<Paczka>& getPaczka() const;
};
