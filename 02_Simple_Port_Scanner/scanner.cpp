#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

// Function to check if a port is open
bool checkPort(const std::string& address, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set a timeout or keep it simple (blocking socket)
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address.c_str(), &target.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    // Attempt connection
    int result = connect(sock, (struct sockaddr*)&target, sizeof(target));
    
    close(sock);
    return (result == 0); // If result is 0, the port is open
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <IP Address> <Start Port> <End Port>\n";
        std::cout << "Example: " << argv[0] << " 127.0.0.1 1 1024\n";
        return 1;
    }

    std::string ip = argv[1];
    int startPort = std::stoi(argv[2]);
    int endPort = std::stoi(argv[3]);

    std::cout << "\nScanning " << ip << " from port " << startPort << " to " << endPort << "...\n";
    std::cout << "----------------------------------------\n";

    for (int port = startPort; port <= endPort; ++port) {
        if (checkPort(ip, port)) {
            std::cout << "[+] Port " << port << " is OPEN\n";
        }
    }

    std::cout << "----------------------------------------\n";
    std::cout << "Scan complete.\n";

    return 0;
}
