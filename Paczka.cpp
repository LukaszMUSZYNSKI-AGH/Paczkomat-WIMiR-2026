#include "Paczka.h"

Paczka::Paczka(int kod, std::string tel, char r)
    : kodPaczki(kod), telefonOdbiorcy(tel), rozmiar(r)
{}

int Paczka::getKodPaczki() const {
    return kodPaczki;
}

std::string Paczka::getTelOdbiorcy() const {
    return telefonOdbiorcy;
}

char Paczka::getRozmiarPaczki() const {
    return rozmiar;
}
