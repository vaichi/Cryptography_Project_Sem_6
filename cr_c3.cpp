#include <iostream>
#include <fstream>
#include <cstring>
#include <openssl/evp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

using namespace std;

// Function to encrypt image file using OpenSSL command
bool encryptImage(const char* inputFileName, const char* outputFileName) {
    string command = "openssl enc -aes-256-cbc -pass pass:kekayan -p -in " + string(inputFileName) + " -out " + string(outputFileName) + " -iter 10000";
    return system(command.c_str()) == 0;
}

int main() {
    // Get input image file name from user
    std::string inputFileName;
    std::cout << "Enter the input image file name: ";
    std::cin >> inputFileName;

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error creating socket." << endl;
        return 1;
    }

    // Specify the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080); // Assuming the server is listening on port 8080
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Assuming the server is running on localhost

    // Connect to the server
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        cerr << "Error connecting to server." << endl;
        close(clientSocket);
        return 1;
    }

    // Encrypt image file
    if (!encryptImage(inputFileName.c_str(), "filed.enc")) {
        cerr << "Error encrypting image." << endl;
        close(clientSocket);
        return 1;
    }

    // Send encrypted image file size
    ifstream imageFile("filed.enc", ios::binary | ios::ate);
    if (!imageFile) {
        cerr << "Error opening image file." << endl;
        close(clientSocket);
        return 1;
    }
    int imageSize = imageFile.tellg();
    imageFile.seekg(0, ios::beg);
    if (send(clientSocket, &imageSize, sizeof(imageSize), 0) != sizeof(imageSize)) {
        cerr << "Error sending image size." << endl;
        imageFile.close();
        close(clientSocket);
        return 1;
    }

    // Send encrypted image file data
    char buffer[1024];
    while (imageSize > 0) {
        int bytesToRead = min(imageSize, static_cast<int>(sizeof(buffer)));
        imageFile.read(buffer, bytesToRead);
        int bytesSent = send(clientSocket, buffer, bytesToRead, 0);
        if (bytesSent <= 0) {
            cerr << "Error sending image file data." << endl;
            imageFile.close();
            close(clientSocket);
            return 1;
        }
        imageSize -= bytesSent;
    }

    // Close image file
    imageFile.close();

    // Receive authentication result from server
    char authResult[256];
    int bytesReceived = recv(clientSocket, authResult, sizeof(authResult), 0);
    if (bytesReceived <= 0) {
        cerr << "Error receiving authentication result." << endl;
    } else {
        authResult[bytesReceived] = '\0'; // Null-terminate the received data to use it as a C-string
        cout << "Authentication result from server: " << authResult << endl;
    }

    // Close socket
    close(clientSocket);

    return 0;
}

