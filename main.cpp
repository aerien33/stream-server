#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <vector>
#include <regex>
#include <unordered_map>

// BUF_SIZE >= 1024
constexpr ssize_t BUF_SIZE = 1024;

std::vector<std::string> divide(std::string &data, const std::string &del) {
	std::vector<std::string> parts;

	auto pos = data.find(del);
	while (pos != std::string::npos) {
		parts.push_back(data.substr(0, pos));
		data.erase(0, pos + del.length());
		pos = data.find(del);
	}

	if (del == " " && !data.empty()) {
		parts.push_back(data.substr(0, pos));
		data.erase(0, pos + del.length());
	}

	return parts;
}

std::vector<std::string> get_queries(std::string &data) {
	return divide(data, "\r\n");
}

std::vector<std::string> get_words(std::string &data) {
	return divide(data, " ");
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
		auto words = get_words(query);
		auto palindromes = 0;

		for (const auto & word : words) {
			if (is_palindrome(word)) {
				palindromes++;
			}
		}

		response += std::to_string(palindromes) + "/" + std::to_string(words.size());
	}

	std::cout << "Response: \"" << response << "\"" << std::endl;
	return response + "\r\n";
}

int main() {
	const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket");
		return 1;
	}

	std::cout << "Server socket created: fd=" << server_fd << std::endl;

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

	std::unordered_map<int, std::string> state;

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
				std::cout << "Hey! Here is client_fd=" << fd << std::endl;

				char buffer[BUF_SIZE];
				ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

				if (bytes == -1) {
					perror("recv");
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
					continue;
				}

				if (bytes == 0) {
					std::cout << "Received 0 bytes from fd=" << fd << std::endl;
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
					continue;
				}

				std::string data;
				if (state.find(fd) != state.end()) {
					std::cout << "Found state of fd=" << fd << ", state=\"" << state[fd] << "\"" << std::endl;
					data = state[fd];
				}

				data += std::string(buffer, bytes);
				std::cout << "Data: \"" << data << "\"" << std::endl;

				auto queries = get_queries(data);
				for (const auto &query : queries) {
					std::cout << "Query: \"" << query << "\"" << std::endl;
				}

				if (!data.empty()) {
					state[fd] = data;
					std::cout << "Saving state for fd=" << fd << ": \"" << data << "\"" << std::endl;
				} else if (state.find(fd) != state.end()) {
					state.erase(fd);
					std::cout << "Deleting state of fd=" << fd << std::endl;
				}

				if (!state.empty()) {
					std::cout << "State: " << std::endl;
					for (auto &pair : state) {
						std::cout << "fd: " << pair.first << "-> data: \"" << pair.second << "\"" << std::endl;
					}
				}

				std::string response;
				for (auto &query : queries) {
					response += get_response(query);
				}

				if (!response.empty() && is_valid_response(response)) {
					send(fd, response.data(), response.size(), 0);
				}
			}
		}
	}

	return 0;
}