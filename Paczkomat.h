#pragma once
#include <string>
#include <vector>
#include "Skrytka.h"
#include "Paczka.h"

class Paczkomat {
private:
    std::string adres;
    std::string numer;
    std::vector<Skrytka> skrytki;

public:
    Paczkomat(std::string a, std::string num, std::vector<Skrytka> s);

    // GUI-friendly — zwracają true przy sukcesie
    bool odbiorGUI(const std::string& tel, int kod);

    // nadanie klienckie (tel + kod podaje klient)
    bool nadanieGUI(const std::string& tel, int kod, char rozmiar);

    // nadanie kurierskie (kod generuje system, tel bierze z listy dostaw)
    // zwraca przydzielony kod lub -1 gdy brak miejsca
    int  nadanieKurierGUI(const std::string& tel, char rozmiar);

    // reset skrytki przez serwisanta (usuwa paczkę bez kodu)
    bool resetSkrytki(int numerSkrytki);

    // dostęp tylko do odczytu dla renderowania
    const std::vector<Skrytka>& getSkrytki() const;
};
