#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
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

bool is_terminator(const unsigned char* c) {
	return *c == '\r' && c[1] == '\n';
}

bool is_single_space(const unsigned char* c) {
	return *c == ' ' && c[1] != ' ';
}

bool is_letter(const unsigned char* c) {
	if (*c >= 'a' && *c <= 'z') return true;
	if (*c >= 'A' && *c <= 'Z') return true;
	return false;
}

bool is_valid_char(const unsigned char* c) {
	if (is_letter(c)) return true;
	if (is_single_space(c)) return true;
	if (is_terminator(c)) return true;
	return false;
}

bool is_valid_query(const unsigned char *buf, size_t len) {
	if (len == 2 && is_terminator(buf)) return true;
	if (!is_letter(buf)) return false;
	if (!is_letter(&buf[len - 1])) return false;

	for (size_t i = 1; i < len; i++) {
		if (!is_valid_char(&buf[i])) return false;
	}

	return true;
}

ssize_t generate_error(unsigned char *buf) {
	return sprintf(reinterpret_cast<char *>(buf), "ERROR");
}

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

	if (listen(server_fd, 10) < 0) {
		perror("listen");
		return 1;
	}

	const int epoll_fd = epoll_create1(0);

	epoll_event ev{};
	ev.events = EPOLLIN;
	ev.data.fd = server_fd;

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

	epoll_event events[10];

	while (true) {
		const int n = epoll_wait(epoll_fd, events, 10, -1);

		for (int i = 0; i < n; i++) {
			const int fd = events[i].data.fd;

			if (fd == server_fd) {
				const int client_fd = accept(server_fd, nullptr, nullptr);

				epoll_event cev{};
				cev.events = EPOLLIN;
				cev.data.fd = client_fd;

				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &cev);

			} else {
				char buffer[1024];
				ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
				if (bytes <= 0) {
					if (close(fd)) {
						perror("close");
						return 1;
					}

					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

				} else {
					//validate data
					//store data OR process data
					//generate response
					//validate response

					auto buf = reinterpret_cast<unsigned char*>(buffer);
					if (!is_valid_query(buf, bytes)) {
						generate_error(buf);
					}

					send(fd, buffer, bytes, 0);
				}
			}
		}
	}

	return 0;
}