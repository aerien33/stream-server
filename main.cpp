#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

/** ---- SPECYFIKACJA ----
Klient wysyła jedną lub więcej linii zawierających wyrazy.
Dla każdej odebranej linii serwer zwraca:
 - linię zawierającą albo obliczony wynik,
 - albo komunikat o błędzie.

Ogólna definicja linii jest zapożyczona z innych protokołów tekstowych:
ciąg drukowalnych znaków ASCII (być może pusty)
zakończony dwuznakiem \r\n pełniącym rolę terminatora linii.

Linia z zapytaniem klienta może zawierać tylko litery
oraz spacje pełniące rolę separatorów słów.
Obowiązują te same wymagania i interpretacje co w uprzednio rozważanym protokole datagramowym
(puste zapytanie z zero słów jest uznawane za poprawne, utożsamiamy wielkie i małe litery, itd.).

Linia z odpowiedzią serwera może zawierać albo dwa niepuste ciągi cyfr rozdzielone znakiem /,
albo pięć liter składających się na słowo „ERROR”.

(Uwaga na marginesie: wszystkie linie, i te wysyłane przez klientów, i przez serwer,
mają oczywiście do opisanej powyżej zawartości dołączony terminator linii, czyli \r\n.)

Serwer może, ale nie musi, zamykać połączenie w reakcji na nienaturalne zachowanie klienta.
Obejmuje to wysyłanie danych binarnych zamiast znaków ASCII,
wysyłanie linii o długości przekraczającej przyjęty w kodzie źródłowym serwera limit,
długi okres nieaktywności klienta itd. Jeśli serwer narzuca maksymalną długość linii,
to limit ten powinien wynosić co najmniej 1024 bajty (1022 drukowalne znaki i dwubajtowy terminator linii).

Serwer nie powinien zamykać połączenia gdy udało mu się odebrać poprawną linię w sensie ogólnej definicji,
ale dane w niej zawarte są niepoprawne (np. oprócz liter i spacji są przecinki).
Powinien wtedy zwracać komunikat błędu i przechodzić do przetwarzania następnej linii przesłanej przez klienta.
**/

int main() {
	const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		return 1;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2020);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(server_fd, 5) < 0) {
		perror("listen");
		return 1;
	}

	std::cout << "Server listening on port 2020...\n";

	sockaddr_in client_addr{};
	socklen_t client_len = sizeof(client_addr);

	const int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
	if (client_fd < 0) {
		perror("accept");
		return 1;
	}

	char buffer[1024] = {0};
	int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

	std::cout << "Received: " << buffer << "\n";

	const char* msg = "Hello from server";
	send(client_fd, msg, strlen(msg), 0);

	if (close(client_fd)) {
		perror("close");
		return 1;
	}

	if (close(server_fd) < 0) {
		perror("close");
		return 1;
	}

	return 0;
}