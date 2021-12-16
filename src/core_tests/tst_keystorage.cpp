#ifndef TST_KEYSTORAGE_HPP
#define TST_KEYSTORAGE_HPP

#include <Data/WalletAssetsModel.hpp>
#include <EthCore/Encodings.hpp>
#include <EthCore/Types.hpp>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <bip39.h>
#include <boost/progress.hpp>
#include <chainparams.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <key_io.h>
#include <outputtype.h>
#include <random.h>
#include <utilstrencodings.h>
#include <wallet.h>
#include <walletdb.h>

using boost::progress_timer;
using namespace testing;

class BitcoinWalletTests : public ::testing::Test {
protected:
    bitcoin::CHDChain hdChain;
    QDir _dataPath;
    WalletAssetsModel _assetsModel;

    BitcoinWalletTests()
        : _dataPath("/tmp/walletTests")
        , _assetsModel("assets_conf.json")
    {

        bitcoin::RandomInit();
        if (!_dataPath.exists())
            _dataPath.mkpath(".");

        _secp256k1VerifyHandle.reset(new bitcoin::ECCVerifyHandle);
    }

    QString createTempPath(QString name) const
    {
        QDir dir(_dataPath.absoluteFilePath(
            QString("%1_%2").arg(name).arg(QDateTime::currentSecsSinceEpoch())));

        if (!dir.exists()) {
            dir.mkdir(".");
        }

        return dir.absolutePath();
    }

    virtual void SetUp()
    {
        bitcoin::ECC_Start();

        // master key for this seed -
        // xprv9s21ZrQH143K25QhxbucbDDuQ4naNntJRi4KUfWT7xo4EKsHt2QJDu7KXp1A3u7Bi1j8ph3EGsZ9Xvz9dGuVrtHHs7pXeTzjuxBrCmmhgC6
        // for bitcoin
        //        auto seed =
        //        bitcoin::ParseHex("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be");
        //        hdChain.vchSeed = SecureVector(seed.begin(), seed.end());
        //        storage.setHDChain(hdChain);
    }

    virtual void TearDown() { bitcoin::ECC_Stop(); }

    std::unique_ptr<bitcoin::ECCVerifyHandle> _secp256k1VerifyHandle;
};

TEST_F(BitcoinWalletTests, CreateWalletFromSeed)
{
    bitcoin::CWallet wallet(createTempPath("createWalletFromSeed").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    wallet.GenerateNewHDChain("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45"
                              "d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be");
}

TEST_F(BitcoinWalletTests, CreateWalletFromMnemonic)
{
    bitcoin::CWallet wallet(createTempPath("createWalletWithMnemonic").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    wallet.GenerateNewHDChain("",
        "unlock gaze river calm pink tree marriage lyrics purpose bag develop food devote thing "
        "convince learn boat blue romance sail catch odor rain glory",
        "");
    bitcoin::CHDChain hdChain;
    wallet.GetHDChain(hdChain);
    ASSERT_EQ("fdd7161e7938f7926751bbf770c7cd0ba049f2bc521ef30b195819876b8ace4bed8f0d502b3ded9598e3"
              "f710de0ed3166d4e737f10c7c71b17281be5e2ab1b76",
        bitcoin::HexStr(hdChain.GetSeed()));
}

TEST_F(BitcoinWalletTests, CreateWalletWithMnemonic)
{
    auto mnemonic = bitcoin::CMnemonic::Generate(256);
    bitcoin::CWallet wallet(createTempPath("createWalletWithMnemonic").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    wallet.GenerateNewHDChain(
        std::string(), std::string{ mnemonic.begin(), mnemonic.end() }, std::string());

    bitcoin::CHDChain hdChain;
    wallet.GetHDChain(hdChain);
    SecureString mnemonicResult, paraphraseResult;
    hdChain.GetMnemonic(mnemonicResult, paraphraseResult);
    ASSERT_EQ(mnemonic, mnemonicResult);
}

TEST_F(BitcoinWalletTests, CreateKeyBenchmark)
{
    bitcoin::CWallet wallet(createTempPath("createKeyBenchmark").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    wallet.GenerateNewHDChain("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45"
                              "d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be");
    bitcoin::CHDChain hdChain;
    wallet.GetHDChain(hdChain);
    auto scope = hdChain.GetChainScope(44);
    scope.AddAccount(0);
    hdChain.SetChainScope(44, scope);
    bitcoin::WalletBatch batch(wallet.GetDBHandle());
    wallet.SetHDChain(&batch, hdChain, false);

    auto testKeyGeneration = [&wallet, &batch](int numberOfKeys) {
        progress_timer timer;
        for (size_t i = 0; i < numberOfKeys; ++i) {
            wallet.GenerateNewKey(batch, 44, 0, 0, false);
        }
    };

    for (auto&& testSet : { 10, 100, 1000 }) {
        std::cout << "Benchmarking test with params: " << testSet << std::endl;
        testKeyGeneration(testSet);
        std::cout << std::endl;
    }
}

TEST_F(BitcoinWalletTests, AuxChainNextKey)
{
    bitcoin::CWallet wallet(createTempPath("auxChainNextKey").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    auto genMnemonic = bitcoin::CMnemonic::Generate(256);
    wallet.GenerateNewHDChain(std::string(), std::string{ genMnemonic.begin(), genMnemonic.end() });
    const AssetID assetID = 384;
    const int accountIndex = 5;
    for (auto i = 0; i < 50; ++i) {
        auto pubKey = wallet.GetNewAuxKey(assetID, accountIndex);
        bitcoin::CHDPubKey hdPubKey;
        wallet.GetHDPubKey(assetID, pubKey.GetID(), hdPubKey);
        ASSERT_EQ(hdPubKey.nAccountIndex, accountIndex);
        ASSERT_EQ(hdPubKey.extPubKey.nChild, i);
    }
}

TEST_F(BitcoinWalletTests, EthAddressGeneration)
{
    auto vchSeed = bitcoin::ParseHex(
        "ad20c59231d1e1a6181d2b7ea6e970a07aada9810a8595c0cd974b4ac105bad2231ebdb47"
        "8b6bf29809eb2fc5ff4fe780caac9baebf5e00954051d5738a80606");
    bitcoin::CHDChain hdChain;
    hdChain.SetSeed(SecureVector{ vchSeed.begin(), vchSeed.end() }, true);
    bitcoin::CExtKey extKey;
    const auto ethID = 60;
    hdChain.DeriveChildExtKey(44, ethID, 0, 0, 0, extKey);
    auto neuter = extKey.Neuter();
    auto pubkey = neuter.pubkey;

    ASSERT_EQ("0xA4A33005b69b055Da57ec494026f2f9f389aa9Ea", eth::EncodeDestination(pubkey));
}

TEST_F(BitcoinWalletTests, WalletEncryption)
{
    bitcoin::CWallet wallet(createTempPath("walletEncryption").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    auto genMnemonic = bitcoin::CMnemonic::Generate(256);
    wallet.GenerateNewHDChain(std::string(), std::string{ genMnemonic.begin(), genMnemonic.end() });
    bitcoin::CHDChain hdChainCurrent;
    ASSERT_TRUE(wallet.GetDecryptedHDChain(hdChainCurrent));
    SecureString pwd{ "pwd" };
    ASSERT_TRUE(wallet.EncryptWallet(pwd));
    ASSERT_FALSE(wallet.GetDecryptedHDChain(hdChainCurrent));

    ASSERT_TRUE(wallet.GetHDChain(hdChainCurrent));
    SecureString encryptedMnemonic, encryptedPassphrase;
    ASSERT_TRUE(hdChainCurrent.GetMnemonic(encryptedMnemonic, encryptedPassphrase));
    ASSERT_NE(genMnemonic, encryptedMnemonic);

    ASSERT_TRUE(wallet.Unlock(pwd));
    bitcoin::CHDChain hdChainDecrypted;
    ASSERT_TRUE(wallet.GetDecryptedHDChain(hdChainDecrypted));
    ASSERT_TRUE(hdChainDecrypted.GetMnemonic(encryptedMnemonic, encryptedPassphrase));
    ASSERT_EQ(genMnemonic, encryptedMnemonic);
}

TEST_F(BitcoinWalletTests, EncryptedWalletKeyGeneration)
{
    bitcoin::CWallet wallet(createTempPath("walletEncryptionGenerateKey").toStdString());
    bool firstRun = false;
    wallet.LoadWallet(firstRun);
    auto genMnemonic = bitcoin::CMnemonic::Generate(256);
    wallet.GenerateNewHDChain(std::string(), std::string{ genMnemonic.begin(), genMnemonic.end() });
    bitcoin::CHDChain hdChainCurrent;
    SecureString pwd{ "pwd" };

    wallet.GetHDChain(hdChainCurrent);
    auto scope = hdChainCurrent.GetChainScope(44);
    scope.AddAccount(0);
    hdChainCurrent.SetChainScope(44, scope);
    ASSERT_TRUE(wallet.SetHDChain(nullptr, hdChainCurrent, true));

    ASSERT_TRUE(wallet.EncryptWallet(pwd));

    bitcoin::WalletBatch batch(wallet.GetDBHandle());
    EXPECT_ANY_THROW(wallet.GenerateNewKey(batch, 44, 0, 0, false));
    ASSERT_TRUE(wallet.Unlock(pwd));
    EXPECT_NO_THROW(wallet.GenerateNewKey(batch, 44, 0, 0, false));
}

TEST_F(BitcoinWalletTests, AuxChainNextKeyAfterReload)
{
    const auto dbPath = createTempPath("auxChainNextKeyAfterReload").toStdString();
    const auto numberOfKeys = 50;
    const AssetID assetID = 384;
    const int accountIndex = 5;
    bool firstRun = false;
    {
        bitcoin::CWallet wallet(dbPath);
        wallet.LoadWallet(firstRun);
        auto genMnemonic = bitcoin::CMnemonic::Generate(256);
        wallet.GenerateNewHDChain(
            std::string(), std::string{ genMnemonic.begin(), genMnemonic.end() });
        for (auto i = 0; i < numberOfKeys; ++i) {
            wallet.GetNewAuxKey(assetID, accountIndex);
        }
    }

    bitcoin::CWallet wallet(dbPath);
    wallet.LoadWallet(firstRun);

    for (auto i = 0; i < numberOfKeys; ++i) {
        auto pubKey = wallet.GetNewAuxKey(assetID, accountIndex);
        bitcoin::CHDPubKey hdPubKey;
        wallet.GetHDPubKey(assetID, pubKey.GetID(), hdPubKey);
        ASSERT_EQ(hdPubKey.nAccountIndex, accountIndex);
        ASSERT_EQ(hdPubKey.extPubKey.nChild, i + numberOfKeys);
    }
}

TEST_F(BitcoinWalletTests, RecoverAuxChain)
{
    const auto dbName = "recoverAuxChain";
    const auto dbPath = createTempPath(dbName).toStdString();
    bool firstRun = false;
    const AssetID assetID = 384;
    const int accountIndex = 5;
    const int numberOfKeys = 50;
    std::vector<bitcoin::CKeyID> generatedPubKeys;
    std::string mnemonic;
    {
        bitcoin::CWallet wallet(dbPath);
        wallet.LoadWallet(firstRun);
        auto genMnemonic = bitcoin::CMnemonic::Generate(256);
        wallet.GenerateNewHDChain(
            std::string(), std::string{ genMnemonic.begin(), genMnemonic.end() });
        bitcoin::CHDChain hdChain;
        wallet.GetHDChain(hdChain);
        SecureString mnemonicResult, paraphraseResult;
        hdChain.GetMnemonic(mnemonicResult, paraphraseResult);
        mnemonic = std::string{ mnemonicResult.begin(), mnemonicResult.end() };

        for (auto i = 0; i < numberOfKeys; ++i) {
            auto keyID = wallet.GetNewAuxKey(assetID, accountIndex).GetID();
            generatedPubKeys.emplace_back(keyID);
        }
    }

    ASSERT_TRUE(QDir(QString::fromStdString(dbPath)).removeRecursively());

    bitcoin::CWallet wallet(createTempPath(dbName).toStdString());
    wallet.LoadWallet(firstRun);
    wallet.GenerateNewHDChain(std::string(), mnemonic);
    wallet.RecoverAuxChain({ assetID }, { static_cast<uint32_t>(accountIndex) }, numberOfKeys);

    bitcoin::CHDChain chain;
    wallet.GetHDChain(chain);

    for (auto i = 0; i < numberOfKeys; ++i) {
        auto pubKey = generatedPubKeys.at(i);
        bitcoin::CHDPubKey hdPubKey;
        wallet.GetHDPubKey(assetID, pubKey, hdPubKey);

        ASSERT_EQ(hdPubKey.nAccountIndex, accountIndex);
        ASSERT_EQ(hdPubKey.extPubKey.nChild, i);
        ASSERT_EQ(hdPubKey.extPubKey.pubkey.GetID(), pubKey);
    }
}

static void DeriveChildExtKey(bitcoin::CExtKey masterKey, uint32_t nPurpose, uint32_t nCoinType,
    uint32_t nAccountIndex, bool fInternal, uint32_t nChildIndex, bitcoin::CExtKey& extKeyRet)
{
    using namespace bitcoin;
    // Use BIP44 keypath scheme i.e. m / purpose' / coin_type' / account' / change / address_index
    // hd master key
    CExtKey purposeKey; // key at m/purpose'
    CExtKey cointypeKey; // key at m/purpose'/coin_type'
    CExtKey accountKey; // key at m/purpose'/coin_type'/account'
    CExtKey changeKey; // key at m/purpose'/coin_type'/account'/change
    CExtKey childKey; // key at m/purpose'/coin_type'/account'/change/address_index

    // Use hardened derivation for purpose, coin_type and account
    // (keys >= 0x80000000 are hardened after bip32)

    // derive m/purpose'
    masterKey.Derive(purposeKey, nPurpose | 0x80000000);
    // derive m/purpose'/coin_type'
    purposeKey.Derive(cointypeKey, nCoinType | 0x80000000);
    // derive m/purpose'/coin_type'/account'
    cointypeKey.Derive(accountKey, nAccountIndex | 0x80000000);
    // derive m/purpose'/coin_type'/account/change
    accountKey.Derive(changeKey, fInternal ? 1 : 0);
    // derive m/purpose'/coin_type'/account/change/address_index
    changeKey.Derive(extKeyRet, nChildIndex);
}

#if 0
TEST_F(BitcoinWalletTests, LndRecovery)
{
    using namespace bitcoin;
    const auto dbName = "lndRecovery";
    const auto dbPath = createTempPath(dbName).toStdString();
    bool firstRun = false;
    const AssetID assetID = 384;
    const int numberOfKeys = 50;

    const auto chainParams = _assetsModel.assetById(assetID).params();
    CExtKey rootHDKey = DecodeExtKey("xprv9s21ZrQH143K4TWhkURUCcpEKk3a5cgonu5tbdHKMG9Z9N3Vk2uife49XfUT8tWCLFT99FVEncGs3VyKvUPziwSp32rtMMLgp7jEmb5koBv", chainParams);

    std::string targetAddress{"xc1q5d9ytxa8z4ysvnyykykknx2s2sleqq9yu6xr3c"};

    auto deriveGeneralKey = std::bind(DeriveChildExtKey, rootHDKey, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    auto deriveMainChain = std::bind(deriveGeneralKey, 84, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);

    CExtKey extKey;
    for(auto purpose : { 84, 1017 })
    {
        for(auto coinType : { 0, 384 })
        {
            for(int account = 0; account < 13; ++account)
            {
                for(int internal = 1; internal < 2; ++internal)
                {
                    for(int childIndex = 0; childIndex < 3000; ++childIndex)
                    {
                        deriveGeneralKey(purpose, coinType, account, internal, childIndex, extKey);
                        auto decodedAddress = EncodeDestination(GetDestinationForKey(extKey.Neuter().pubkey.GetID(), OutputType::BECH32), chainParams);
                        if(decodedAddress == targetAddress)
                        {
                            std::cout << "Found target address: "
                                      << decodedAddress << " "
                                      << QString("%1/%2/%3/%4/%5").arg(purpose).arg(coinType).arg(account).arg(internal).arg(childIndex).toStdString()
                                      << std::endl;
                        }
                    }
                }
            }
        }
    }
}
#endif

TEST_F(BitcoinWalletTests, EthTxSigning)
{
    eth::Tx tx;
    tx.nonce = "09";
    tx.gasPrice = "04A817C800";
    tx.gasLimit = "5208";
    tx.to = "3535353535353535353535353535353535353535";
    tx.value = "0DE0B6B3A7640000";
    tx.data = "";
    auto raw
        = bitcoin::ParseHex("4646464646464646464646464646464646464646464646464646464646464646");
    bitcoin::CKey privKey;
    privKey.Set(raw.begin(), raw.end(), true);
    ASSERT_TRUE(privKey.IsValid());
    auto signedTx = eth::SignTransaction(tx, 1, privKey);
    ASSERT_EQ(signedTx.sig.r, "28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276");
    ASSERT_EQ(signedTx.sig.s, "67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83");
    ASSERT_EQ(signedTx.sig.v, 37);
    ASSERT_EQ(signedTx.toHex(),
        "0xf86c098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a76400008025"
        "a028ef"
        "61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b38"
        "00ccf555c9f3dc64214b297fb1966a3b6d83");
}

TEST_F(BitcoinWalletTests, SignTransaction)
{
    eth::Tx tx;
    tx.nonce = "00";
    tx.gasPrice = "04A817C800";
    tx.gasLimit = "5208";
    tx.to = "2dc866946A7698d24143612B09B1F67d5966dAeE";
    tx.value = "5F5E100";
    tx.data = "";
    auto raw
        = bitcoin::ParseHex("4646464646464646464646464646464646464646464646464646464646464646");
    bitcoin::CKey privKey;
    privKey.Set(raw.begin(), raw.end(), true);
    ASSERT_TRUE(privKey.IsValid());
    auto signedTx = eth::SignTransaction(tx, 1, privKey);
}

TEST_F(BitcoinWalletTests, ethTxConvertNonce)
{
    eth::u64 nonce = 10;
    auto hexNonce = QString::number(nonce.convert_to<int64_t>(), 16).toUpper().toStdString();
    eth::Tx tx;
    tx.nonce = hexNonce;
    tx.normalize();
    ASSERT_EQ(tx.nonce, "0A");
}

#endif // TST_KEYSTORAGE_HPP
