#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")


///  kad se startuje proces, prvo se na konzoli unese processId pa port replikatora na koji proces hoce da se konektuje, desni(glavni) replikator je na 27000, a levi na 27001
///  onda, kada proces uspostavi konekciju odmah salje poruku u kojoj navodi svoj processId, message deo ostavi prazan, i postavi odgovarajuci flag (struktura MESSAGE u common projektu)
///  kad posalje poruku, onda ceka odgovor od replikatora da li je registracija uspesna, to se utvrdi na osnovu flag-a u poruci koju proces primi od replikatora
///  ako registracija nije uspesna, zatvaras socket i onda mozes da biras kako ces dalje, ili ces da ugasis aplikaciju, ili ces opet da radis konektovanje i sve ovo opet
///  ako je registracija uspesna, onda mozes da startujes one 2 niti, jedna koja ce da prima poruke od replikatora, druga koja ce da salje poruke replikatoru

int main()
{
    
}
