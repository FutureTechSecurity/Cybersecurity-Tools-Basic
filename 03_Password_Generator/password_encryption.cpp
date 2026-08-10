#include <sodium.h>
#include <iostream>
#include <string>

class PasswordManager {
public:

    // Hash a password using Argon2id
    static std::string hashPassword(const std::string& password) {

        char hash[crypto_pwhash_STRBYTES];

        if (crypto_pwhash_str(
                hash,
                password.c_str(),
                password.size(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE
            ) != 0) {

            throw std::runtime_error(
                "Password hashing failed: insufficient memory"
            );
        }

        return std::string(hash);
    }

    // Verify a password against an Argon2id hash
    static bool verifyPassword(
        const std::string& password,
        const std::string& hash
    ) {

        return crypto_pwhash_str_verify(
            hash.c_str(),
            password.c_str(),
            password.size()
        ) == 0;
    }
};


int main() {

    // Initialize libsodium
    if (sodium_init() < 0) {
        std::cerr << "[ERROR] Unable to initialize libsodium\n";
        return 1;
    }

    std::cout << "=== Secure Password System ===\n\n";

    std::string password;

    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    try {

        // Generate Argon2id password hash
        std::string passwordHash =
            PasswordManager::hashPassword(password);

        std::cout << "\nStored password hash:\n";
        std::cout << passwordHash << "\n";

        // Test password
        std::string testPassword;

        std::cout << "\nEnter password to verify: ";
        std::getline(std::cin, testPassword);

        if (PasswordManager::verifyPassword(
                testPassword,
                passwordHash)) {

            std::cout << "[PASS] Password is correct.\n";

        } else {

            std::cout << "[FAIL] Invalid password.\n";
        }

    }
    catch (const std::exception& e) {

        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
