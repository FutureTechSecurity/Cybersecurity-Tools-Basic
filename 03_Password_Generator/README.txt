
Security architecture

                Password Generator
                         │
                         ▼
               libsodium CSPRNG
                         │
                         ▼
              Random Password
                         │
                         ▼
                     Argon2id
                         │
                         ▼
                  Password Hash
                         │
                         ▼
                  Secure Storage


=====================================
 Secure Password Generator
=====================================

Password length [minimum 8]: 24

Generated password:
qG7!xM2@vL9#pR4$kT8&nZ1?

Argon2id password hash:
$argon2id$v=19$m=65536,t=2,p=1$...

[PASS] Password verification successful.
