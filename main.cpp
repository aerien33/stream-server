#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <vector>
#include <regex>

// BUF_SIZE >= 1024
constexpr ssize_t BUF_SIZE = 1024;

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

std::vector<std::string> divide(std::string &data, const std::string &del) {
	std::vector<std::string> parts;

	auto pos = data.find(del);
	while (pos != std::string::npos) {
		parts.push_back(data.substr(0, pos));
		data.erase(0, pos + del.length());
		pos = data.find(del);
	}

	if (!data.empty()) {
		//save unfinished query to vector state
	}

	return parts;
}

bool is_palindrome(const std::string &word) {
	if (word.empty()) return false;

	auto i = word.begin();
	auto j = word.end() - 1;

	while (i < j) {
		if (*i != *j && *i != *j - 32) return false;
		++i; --j;
	}

	return true;
}

bool is_valid_query(const std::string &data) {
	return std::regex_match(data, std::regex(R"(^([A-Za-z]+( [A-Za-z]+)*|)$)"));
}

bool is_valid_response(const std::string &data) {
	return std::regex_match(data,std::regex(R"(((\d+/\d+)*(ERROR)*\r\n)+)"));
}

std::string get_response(std::string &query) {
	std::string response;

	if (!is_valid_query(query)) {
		response = "ERROR";
	} else {
		auto words = divide(query, " ");
		auto palindromes = 0;

		for (const auto & word : words) {
			if (is_palindrome(word)) {
				palindromes++;
			}
		}

		response += palindromes + "/" + words.size();
	}

	return response + "\r\n";
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

	while (true) {
		epoll_event events[10];
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
				char buffer[BUF_SIZE];
				ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
				if (bytes < 2 || bytes >= BUF_SIZE) {
					if (close(fd)) {
						perror("close");
						return 1;
					}

					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

				} else {
					std::string data(buffer, bytes);
					auto queries = divide(data, "\r\n");

					std::string response;
					for (auto &query : queries) {
						response += get_response(query);
					}

					if (is_valid_response(response)) {
						send(fd, response.data(), response.size(), 0);
					} else {
						perror("invalid response");
						if (close(fd)) perror("close");
						return 1;
					}
				}
			}
		}
	}

	return 0;
}