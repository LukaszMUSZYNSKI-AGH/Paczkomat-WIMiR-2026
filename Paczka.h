#pragma once
#include <string>

class Paczka {
private:
    int kodPaczki;
    std::string telefonOdbiorcy;
    char rozmiar;

public:
    Paczka(int kod, std::string tel, char r);

    int getKodPaczki() const;
    std::string getTelOdbiorcy() const;
    char getRozmiarPaczki() const;
};
