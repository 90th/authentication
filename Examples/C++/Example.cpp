#define CURL_STATICLIB

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <fstream>
#include <windows.h>
#include <sddl.h>
#include "skCrypter.h"
#include <winternl.h>

std::string encodeHwid(const std::wstring& hwid) {
    std::string encodedHwid = "";
    for (char c : hwid) {
        if (std::isalnum(c) || c == '-' || c == '_') {
            encodedHwid += c;
        } else {
            encodedHwid += "%" + std::to_string(static_cast<unsigned char>(c));
        }
    }
    return encodedHwid;
}


std::string WideStringToUtf8(const std::wstring& wstr)
{
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);

    std::string result;
    result.resize(utf8Len);

    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], utf8Len, NULL, NULL);

    return result;
}

std::wstring GetSid()
{
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        DWORD dwBufferSize = 0;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            TOKEN_USER* pTokenUser = reinterpret_cast<TOKEN_USER*>(new char[dwBufferSize]);
            if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize))
            {
                LPWSTR sidString = NULL;
                if (ConvertSidToStringSidW(pTokenUser->User.Sid, &sidString))
                {
                    std::wstring sid = sidString;
                    LocalFree(sidString);
                    delete[] reinterpret_cast<char*>(pTokenUser);
                    return sid;
                }
            }
            delete[] reinterpret_cast<char*>(pTokenUser);
        }
        CloseHandle(hToken);
    }
    return L"";
}

// Callback function to write the HTTP response
size_t writeCallback(void* buffer, size_t size, size_t nmemb, std::string* response)
{
    response->append(static_cast<char*>(buffer), size * nmemb);
    return size * nmemb;
}

std::string sendHttpRequest(const std::string& url)
{
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // Set the callback function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << skCrypt("Failed to send HTTP request: ") << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    return response;
}

std::string GetCurrentModulePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::string(buffer);
}

std::string CalculateFileChecksum(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error(skCrypt("Failed to open file"));
    }

    CryptoPP::SHA256 hash;
    byte digest[CryptoPP::SHA256::DIGESTSIZE];

    while (file.good()) {
        char buffer[4096];
        file.read(buffer, sizeof(buffer));
        hash.Update(reinterpret_cast<const byte*>(buffer), file.gcount());
    }

    hash.Final(digest);

    std::string checksum;
    CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(checksum));
    encoder.Put(digest, CryptoPP::SHA256::DIGESTSIZE);
    encoder.MessageEnd();

    return checksum;
}

std::string GenerateApiUrl(const std::string& licenseKey, const std::wstring& sid, const std::string& apiKey, const std::string& checksum) {
    std::string apiUrl = skCrypt("http://1x6.mygamesonline.org/v2/api.php").decrypt();
    apiUrl += skCrypt("?licenseKey=").decrypt() + licenseKey;
    apiUrl += skCrypt("&hwid=").decrypt() + encodeHwid(sid);
    apiUrl += skCrypt("&apiKey=").decrypt() + apiKey;
    apiUrl += skCrypt("&programHash=").decrypt() + checksum;
    return apiUrl;
}

bool SendHttpRequestAndCheckResponse(const std::string& apiUrl) {
    std::string response = sendHttpRequest(apiUrl);
    return (response == "OK");
}

bool Authenticate() {
    std::string filePath = GetCurrentModulePath();
    std::string checksum = CalculateFileChecksum(filePath);
    std::wstring hwidW = GetSid();
    std::string apiKey = skCrypt("YOUR_API_KEY_1").decrypt();
    std::string licenseKey;
    std::string hwid = WideStringToUtf8(hwidW);

    std::cout << skCrypt("Enter license key: ");
    std::cin >> licenseKey;

    std::wstring sid = GetSid();
    if (!sid.empty() && sid.back() == L'\0') {
        sid.pop_back();
    }

    std::string apiUrl = GenerateApiUrl(licenseKey, sid, apiKey, checksum);
    return SendHttpRequestAndCheckResponse(apiUrl);
}


int main() {
    if (Authenticate()) {
        std::cout << skCrypt("Authentication successful. Proceeding with the program.") << std::endl;
		Beep(1000, 500);
        std::string dummy;
        std::getline(std::cin, dummy);
    } else {
        std::cout << skCrypt("Authentication failed. Exiting the program.") << std::endl;
        Beep(500, 500);
        std::string dummy;
        std::getline(std::cin, dummy);
    }
    return 0;
}