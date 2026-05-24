#pragma once
#include <string>
#include <vector>
#include "Skrytka.h"
#include "Paczka.h"

class Paczkomat {
private:
    std::string          adres;
    std::string          numer;
    std::vector<Skrytka> skrytki;

public:
    Paczkomat(std::string a, std::string num, std::vector<Skrytka> s);

    void ListaSkrytek() const;
    void odbior();
    void nadanie();

    // GUI-friendly variants: return true on success
    bool odbiorGUI(const std::string& tel, int kod);
    bool nadanieGUI(const std::string& tel, int kod, char rozmiar);

    // Read-only access to lockers for rendering
    const std::vector<Skrytka>& getSkrytki() const;
};
