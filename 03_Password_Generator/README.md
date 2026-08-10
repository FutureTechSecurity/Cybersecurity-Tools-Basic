if Running in the Compiler

g++ -std=c++17 password_manager.cpp -lsodium -o password_manager

Install libsodium-dev for Ubuntu
sudo apt install libsodium-dev

Working
                User Password
                     │
                     ▼
                 Argon2id
                     │
              ┌──────┴──────┐
              │             │
            Salt          Hash
              │             │
              └──────┬──────┘
                     ▼
             Stored password
                  hash

During Login

Entered Password
       │
       ▼
    Argon2id
       │
       ▼
Compare with stored hash
       │
   ┌───┴───┐
   ▼       ▼
 MATCH   NO MATCH
   │       │
 ACCESS   DENY
