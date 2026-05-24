#include "Paczkomat.h"
#include <iostream>

Paczkomat::Paczkomat(std::string a, std::string num, std::vector<Skrytka> s)
    : adres(a), numer(num), skrytki(s)
{}

void Paczkomat::ListaSkrytek() const {
    for (const Skrytka& x : skrytki) {
        std::cout << "Numer: "  << x.getNumer()          << " | "
                  << "Rozmiar: " << x.getRozmiarSkrytki() << " | "
                  << (x.czyZajeta() ? "Zajeta" : "Wolna")
                  << std::endl;
    }
}

void Paczkomat::odbior() {
    int         kod;
    std::string tel;

    std::cout << "Nr telefonu: ";
    std::cin  >> tel;
    std::cout << "Kod odbioru: ";
    std::cin  >> kod;

    for (Skrytka& x : skrytki) {
        // sprawdzamy czy skrytka jest zajeta i ma paczke
        if (!x.czyZajeta()) continue;

        const auto& paczka = x.getPaczka();

        // has_value() sprawdza czy optional nie jest pusty
        if (paczka.has_value() &&
            paczka->getKodPaczki()   == kod &&
            paczka->getTelOdbiorcy() == tel)
        {
            x.otworz();
            std::cout << "Skrytka " << x.getNumer() << " otwarta. Prosze zabrac paczke." << std::endl;
            std::cout << "Nacisnij ENTER gdy zabierzesz paczke..." << std::endl;
            std::cin.ignore();
            std::cin.get();

            x.wyjmowaniePaczki();
            x.zamknij();
            std::cout << "Dziekujemy, do widzenia!" << std::endl;
            return;
        }
    }

    std::cout << "Nie znaleziono paczki o podanym kodzie i numerze telefonu." << std::endl;
}

void Paczkomat::nadanie() {
    int         kod;
    std::string tel;
    char        rozmiar;
    bool        wlozona = false;

    std::cout << "Podaj nr telefonu: ";
    std::cin  >> tel;
    std::cout << "Podaj kod odbioru: ";
    std::cin  >> kod;
    std::cout << "Podaj rozmiar paczki (A/B/C): ";
    std::cin  >> rozmiar;

    Paczka p(kod, tel, rozmiar);

    for (Skrytka& x : skrytki) {
        if (x.getRozmiarSkrytki() == rozmiar && !x.czyZajeta()) {
            x.otworz();
            std::cout << "Skrytka " << x.getNumer() << " otwarta." << std::endl;
            std::cout << "Wloz paczke i zamknij skrytke." << std::endl;
            std::cout << "Nacisnij ENTER gdy zamkniesz..." << std::endl;
            std::cin.ignore();
            std::cin.get();

            x.wkladaniePaczki(p);
            x.zamknij();
            std::cout << "Paczka przyjeta!" << std::endl;
            wlozona = true;
            break;
        }
    }

    if (!wlozona) {
        std::cout << "Brak wolnych skrytek rozmiaru " << rozmiar << std::endl;
    }
}

bool Paczkomat::odbiorGUI(const std::string& tel, int kod) {
    for (Skrytka& x : skrytki) {
        if (!x.czyZajeta()) continue;
        const auto& p = x.getPaczka();
        if (p.has_value() &&
            p->getKodPaczki()   == kod &&
            p->getTelOdbiorcy() == tel)
        {
            x.wyjmowaniePaczki();
            return true;
        }
    }
    return false;
}

bool Paczkomat::nadanieGUI(const std::string& tel, int kod, char rozmiar) {
    for (Skrytka& x : skrytki) {
        if (x.getRozmiarSkrytki() == rozmiar && !x.czyZajeta()) {
            Paczka p(kod, tel, rozmiar);
            x.wkladaniePaczki(p);
            return true;
        }
    }
    return false;
}

const std::vector<Skrytka>& Paczkomat::getSkrytki() const {
    return skrytki;
}
