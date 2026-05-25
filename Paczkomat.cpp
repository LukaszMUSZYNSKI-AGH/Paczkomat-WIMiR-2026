#include "Paczkomat.h"
#include <cstdlib>   // rand
#include <ctime>

Paczkomat::Paczkomat(std::string a, std::string num, std::vector<Skrytka> s)
    : adres(a), numer(num), skrytki(s)
{
    srand((unsigned)time(nullptr));
}

// ── Odbiór (klient podaje tel + kod) ──────────────────────────────────────
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

// ── Nadanie klienckie (klient podaje tel + własny kod) ────────────────────
// Sprawdza unikalność tel i kodu przed włożeniem.
bool Paczkomat::nadanieGUI(const std::string& tel, int kod, char rozmiar) {
    // walidacja duplikatów
    for (const Skrytka& x : skrytki) {
        if (!x.czyZajeta()) continue;
        const auto& p = x.getPaczka();
        if (!p.has_value()) continue;
        if (p->getTelOdbiorcy() == tel) return false;   // tel zajęty
        if (p->getKodPaczki()   == kod) return false;   // kod zajęty
    }

    // szukamy skrytki — najpierw żądany rozmiar, potem większe
    static const char kolejnosc[] = {'A','B','C'};
    int start = 0;
    for (int i = 0; i < 3; ++i) if (kolejnosc[i] == rozmiar) { start = i; break; }

    for (int i = start; i < 3; ++i) {
        for (Skrytka& x : skrytki) {
            if (!x.czyZajeta() && x.getRozmiarSkrytki() == kolejnosc[i]) {
                Paczka p(kod, tel, kolejnosc[i]);
                x.wkladaniePaczki(p);
                return true;
            }
        }
    }
    return false;   // brak miejsca
}

// ── Nadanie kurierskie (system generuje unikalny kod) ─────────────────────
// Zwraca przydzielony kod (> 0) lub -1 gdy brak miejsca / tel już istnieje.
int Paczkomat::nadanieKurierGUI(const std::string& tel, char rozmiar) {
    // sprawdź czy tel już w systemie
    for (const Skrytka& x : skrytki) {
        if (!x.czyZajeta()) continue;
        const auto& p = x.getPaczka();
        if (p.has_value() && p->getTelOdbiorcy() == tel) return -1;
    }

    // generuj unikalny 4-cyfrowy kod
    int kod = 0;
    for (int attempt = 0; attempt < 1000; ++attempt) {
        int kandydat = 1000 + rand() % 9000;  // 1000–9999
        bool zajety = false;
        for (const Skrytka& x : skrytki) {
            if (!x.czyZajeta()) continue;
            const auto& p = x.getPaczka();
            if (p.has_value() && p->getKodPaczki() == kandydat) { zajety = true; break; }
        }
        if (!zajety) { kod = kandydat; break; }
    }
    if (kod == 0) return -1;  // nie udało się wygenerować (ekstremalny przypadek)

    // szukamy skrytki — najpierw żądany rozmiar, potem większe
    static const char kolejnosc[] = {'A','B','C'};
    int start = 0;
    for (int i = 0; i < 3; ++i) if (kolejnosc[i] == rozmiar) { start = i; break; }

    for (int i = start; i < 3; ++i) {
        for (Skrytka& x : skrytki) {
            if (!x.czyZajeta() && x.getRozmiarSkrytki() == kolejnosc[i]) {
                Paczka p(kod, tel, kolejnosc[i]);
                x.wkladaniePaczki(p);
                return kod;
            }
        }
    }
    return -1;  // brak miejsca
}

// ── Reset skrytki (serwisant) ─────────────────────────────────────────────
bool Paczkomat::resetSkrytki(int numerSkrytki) {
    for (Skrytka& x : skrytki) {
        if (x.getNumer() == numerSkrytki && x.czyZajeta()) {
            x.wyjmowaniePaczki();
            return true;
        }
    }
    return false;
}

const std::vector<Skrytka>& Paczkomat::getSkrytki() const {
    return skrytki;
}
