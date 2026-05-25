#include "Skrytka.h"
#include "Paczka.h"
#include <iostream>

Skrytka::Skrytka(int n, char r)
    : numer(n), rozmiar(r), Zajeta(false), Zamknieta(true), paczka(std::nullopt)
{}

void Skrytka::otworz() {
    Zamknieta = false;
}

void Skrytka::zamknij() {
    Zamknieta = true;
}

void Skrytka::wkladaniePaczki(const Paczka& p) {
    if (Zajeta) {
        std::cout << "Skrytka jest zajeta!" << std::endl;
        return;
    }
    paczka = p;      // optional przechowuje cały obiekt Paczka
    Zajeta = true;
}

void Skrytka::wyjmowaniePaczki() {
    if (!Zajeta) {
        std::cout << "Skrytka jest pusta!" << std::endl;
        return;
    }
    paczka = std::nullopt; // jedna linijka zamiast zerowania trzech pól
    Zajeta = false;
}

bool Skrytka::czyZajeta() const {
    return Zajeta;
}

bool Skrytka::czyZamknieta() const {
    return Zamknieta;
}

char Skrytka::getRozmiarSkrytki() const {
    return rozmiar;
}

int Skrytka::getNumer() const {
    return numer;
}

const std::optional<Paczka>& Skrytka::getPaczka() const {
    return paczka;
}
