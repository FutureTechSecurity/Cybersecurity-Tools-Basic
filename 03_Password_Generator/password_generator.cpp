#include <sodium.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>


class PasswordManager {
public:

    // Generate a cryptographically secure random password.
    static std::string generatePassword(
        std::size_t length = 20,
        bool uppercase = true,
        bool lowercase = true,
        bool numbers = true,
        bool symbols = true
    ) {
        const std::string UPPER =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        const std::string LOWER =
            "abcdefghijklmnopqrstuvwxyz";

        const std::string NUMBERS =
            "0123456789";

        const std::string SYMBOLS =
            "!@#$%^&*()-_=+[]{};:,.?";

        std::string characterSet;
        std::string password;

        // Require at least one character from every selected category.
        if (uppercase)
            characterSet += UPPER;

        if (lowercase)
            characterSet += LOWER;

        if (numbers)
            characterSet += NUMBERS;

        if (symbols)
            characterSet += SYMBOLS;

        if (characterSet.empty()) {
            throw std::invalid_argument(
                "At least one character category must be enabled."
            );
        }

        if (length < 8) {
            throw std::invalid_argument(
                "Password length must be at least 8 characters."
            );
        }

        // Guarantee one character from each selected category.
        if (uppercase)
            password += randomCharacter(UPPER);

        if (lowercase)
            password += randomCharacter(LOWER);

        if (numbers)
            password += randomCharacter(NUMBERS);

        if (symbols)
            password += randomCharacter(SYMBOLS);

        // Fill remaining characters.
        while (password.length() < length) {
            password += randomCharacter(characterSet);
        }

        // Cryptographically secure shuffle.
        secureShuffle(password);

        return password;
    }


    // Generate an Argon2id password hash.
    static std::string hashPassword(
        const std::string& password
    ) {
        char hash[crypto_pwhash_STRBYTES];

        if (crypto_pwhash_str(
                hash,
                password.c_str(),
                password.length(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE
            ) != 0) {

            throw std::runtime_error(
                "Password hashing failed."
            );
        }

        return std::string(hash);
    }


    // Verify a password against an Argon2id hash.
    static bool verifyPassword(
        const std::string& password,
        const std::string& hash
    ) {
        return crypto_pwhash_str_verify(
            hash.c_str(),
            password.c_str(),
            password.length()
        ) == 0;
    }


private:

    // Securely select a character.
    static char randomCharacter(
        const std::string& characters
    ) {
        const std::size_t index =
            randombytes_uniform(
                static_cast<uint32_t>(characters.length())
            );

        return characters[index];
    }


    // Secure Fisher-Yates shuffle.
    static void secureShuffle(std::string& value) {

        for (std::size_t i = value.length() - 1; i > 0; --i) {

            const std::size_t j =
                randombytes_uniform(
                    static_cast<uint32_t>(i + 1)
                );

            std::swap(value[i], value[j]);
        }
    }
};


int main() {

    // Initialize libsodium.
    if (sodium_init() < 0) {
        std::cerr
            << "[ERROR] Unable to initialize libsodium.\n";
        return 1;
    }

    try {

        std::cout
            << "=====================================\n"
            << " Secure Password Generator\n"
            << "=====================================\n\n";

        std::size_t length;

        std::cout
            << "Password length [minimum 8]: ";

        std::cin >> length;

        // Generate password.
        std::string password =
            PasswordManager::generatePassword(
                length,
                true,   // Uppercase
                true,   // Lowercase
                true,   // Numbers
                true    // Symbols
            );

        std::cout << "\nGenerated password:\n";
        std::cout << password << "\n";

        // Hash generated password.
        std::string hash =
            PasswordManager::hashPassword(password);

        std::cout << "\nArgon2id password hash:\n";
        std::cout << hash << "\n";

        // Verify generated password.
        if (PasswordManager::verifyPassword(
                password,
                hash)) {

            std::cout
                << "\n[PASS] Password verification successful.\n";

        } else {

            std::cout
                << "\n[FAIL] Password verification failed.\n";
        }

    }
    catch (const std::exception& error) {

        std::cerr
            << "\n[ERROR] "
            << error.what()
            << "\n";

        return 1;
    }

    return 0;
}
```
