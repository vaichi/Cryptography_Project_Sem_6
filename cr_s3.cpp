#include <iostream>
#include <fstream>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <sstream>
#include <iomanip>

using namespace std;

string sname;
string sroll;

// Function to compute the SHA-256 hash of a file
std::string computeSHA256Hash(const char* fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << fileName << std::endl;
        return "";
    }

    // Compute SHA-256 hash
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    // Convert hash to hexadecimal string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// Declare decryptImage function
bool decryptImage(const char* inputFileName, const char* outputFileName);

// Function to compare decrypted image with stored images
bool compareImages(const char* decryptedImageFile, const std::map<std::string, std::pair<std::string, std::string>>& imageStudentMap) {
    // Read the decrypted image
    std::ifstream decryptedImage(decryptedImageFile, std::ios::binary);
    if (!decryptedImage) {
        std::cerr << "Error opening decrypted image file." << std::endl;
        return false;
    }

    // Compute hash of the decrypted image
    std::string decryptedImageHash = computeSHA256Hash(decryptedImageFile);

    // Compare hash of decrypted image with hashes of stored images
    for (const auto& entry : imageStudentMap) {
        const std::string& storedImageFileName = entry.first;
        std::string storedImageHash = computeSHA256Hash(storedImageFileName.c_str());
        if (decryptedImageHash == storedImageHash) {
            std::cout << "Image " << decryptedImageFile << " matches with " << storedImageFileName << std::endl;
            
            sname = entry.second.first;
            sroll = entry.second.second;
            return true; // Authentication successful
        }
    }

    std::cout << "Image " << decryptedImageFile << " does not match with any stored image." << std::endl;
    return false; // Authentication failed
}

int main() {
    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // Bind socket to port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening on socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    // Accept incoming connection
    sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
    if (clientSocket == -1) {
        std::cerr << "Error accepting connection." << std::endl;
        close(serverSocket);
        return 1;
    }

    // Receive encrypted image file size
    int imageSize;
    int bytesRead = recv(clientSocket, &imageSize, sizeof(imageSize), 0);
    if (bytesRead != sizeof(imageSize)) {
        std::cerr << "Error receiving image size." << std::endl;
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    // Receive encrypted image file data
    std::ofstream encryptedImageFile("filed.enc", std::ios::binary);
    if (!encryptedImageFile) {
        std::cerr << "Error opening received encrypted image file." << std::endl;
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    char buffer[1024];
    int bytesReceived = 0;
    while (bytesReceived < imageSize) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::cerr << "Error receiving encrypted image file data." << std::endl;
            encryptedImageFile.close();
            close(clientSocket);
            close(serverSocket);
            return 1;
        }
        encryptedImageFile.write(buffer, bytesRead);
        bytesReceived += bytesRead;
    }

    // Close encrypted image file
    encryptedImageFile.close();

    // Decrypt image
    if (!decryptImage("filed.enc", "decrypted.jpg")) {
        std::cerr << "Error decrypting image." << std::endl;
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    // Map image file names to student names and roll numbers
    std::map<std::string, std::pair<std::string, std::string>> imageStudentMap = {
        {"server_eye0.jpg", {"Tanu Priya", "21CSB0A58"}},
        {"server_eye1.jpg", {"Vaishnavi Pradeep", "21CSB0F12"}},
        {"server_eye2.jpg", {"Shreya Kyasaram", "21CSB0F02"}}
    };

    // Perform image comparison
    if (compareImages("decrypted.jpg", imageStudentMap)) {
        // Authentication successful
        std::string authenticationResult = "Authentication successful\nStudent Name: " + sname + "\nRoll Number: " + sroll;
        send(clientSocket, authenticationResult.c_str(), authenticationResult.size(), 0);
    } else {
        // Authentication failed
        const char* authenticationResult = "Authentication failed";
        send(clientSocket, authenticationResult, strlen(authenticationResult), 0);
    }

    // Close client socket
    close(clientSocket);

    // Close server socket
    close(serverSocket);

    return 0;
}

// Decrypt Image Function
bool decryptImage(const char* inputFileName, const char* outputFileName) {
    std::string command = "openssl enc -d -aes-256-cbc -pass pass:kekayan -p -in " + std::string(inputFileName) + " -out " + std::string(outputFileName) + " -iter 10000";
    return system(command.c_str()) == 0;
}

