#include <iostream>
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <string>

uint8_t GF_Multiplication(uint8_t num1, uint8_t num2) {
    unsigned char result = 0;
    unsigned char overflow = 0;
    for (uint8_t x = 0; x < 8; x++) {
        if (num2 & 1) {  // if num2 least significant bit and this number = 1, returns 1
            result ^= num1;
        }
        overflow = num1 & 0x80; // if highest bit is 1 in both, set overflow to 1
        num1 <<= 1; // left shift a to increase its value to take into account that the degree of b increased on the next b, yet we cant account for it there because it had to be right shifted to be least significant bit
        
        if (overflow) {
            num1 ^= 0x1b; // because field property is that addition and subtraction are the same, so x^8 is equal to the rest of the polynomial, which can be XOR'ed to reduce the polynomial back to GF(2^8) with a unique value
        }

        num2 >>= 1; // shifted to the right for new lowest bit value
    
    }
    return result;
}

uint8_t sbox_calc(uint8_t input) {
    uint8_t inverse = 0;
    if (input != 0) {
        // find the multiplicative inverse, when they multiply to equal 1, its the inverse
        for (uint16_t i = 1; i < 256; i++) {
            if (GF_Multiplication(input, i) == 1) {
                inverse = i;
                break;
            }
        }
    }
    
    // apply affine transformation by rotating bits and XORing with constant 0x63
    uint8_t result = 0;
    uint8_t c = 0x63;
    
    for (int i = 0; i < 8; i++) {
        uint8_t bit = 0;
        bit ^= (inverse >> i) & 1;
        bit ^= (inverse >> ((i + 4) % 8)) & 1;
        bit ^= (inverse >> ((i + 5) % 8)) & 1;
        bit ^= (inverse >> ((i + 6) % 8)) & 1;
        bit ^= (inverse >> ((i + 7) % 8)) & 1;
        bit ^= (c >> i) & 1;
        
        result |= (bit << i);
    }
    
    return result;
}

void SubBytes(uint8_t state[4][4]) {
    // apply S-box substitution to every byte in the 4x4 state matrix
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = sbox_calc(state[i][j]);
        }
    }
}

void ShiftRows(uint8_t state[4][4]) {
    uint8_t temp;
    
    // row 0: no shift
    
    // row 1: shift left by 1
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;
    
    // row 2: shift left by 2
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;
    
    // row 3: shift left by 3 (or right by 1)
    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

void MixColumns(uint8_t state[4][4]) {
    // multiply each column by the fixed MixColumns matrix in GF(2^8)
    for (int col = 0; col < 4; col++) {
        uint8_t s0 = state[0][col];
        uint8_t s1 = state[1][col];
        uint8_t s2 = state[2][col];
        uint8_t s3 = state[3][col];
        
        // each row of result is a linear combination using GF multiplication
        state[0][col] = GF_Multiplication(0x02, s0) ^ GF_Multiplication(0x03, s1) ^ s2 ^ s3;
        state[1][col] = s0 ^ GF_Multiplication(0x02, s1) ^ GF_Multiplication(0x03, s2) ^ s3;
        state[2][col] = s0 ^ s1 ^ GF_Multiplication(0x02, s2) ^ GF_Multiplication(0x03, s3);
        state[3][col] = GF_Multiplication(0x03, s0) ^ s1 ^ s2 ^ GF_Multiplication(0x02, s3);
    }
}

void AddRoundKey(uint8_t state[4][4], uint8_t roundKey[4][4]) {
    // XOR each byte of state with corresponding byte of round key
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] ^= roundKey[i][j];
        }
    }
}

uint8_t Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36}; // round constants for key expansion

void KeyExpansion(uint8_t key[16], uint8_t roundKeys[11][4][4]) {
    // copy original key as first round key (round 0)
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            roundKeys[0][i][j] = key[i + 4*j];
        }
    }
    
    // generate 10 more round keys from the original
    uint8_t temp[4];
    for (int round = 1; round <= 10; round++) {
        // take last column of previous round key
        for (int i = 0; i < 4; i++) {
            temp[i] = roundKeys[round-1][i][3];
        }
        
        // rotate bytes up by 1 (RotWord)
        uint8_t t = temp[0];
        temp[0] = temp[1];
        temp[1] = temp[2];
        temp[2] = temp[3];
        temp[3] = t;
        
        // apply S-box to each byte (SubWord)
        for (int i = 0; i < 4; i++) {
            temp[i] = sbox_calc(temp[i]);
        }
        
        // XOR first byte with round constant
        temp[0] ^= Rcon[round-1];
        
        // generate new round key by XORing previous round key with temp
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                if (col == 0) {
                    roundKeys[round][row][col] = roundKeys[round-1][row][col] ^ temp[row];
                } else {
                    roundKeys[round][row][col] = roundKeys[round-1][row][col] ^ roundKeys[round][row][col-1];
                }
            }
        }
    }
}

void AES_Encrypt(uint8_t state[4][4], uint8_t key[16]) {
    // generate all 11 round keys from the original key
    uint8_t roundKeys[11][4][4];
    KeyExpansion(key, roundKeys);
    
    std::cout << "=== AES-128 Encryption ===" << std::endl << std::endl;
    
    // round 0: just add round key
    std::cout << "Initial AddRoundKey (Round 0):" << std::endl;
    AddRoundKey(state, roundKeys[0]);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(state[i][j]) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // rounds 1-9: SubBytes -> ShiftRows -> MixColumns -> AddRoundKey
    for (int round = 1; round <= 9; round++) {
        std::cout << "Round " << std::dec << round << ":" << std::endl;
        
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, roundKeys[round]);
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(state[i][j]) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    // round 10 (final): SubBytes -> ShiftRows -> AddRoundKey (NO MixColumns!)
    std::cout << "Round 10 (Final):" << std::endl;
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, roundKeys[10]);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(state[i][j]) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::string plaintext_input;
    std::string key_input;
    
    // get plaintext from user
    std::cout << "Enter plaintext (exactly 16 characters): ";
    std::getline(std::cin, plaintext_input);
    
    // pad or truncate to 16 bytes
    if (plaintext_input.length() < 16) {
        plaintext_input.resize(16, ' '); // pad with spaces
    } else if (plaintext_input.length() > 16) {
        plaintext_input = plaintext_input.substr(0, 16); // truncate to 16
    }
    
    // get key from user
    std::cout << "Enter key (exactly 16 characters): ";
    std::getline(std::cin, key_input);
    
    // pad or truncate to 16 bytes
    if (key_input.length() < 16) {
        key_input.resize(16, ' '); // pad with spaces
    } else if (key_input.length() > 16) {
        key_input = key_input.substr(0, 16); // truncate to 16
    }
    
    // convert plaintext string to 4x4 state matrix
    uint8_t plaintext[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            plaintext[i][j] = plaintext_input[i + 4*j];
        }
    }
    
    // convert key string to byte array
    uint8_t key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = key_input[i];
    }
    
    // print original plaintext
    std::cout << "\nOriginal plaintext:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(plaintext[i][j]) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    // run AES encryption
    AES_Encrypt(plaintext, key);
    
    // print final ciphertext
    std::cout << "Final ciphertext:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(plaintext[i][j]) << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}