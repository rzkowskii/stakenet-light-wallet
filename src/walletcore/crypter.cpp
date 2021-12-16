// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypter.h"

#include "crypto/aes.h"
#include "crypto/sha512.h"
#include "script/script.h"
#include "script/standard.h"

#include <boost/foreach.hpp>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <string>
#include <vector>

namespace bitcoin {

int CCrypter::BytesToKeySHA512AES(const std::vector<unsigned char>& chSalt,
    const SecureString& strKeyData, int count, unsigned char* key, unsigned char* iv) const
{
    // This mimics the behavior of openssl's EVP_BytesToKey with an aes256cbc
    // cipher and sha512 message digest. Because sha512's output size (64b) is
    // greater than the aes256 block size (16b) + aes256 key size (32b),
    // there's no need to process more than once (D_0).

    if (!count || !key || !iv)
        return 0;

    unsigned char buf[CSHA512::OUTPUT_SIZE];
    CSHA512 di;

    di.Write((const unsigned char*)strKeyData.c_str(), strKeyData.size());
    if (chSalt.size())
        di.Write(&chSalt[0], chSalt.size());
    di.Finalize(buf);

    for (int i = 0; i != count - 1; i++)
        di.Reset().Write(buf, sizeof(buf)).Finalize(buf);

    memcpy(key, buf, WALLET_CRYPTO_KEY_SIZE);
    memcpy(iv, buf + WALLET_CRYPTO_KEY_SIZE, WALLET_CRYPTO_IV_SIZE);
    OPENSSL_cleanse(buf, sizeof(buf));
    return WALLET_CRYPTO_KEY_SIZE;
}

bool CCrypter::SetKeyFromPassphrase(const SecureString& strKeyData,
    const std::vector<unsigned char>& chSalt, const unsigned int nRounds,
    const unsigned int nDerivationMethod)
{
    if (nRounds < 1 || chSalt.size() != WALLET_CRYPTO_SALT_SIZE)
        return false;

    int i = 0;
    if (nDerivationMethod == 0)
        i = BytesToKeySHA512AES(chSalt, strKeyData, nRounds, vchKey.data(), vchIV.data());

    if (i != (int)WALLET_CRYPTO_KEY_SIZE) {
        OPENSSL_cleanse(vchKey.data(), vchKey.size());
        OPENSSL_cleanse(vchIV.data(), vchIV.size());
        return false;
    }

    fKeySet = true;
    return true;
}

bool CCrypter::SetKey(const CKeyingMaterial& chNewKey, const std::vector<unsigned char>& chNewIV)
{
    if (chNewKey.size() != WALLET_CRYPTO_KEY_SIZE || chNewIV.size() != WALLET_CRYPTO_IV_SIZE)
        return false;

    memcpy(vchKey.data(), chNewKey.data(), chNewKey.size());
    memcpy(vchIV.data(), chNewIV.data(), chNewIV.size());

    fKeySet = true;
    return true;
}

bool CCrypter::Encrypt(
    const CKeyingMaterial& vchPlaintext, std::vector<unsigned char>& vchCiphertext) const
{
    if (!fKeySet)
        return false;

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCKSIZE bytes
    vchCiphertext.resize(vchPlaintext.size() + AES_BLOCKSIZE);

    AES256CBCEncrypt enc(vchKey.data(), vchIV.data(), true);
    size_t nLen = enc.Encrypt(&vchPlaintext[0], vchPlaintext.size(), &vchCiphertext[0]);
    if (nLen < vchPlaintext.size())
        return false;
    vchCiphertext.resize(nLen);

    return true;
}

bool CCrypter::Decrypt(
    const std::vector<unsigned char>& vchCiphertext, CKeyingMaterial& vchPlaintext) const
{
    if (!fKeySet)
        return false;

    // plaintext will always be equal to or lesser than length of ciphertext
    int nLen = vchCiphertext.size();

    vchPlaintext.resize(nLen);

    AES256CBCDecrypt dec(vchKey.data(), vchIV.data(), true);
    nLen = dec.Decrypt(&vchCiphertext[0], vchCiphertext.size(), &vchPlaintext[0]);
    if (nLen == 0)
        return false;
    vchPlaintext.resize(nLen);
    return true;
}

static bool EncryptSecret(const CKeyingMaterial& vMasterKey, const CKeyingMaterial& vchPlaintext,
    const uint256& nIV, std::vector<unsigned char>& vchCiphertext)
{
    CCrypter cKeyCrypter;
    std::vector<unsigned char> chIV(WALLET_CRYPTO_IV_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_IV_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV))
        return false;
    return cKeyCrypter.Encrypt(*((const CKeyingMaterial*)&vchPlaintext), vchCiphertext);
}

// General secure AES 256 CBC encryption routine
bool EncryptAES256(const SecureString& sKey, const SecureString& sPlaintext, const std::string& sIV,
    std::string& sCiphertext)
{
    // Verify key sizes
    if (sKey.size() != 32 || sIV.size() != AES_BLOCKSIZE) {
        return false;
    }

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCKSIZE bytes
    sCiphertext.resize(sPlaintext.size() + AES_BLOCKSIZE);

    AES256CBCEncrypt enc((const unsigned char*)&sKey[0], (const unsigned char*)&sIV[0], true);
    size_t nLen = enc.Encrypt(
        (const unsigned char*)&sPlaintext[0], sPlaintext.size(), (unsigned char*)&sCiphertext[0]);
    if (nLen < sPlaintext.size())
        return false;
    sCiphertext.resize(nLen);
    return true;
}

static bool DecryptSecret(const CKeyingMaterial& vMasterKey,
    const std::vector<unsigned char>& vchCiphertext, const uint256& nIV,
    CKeyingMaterial& vchPlaintext)
{
    CCrypter cKeyCrypter;
    std::vector<unsigned char> chIV(WALLET_CRYPTO_IV_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_IV_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV))
        return false;
    return cKeyCrypter.Decrypt(vchCiphertext, *((CKeyingMaterial*)&vchPlaintext));
}

// General secure AES 256 CBC decryption routine
bool DecryptAES256(const SecureString& sKey, const std::string& sCiphertext, const std::string& sIV,
    SecureString& sPlaintext)
{
    // Verify key sizes
    if (sKey.size() != 32 || sIV.size() != AES_BLOCKSIZE) {
        return false;
    }

    // plaintext will always be equal to or lesser than length of ciphertext
    int nLen = sCiphertext.size();

    sPlaintext.resize(nLen);

    AES256CBCDecrypt dec((const unsigned char*)&sKey[0], (const unsigned char*)&sIV[0], true);
    nLen = dec.Decrypt(
        (const unsigned char*)&sCiphertext[0], sCiphertext.size(), (unsigned char*)&sPlaintext[0]);
    if (nLen == 0)
        return false;
    sPlaintext.resize(nLen);
    return true;
}

static bool DecryptKey(const CKeyingMaterial& vMasterKey,
    const std::vector<unsigned char>& vchCryptedSecret, const CPubKey& vchPubKey, CKey& key)
{
    CKeyingMaterial vchSecret;
    if (!DecryptSecret(vMasterKey, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
        return false;

    if (vchSecret.size() != 32)
        return false;

    key.Set(vchSecret.begin(), vchSecret.end(), vchPubKey.IsCompressed());
    return key.VerifyPubKey(vchPubKey);
}

bool CCryptoKeyStore::SetCrypted()
{
    LOCK(cs_KeyStore);
    if (fUseCrypto)
        return true;
    fUseCrypto = true;
    return true;
}

bool CCryptoKeyStore::Lock(bool fAllowMixing)
{
    if (!SetCrypted())
        return false;

    if (!fAllowMixing) {
        LOCK(cs_KeyStore);
        vMasterKey.clear();
    }

    fOnlyMixingAllowed = fAllowMixing;
    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn, bool fForMixingOnly)
{
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        vMasterKey = vMasterKeyIn;

        if (!cryptedHDChain.IsNull()) {
            bool chainPass = false;
            // try to decrypt seed and make sure it matches
            CHDChain hdChainTmp;
            if (DecryptHDChain(hdChainTmp)) {
                // make sure seed matches this chain
                chainPass = cryptedHDChain.GetID() == hdChainTmp.GetSeedHash();
            }
            if (!chainPass) {
                vMasterKey.clear();
                return false;
            }
        }
        fDecryptionThoroughlyChecked = true;
    }
    fOnlyMixingAllowed = fForMixingOnly;
    NotifyStatusChanged(this);
    return true;
}

bool CCryptoKeyStore::EncryptHDChain(const CKeyingMaterial& vMasterKeyIn)
{
    // should call EncryptKeys first

    if (!cryptedHDChain.IsNull())
        return true;

    if (cryptedHDChain.IsCrypted())
        return true;

    // make sure seed matches this chain
#ifndef Q_OS_WIN
    if (hdChainInternal.GetID() != hdChainInternal.GetSeedHash())
        return false;
#endif

    std::vector<unsigned char> vchCryptedSeed;
    if (!EncryptSecret(vMasterKeyIn, hdChainInternal.GetSeed(), hdChainInternal.GetID(), vchCryptedSeed))
        return false;

    hdChainInternal.Debug(__func__);
    cryptedHDChain = hdChainInternal;
    cryptedHDChain.SetCrypted(true);

    SecureVector vchSecureCryptedSeed(vchCryptedSeed.begin(), vchCryptedSeed.end());
    if (!cryptedHDChain.SetSeed(vchSecureCryptedSeed, false))
        return false;

    SecureVector vchMnemonic;
    SecureVector vchMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (hdChainInternal.GetMnemonic(vchMnemonic, vchMnemonicPassphrase)) {
        std::vector<unsigned char> vchCryptedMnemonic;
        std::vector<unsigned char> vchCryptedMnemonicPassphrase;

        if (!vchMnemonic.empty()
            && !EncryptSecret(vMasterKeyIn, vchMnemonic, hdChainInternal.GetID(), vchCryptedMnemonic))
            return false;
        if (!vchMnemonicPassphrase.empty()
            && !EncryptSecret(vMasterKeyIn, vchMnemonicPassphrase, hdChainInternal.GetID(),
                   vchCryptedMnemonicPassphrase))
            return false;

        SecureVector vchSecureCryptedMnemonic(vchCryptedMnemonic.begin(), vchCryptedMnemonic.end());
        SecureVector vchSecureCryptedMnemonicPassphrase(
            vchCryptedMnemonicPassphrase.begin(), vchCryptedMnemonicPassphrase.end());
        if (!cryptedHDChain.SetMnemonic(
                vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase, false))
            return false;
    }

    if (!hdChainInternal.SetNull())
        return false;

    SetCrypted();

    return true;
}

bool CCryptoKeyStore::DecryptHDChain(CHDChain& hdChainRet) const
{
    if (!IsCrypted())
        return true;

    if (cryptedHDChain.IsNull())
        return false;

    if (!cryptedHDChain.IsCrypted())
        return false;

    SecureVector vchSecureSeed;
    SecureVector vchSecureCryptedSeed = cryptedHDChain.GetSeed();
    std::vector<unsigned char> vchCryptedSeed(
        vchSecureCryptedSeed.begin(), vchSecureCryptedSeed.end());
    if (!DecryptSecret(vMasterKey, vchCryptedSeed, cryptedHDChain.GetID(), vchSecureSeed))
        return false;

    hdChainRet = cryptedHDChain;
    if (!hdChainRet.SetSeed(vchSecureSeed, false))
        return false;

    // hash of decrypted seed must match chain id
    if (hdChainRet.GetSeedHash() != cryptedHDChain.GetID())
        return false;

    SecureVector vchSecureCryptedMnemonic;
    SecureVector vchSecureCryptedMnemonicPassphrase;

    // it's ok to have no mnemonic if wallet was initialized via hdseed
    if (cryptedHDChain.GetMnemonic(vchSecureCryptedMnemonic, vchSecureCryptedMnemonicPassphrase)) {
        SecureVector vchSecureMnemonic;
        SecureVector vchSecureMnemonicPassphrase;

        std::vector<unsigned char> vchCryptedMnemonic(
            vchSecureCryptedMnemonic.begin(), vchSecureCryptedMnemonic.end());
        std::vector<unsigned char> vchCryptedMnemonicPassphrase(
            vchSecureCryptedMnemonicPassphrase.begin(), vchSecureCryptedMnemonicPassphrase.end());

        if (!vchCryptedMnemonic.empty()
            && !DecryptSecret(
                   vMasterKey, vchCryptedMnemonic, cryptedHDChain.GetID(), vchSecureMnemonic))
            return false;
        if (!vchCryptedMnemonicPassphrase.empty()
            && !DecryptSecret(vMasterKey, vchCryptedMnemonicPassphrase, cryptedHDChain.GetID(),
                   vchSecureMnemonicPassphrase))
            return false;

        if (!hdChainRet.SetMnemonic(vchSecureMnemonic, vchSecureMnemonicPassphrase, false))
            return false;
    }

    hdChainRet.SetCrypted(false);
    hdChainRet.Debug(__func__);

    return true;
}

bool CCryptoKeyStore::SetHDChain(const CHDChain& chain)
{
    if (IsCrypted())
        return false;

    if (chain.IsCrypted())
        return false;

    hdChainInternal = chain;
    return true;
}

bool CCryptoKeyStore::SetCryptedHDChain(const CHDChain& chain)
{
    if (!SetCrypted())
        return false;

    if (!chain.IsCrypted())
        return false;

    cryptedHDChain = chain;
    return true;
}

bool CCryptoKeyStore::GetHDChain(CHDChain& hdChainRet) const
{
    if (IsCrypted()) {
        hdChainRet = cryptedHDChain;
        return !cryptedHDChain.IsNull();
    }

    hdChainRet = hdChainInternal;
    return !hdChainInternal.IsNull();
}

}
