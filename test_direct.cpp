#include "protocol.h"
#include <iostream>

int main() {
    std::cout << "Direct include test:" << std::endl;
    std::cout << "VALIDATE_SESSION = " << (int)VALIDATE_SESSION << std::endl;
    std::cout << "AUTH_RESPONSE = " << (int)AUTH_RESPONSE << std::endl;
    return 0;
}
