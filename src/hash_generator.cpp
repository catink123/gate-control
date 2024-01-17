#include <bcrypt.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <password-to-hash>", argv[0]);
		return 1;
	}

	std::string password(argv[1]);
	std::string password_hash = bcrypt::generateHash(password);

	printf("%s", password_hash.c_str());

	return 0;
}