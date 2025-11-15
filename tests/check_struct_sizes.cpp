#include <iostream>
#include "messages/authentication_messages.h"

int main() {
    std::cout << "RegisterRequest: " << sizeof(RegisterRequest) << " bytes\n";
    std::cout << "RegisterResponse: " << sizeof(RegisterResponse) << " bytes\n";
    std::cout << "LoginRequest: " << sizeof(LoginRequest) << " bytes\n";
    std::cout << "LoginResponse: " << sizeof(LoginResponse) << " bytes\n";
    std::cout << "LogoutRequest: " << sizeof(LogoutRequest) << " bytes\n";
    std::cout << "LogoutResponse: " << sizeof(LogoutResponse) << " bytes\n";
    return 0;
}
