#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDataStream>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <string>

// ---------------- HARDWARE / STRUCT ALIGNMENT ----------------
#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtLen;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataLen;
};
#pragma pack(pop)

// ---------------- CRYPTOGRAPHY UTILS (AES-128-CBC) ----------------

std::string generate_key() {
    unsigned char key[16];
    RAND_bytes(key, sizeof(key));
    QByteArray ba(reinterpret_cast<char*>(key), sizeof(key));
    return ba.toBase64().toStdString();
}

std::string encrypt_message(const std::string& plainText, const std::string& keyBase64) {
    QByteArray keyData = QByteArray::fromBase64(QByteArray::fromStdString(keyBase64));
    if (keyData.size() != 16) throw std::runtime_error("Key must be 16 bytes when base64 decoded.");

    unsigned char iv[16] = {0}; // Simple fixed IV for presentation footprint simplification

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, reinterpret_cast<const unsigned char*>(keyData.constData()), iv);

    std::vector<unsigned char> cipherText(plainText.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, cipherText.data(), &len, reinterpret_cast<const unsigned char*>(plainText.data()), plainText.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, cipherText.data() + len, &len);
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    QByteArray encBa(reinterpret_cast<char*>(cipherText.data()), ciphertext_len);
    return encBa.toBase64().toStdString();
}

std::string decrypt_message(const std::string& cipherTextBase64, const std::string& keyBase64) {
    QByteArray keyData = QByteArray::fromBase64(QByteArray::fromStdString(keyBase64));
    QByteArray cipherData = QByteArray::fromBase64(QByteArray::fromStdString(cipherTextBase64));
    if (keyData.size() != 16) throw std::runtime_error("Invalid Key length.");

    unsigned char iv[16] = {0};

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, reinterpret_cast<const unsigned char*>(keyData.constData()), iv);

    std::vector<unsigned char> plainText(cipherData.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0, plaintext_len = 0;

    EVP_DecryptUpdate(ctx, plainText.data(), &len, reinterpret_cast<const unsigned char*>(cipherData.constData()), cipherData.size());
    plaintext_len = len;

    int ret = EVP_DecryptFinal_ex(ctx, plainText.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) throw std::runtime_error("Decryption failed. Bad Key or Corrupted Payload.");
    plaintext_len += len;

    return std::string(reinterpret_cast<char*>(plainText.data()), plaintext_len);
}

// ---------------- STEGANOGRAPHY CORE ----------------

void encode_audio(const QString& inPath, const QString& outPath, std::string secretMsg) {
    QFile inFile(inPath);
    if (!inFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Can't open source audio.");

    QByteArray fileData = inFile.readAll();
    inFile.close();

    if (fileData.size() < sizeof(WAVHeader)) throw std::runtime_error("Invalid WAV file structure.");

    secretMsg += "###"; // Terminator sequence
    std::string binaryString = "";
    for (char c : secretMsg) {
        for (int i = 7; i >= 0; --i) {
            binaryString += ((c >> i) & 1) ? '1' : '0';
        }
    }

    size_t headerSize = sizeof(WAVHeader);
    if (headerSize + binaryString.length() > (size_t)fileData.size()) {
        throw std::runtime_error("Audio data payload surface too small for this message string.");
    }

    for (size_t i = 0; i < binaryString.length(); ++i) {
        char bit = binaryString[i] - '0';
        fileData[headerSize + i] = (fileData[headerSize + i] & 254) | bit;
    }

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly)) throw std::runtime_error("Can't create output audio file.");
    outFile.write(fileData);
    outFile.close();
}

std::string decode_audio(const QString& inPath) {
    QFile inFile(inPath);
    if (!inFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Can't open target audio file.");

    QByteArray fileData = inFile.readAll();
    inFile.close();

    size_t headerSize = sizeof(WAVHeader);
    std::string extractedBits = "";
    
    for (size_t i = headerSize; i < (size_t)fileData.size(); ++i) {
        extractedBits += std::to_string(fileData[i] & 1);
    }

    std::string decodedMsg = "";
    for (size_t i = 0; i < extractedBits.length(); i += 8) {
        if (i + 8 > extractedBits.length()) break;
        char byte = 0;
        for (int bit = 0; bit < 8; ++bit) {
            byte = (byte << 1) | (extractedBits[i + bit] - '0');
        }
        decodedMsg += byte;

        if (decodedMsg.length() >= 3 && decodedMsg.substr(decodedMsg.length() - 3) == "###") {
            return decodedMsg.substr(0, decodedMsg.length() - 3);
        }
    }
    return "";
}

// ---------------- GUI LAYOUT CONTROLLER ----------------

class StegoWindow : public QWidget {
    Q_OBJECT
public:
    StegoWindow() {
        setWindowTitle("Secure C++ Audio Steganography Studio");
        resize(600, 450);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        QTabWidget* tabs = new QTabWidget(this);

        // Tab Setup
        QWidget* keyTab = new QWidget();
        QWidget* encodeTab = new QWidget();
        QWidget* decodeTab = new QWidget();

        tabs->addTab(keyTab, "1. Key Manager");
        tabs->addTab(encodeTab, "2. Encrypt & Hide");
        tabs->addTab(decodeTab, "3. Extract & Decrypt");
        mainLayout->addWidget(tabs);

        buildKeyTab(keyTab);
        buildEncodeTab(encodeTab);
        buildDecodeTab(decodeTab);
    }

private:
    QLineEdit *keyGenEdit, *encFileEdit, *encKeyEdit, *decFileEdit, *decKeyEdit;
    QTextEdit *encMsgEdit, *decMsgEdit;

    void buildKeyTab(QWidget* tab) {
        QVBoxLayout* layout = new QVBoxLayout(tab);
        layout->addWidget(new QLabel("<b>AES-128 Custom Cryptography Channel Key</b>"));
        keyGenEdit = new QLineEdit();
        keyGenEdit->setReadOnly(true);
        layout->addWidget(keyGenEdit);

        QPushButton* genBtn = new QPushButton("Generate Symmetric Key");
        layout->addWidget(genBtn);
        connect(genBtn, &QPushButton::clicked, [this]() {
            keyGenEdit->setText(QString::fromStdString(generate_key()));
        });
    }

    void buildEncodeTab(QWidget* tab) {
        QVBoxLayout* layout = new QVBoxLayout(tab);
        
        QHBoxLayout* fLayout = new QHBoxLayout();
        encFileEdit = new QLineEdit();
        QPushButton* browseBtn = new QPushButton("Browse WAV");
        fLayout->addWidget(encFileEdit);
        fLayout->addWidget(browseBtn);
        layout->addWidget(new QLabel("Target Input Carrier File:"));
        layout->addLayout(fLayout);

        encKeyEdit = new QLineEdit();
        encKeyEdit->setEchoMode(QLineEdit::Password);
        layout->addWidget(new QLabel("Encryption Cryptographic Key (Base64):"));
        layout->addWidget(encKeyEdit);

        encMsgEdit = new QTextEdit();
        layout->addWidget(new QLabel("Secret Structural Message Payload:"));
        layout->addWidget(encMsgEdit);

        QPushButton* runBtn = new QPushButton("Run Pipeline Matrix (Encrypt & Inject)");
        layout->addWidget(runBtn);

        connect(browseBtn, &QPushButton::clicked, [this]() {
            QString path = QFileDialog::getOpenFileName(this, "Select Carrier Audio", "", "WAV Files (*.wav)");
            if(!path.isEmpty()) encFileEdit->setText(path);
        });

        connect(runBtn, &QPushButton::clicked, [this]() {
            try {
                std::string ciphertext = encrypt_message(encMsgEdit->toPlainText().toStdString(), encKeyEdit->text().toStdString());
                QString outPath = QFileDialog::getSaveFileName(this, "Save Stego Output", "", "WAV Files (*.wav)");
                if(outPath.isEmpty()) return;
                
                encode_audio(encFileEdit->text(), outPath, ciphertext);
                QMessageBox::information(this, "Success", "Payload secured and integrated inside the target WAV envelope safely.");
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Pipeline Failure", e.what());
            }
        });
    }

    void buildDecodeTab(QWidget* tab) {
        QVBoxLayout* layout = new QVBoxLayout(tab);

        QHBoxLayout* fLayout = new QHBoxLayout();
        decFileEdit = new QLineEdit();
        QPushButton* browseBtn = new QPushButton("Browse Output");
        fLayout->addWidget(decFileEdit);
        fLayout->addWidget(browseBtn);
        layout->addWidget(new QLabel("Encrypted Stego Audio Object:"));
        layout->addLayout(fLayout);

        decKeyEdit = new QLineEdit();
        decKeyEdit->setEchoMode(QLineEdit::Password);
        layout->addWidget(new QLabel("Decryption Key (Base64):"));
        layout->addWidget(decKeyEdit);

        decMsgEdit = new QTextEdit();
        decMsgEdit->setReadOnly(true);
        layout->addWidget(new QLabel("Decrypted Clean System Strings:"));
        layout->addWidget(decMsgEdit);

        QPushButton* runBtn = new QPushButton("Extract & Parse Structural Secret");
        layout->addWidget(runBtn);

        connect(browseBtn, &QPushButton::clicked, [this]() {
            QString path = QFileDialog::getOpenFileName(this, "Select Stego Audio", "", "WAV Files (*.wav)");
            if(!path.isEmpty()) decFileEdit->setText(path);
        });

        connect(runBtn, &QPushButton::clicked, [this]() {
            try {
                std::string rawCipher = decode_audio(decFileEdit->text());
                std::string plainText = decrypt_message(rawCipher, decKeyEdit->text().toStdString());
                decMsgEdit->setText(QString::fromStdString(plainText));
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Extraction Core Error", e.what());
            }
        });
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    StegoWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
