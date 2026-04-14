#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

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