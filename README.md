# AES-Implementation
I made the Advanced Encryption Standard in C++, while the code itself is mine, the ideas of it are obviously based off the pre existing standard, the **main** reason I did this was to learn the math behind the cryptography rather than the code, how it gets actually applied in computer science, as well as working with matrices.
One way I did this actually hurts the performance of the program a lot, I manually calculated the value of the S-Box by knowing that it finds the multiplicative inverse of a matrix, which would always be an existing unique value due to finite field theory, being just one of its uses here

**I do not reccomend using this for personal use** it is likely susceptible to other sort of attacks that I haven't thought about, it simply encrypts a byte of data
