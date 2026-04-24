#include <thread>
#include <cassert>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <iostream>

struct TestCase {
	std::string name;
	std::vector<std::string> input;
	std::string expected;
};

std::vector<TestCase> tests = {
	{
		"\\r\\n",
		{"\r\n"},
		"0/0\r\n"
	},
	{

		"kajak",
		{"kajak\r\n"},
		"1/1\r\n"
	},
	{
		"Ala",
		{"Ala\r\n"},
		"1/1\r\n"
	},
{
	"Ala ma kota",
	{"Ala ma kota\r\n"},
	"1/3\r\n"
	},
{
	"_Ala ma kota",
	{" Ala ma kota\r\n"},
	"ERROR\r\n"
	},
{
	"Ala ma kota_",
	{"Ala ma kota \r\n"},
	"ERROR\r\n"
	},
{
	"m",
	{"m\r\n"},
	"1/1\r\n"
	},
{
	"mm",
	{"mm\r\n"},
	"1/1\r\n"
	},
	{
	"mn",
	{"mn\r\n"},
	"0/1\r\n"
	},
	{
		"mm mn",
		{"mm mn\r\n"},
		"1/2\r\n"
	},
{
	"Ala.",
	{"Ala.\r\n"},
	"ERROR\r\n"
	},
{
	"Ala__ma kota\\r\\n",
	{"Ala  ma kota\r\n"},
	"ERROR\r\n"
	},
{
	"Ala, ma, kota\\r, \\n",
	{"Ala", " ma ", "kota\r", "\n"},
	"1/3\r\n"
	},
{
	"\\r, \\n",
	{"\r", "\n"},
	"0/0\r\n"
	},
	{
	"a, b, a, \\r, \\n",
	{"a", "b", "a", "\r", "\n"},
	"1/1\r\n"
	},
	{
	"Ala ma kota\\r\\nkajak\\r\\n\\r\\nm\\r\\nab1\\r\\n1a\\r\\n1\\r, \\n",
	{"Ala ma kota\r\nkajak\r\n\r\nm\r\nab1\r\n1a\r\n1\r", "\n"},
	"1/3\r\n1/1\r\n0/0\r\n1/1\r\nERROR\r\nERROR\r\nERROR\r\n"
	},
	{
	"Ola ma kota\\r\\ndolary\\r, \\npiesek\\r\\nkok\\r, \\n",
	   {"Ola ma kota\r\ndolary\r", "\npiesek\r\nkok\r", "\n"},
	   "0/3\r\n0/1\r\n0/1\r\n1/1\r\n"
	},
	{
		"Ala i kot",
	   {"Ala i kot\r\n"},
	   "2/3\r\n"
	},
{
	"xyz, ucho oko",
   {"xyz\r\nucho oko\r\n"},
   "0/1\r\n1/2\r\n"
	},
{
	"b\\xc3\\xb3b i fasola\\r\\n",
   {"b\xc3\xb3b i fasola\r\n"},
   "ERROR\r\n"
	},
{
	"oraz\\x00zero\\r\\n",
   {"oraz\x00zero\r\n"},
   "ERROR\r\n"
	},
{
	"ABBA 1972\\r\\n",
   {"ABBA 1972\r\n"},
   "ERROR\r\n"
	},
{
	"a , b , c , d , e , f , g , h , i , j , k , l , m , n , o , p , q , r , s , t , u , v , w , x , y , z\\r\\n",
   {"a ", "b ", "c ", "d ", "e ", "f ", "g ", "h ", "i ", "j ",
   	"k ", "l ", "m ", "n ", "o ", "p ", "q ", "r ", "s ", "t ", "u ", "v ",
   	"w ", "x ", "y ", "z\r\n"},
   "26/26\r\n"
	},
{
	"kukurydza\\r\\n",
   {"kukurydza\r\n"},
   "0/1\r\n"
	},
{
	"kot, \\r\\npies",
   {"kot", "\r\npies"},
   "0/1\r\n"
	},
{
	"kot\\r, \\npies",
   {"kot\r", "\npies"},
   "0/1\r\n"
	}
};

std::string send_data(const std::string &host, const int port,
						   const TestCase &tc) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
	auto res = connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
	if (res < 0) {
		perror("connect");
		exit(1);
	}

	for (const auto& chunk : tc.input) {
		send(sock, chunk.c_str(), chunk.size(), 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	shutdown(sock, SHUT_WR);

	char buffer[1024];
	const auto n = recv(sock, buffer, sizeof(buffer), 0);
	close(sock);

	if (n > 0) {
		const std::string s{buffer};
		return s.substr(0, n);
	}

	return "";
}

bool run_test(const TestCase& tc) {
	std::string result;

	std::thread thr([&] {
			result = send_data("127.0.0.1", 2020, tc);
		});

	thr.join();
	assert(!result.empty());

	return result == tc.expected;
};

int main() {
	int passed = 0;

	for (const auto& tc : tests) {
		bool ok = run_test(tc);
		if (ok) {
			std::cout << "[PASS] " << tc.name << "\n";
			passed++;
		} else {
			std::cout << "[FAIL] " << tc.name << "\n";
		}
	}

	std::cout << passed << "/" << tests.size() << " passed\n";
	return 0;
}