// Copyright (c) %YEAR The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TST_ETHAPI_HPP
#define TST_ETHAPI_HPP

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QWebSocket>
#include <boost/multiprecision/cpp_int.hpp>

#include <Data/TransactionEntry.hpp>
#include <EthCore/Encodings.hpp>
#include <Networking/Web3Client.hpp>
#include <key.h>
#include <utilstrencodings.h>

using namespace eth;

static QUrl testnetWebSocketUrl("wss://ropsten.infura.io/ws/v3/12535e2cda8c485eae0d513904a3723a");

struct EthWeb3ApiTest : ::testing::Test {

    std::unique_ptr<AbstractWeb3Client> client;

    void SetUp()
    {
        client = std::make_unique<Web3Client>(testnetWebSocketUrl);
        client->open();

        QSignalSpy connected(client.get(), &Web3Client::connected);
        ASSERT_TRUE(connected.wait(10000));
    }
};

TEST(EthWebSocketApiTest, Connection)
{
    QWebSocket webSocket;
    QSignalSpy connected(&webSocket, &QWebSocket::connected);
    QObject::connect(&webSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error),
        [&webSocket](auto error) { qDebug() << "error" << error << webSocket.errorString(); });
    webSocket.open(testnetWebSocketUrl);
    ASSERT_TRUE(connected.wait());
}

TEST(EthWebSocketApiTest, GetAccountBalance)
{
    QWebSocket webSocket;
    QSignalSpy connected(&webSocket, &QWebSocket::connected);
    QObject::connect(&webSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error),
        [&webSocket](auto error) { qDebug() << "error" << error << webSocket.errorString(); });
    webSocket.open(testnetWebSocketUrl);

    ASSERT_TRUE(connected.wait());

    QSignalSpy responseReady(&webSocket, &QWebSocket::textMessageReceived);
    webSocket.sendTextMessage(
        "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBalance\",\"params\": "
        "[\"0x8055b3bddfc17f15851205fc327a693970292ce7\", \"latest\"],\"id\":1}");
    ASSERT_TRUE(responseReady.wait());

    qDebug() << responseReady.takeFirst().front().toString();
}

TEST_F(EthWeb3ApiTest, GetAccountBalance)
{
    eth::u256 balance;
    client->getBalance("0x8055b3bddfc17f15851205fc327a693970292ce7")
        .tap([&balance](eth::u256 balanceReceived) { balance = balanceReceived; })
        .wait();

    ASSERT_EQ(1500 * eth::finney, balance);
}

TEST_F(EthWeb3ApiTest, GetAccountBalanceOurPrivateKey)
{
    eth::u256 balance;
    client->getBalance("0x8055b3bddfc17f15851205fc327a693970292ce7")
        .tap([&balance](eth::u256 balanceReceived) { balance = balanceReceived; })
        .wait();

    ASSERT_EQ(1500 * eth::finney, balance);
}

TEST_F(EthWeb3ApiTest, BlockNumber)
{
    eth::u256 blockNumber;
    client->getBlockNumber()
        .tap([&blockNumber](eth::u256 blockNumberReceived) { blockNumber = blockNumberReceived; })
        .wait();

    ASSERT_EQ(true, blockNumber > 0);
}

TEST_F(EthWeb3ApiTest, ChainId)
{
    eth::u256 chainId;
    client->getChainId()
        .tap([&chainId](eth::u256 chainIdReceived) { chainId = chainIdReceived; })
        .wait();

    ASSERT_EQ(3, chainId);
}

TEST_F(EthWeb3ApiTest, EstimateGas)
{
    QVariantMap params;
    params["to"] = "0x11f4d0A3c12e86B4b5F39B213F7E19D048276DAe";
    params["data"] = "0xc6888fa10000000000000000000000000000000000000000000000000000000000000003";

    eth::u256 amt;

    client->estimateGas(params).tap([&amt](eth::u256 amtReceived) { amt = amtReceived; }).wait();

    ASSERT_EQ(21204, amt);
}

TEST_F(EthWeb3ApiTest, GetTransactionCount)
{
    eth::u64 txCount;
    client->getTransactionCount("0x11f4d0A3c12e86B4b5F39B213F7E19D048276DAe")
        .tap([&txCount](eth::u64 txCountReceived) { txCount = txCountReceived; })
        .wait();

    ASSERT_EQ(0, txCount);
}

TEST_F(EthWeb3ApiTest, getTransactionByHash)
{
    eth::u64 blockNumber;
    client
        ->getTransactionByHash("0xf9faa83bfea25209a63e62bf76c6dbdc61886e63b5b0ba6e137e5d2a5b7bb561")
        .tap([&blockNumber](QVariantMap response) {
            blockNumber = eth::u64{ response["blockNumber"].toString().toStdString() };
        })
        .wait();

    ASSERT_EQ(8725038, blockNumber);
}

TEST_F(EthWeb3ApiTest, getTransactionReceipt)
{
    eth::u64 blockNumber;
    client
        ->getTransactionReceipt(
            "0xf9faa83bfea25209a63e62bf76c6dbdc61886e63b5b0ba6e137e5d2a5b7bb561")
        .tap([&blockNumber](QVariantMap response) {
            blockNumber = eth::u64{ response["blockNumber"].toString().toStdString() };
        })
        .wait();

    ASSERT_EQ(8725038, blockNumber);
}

TEST_F(EthWeb3ApiTest, subscribe)
{
    client->subscribe("newHeads").wait();
    QObject dummy;
    QSignalSpy spy(&dummy, &QObject::destroyed);
    spy.wait(100000);
}

TEST_F(EthWeb3ApiTest, balanceOfEncoding)
{
    QString address = "0xa6Af4c9e671a7E70ebA8fb4509a119471116D5BC";
    auto payload = erc20::balanceOfPayload(address);
    ASSERT_EQ(
        "0x70a08231000000000000000000000000a6af4c9e671a7e70eba8fb4509a119471116d5bc", payload);

    QVariantMap params;
    params["to"] = "0x0a180a76e4466bf68a7f86fb029bed3cccfaaac5"; // weth on ropsten
    params["data"] = payload;
    eth::u256 balance;
    client->call(params)
        .then([&balance](QString result) { balance = eth::u256{ result.toStdString() }; })
        .wait();

    ASSERT_GT(balance, 0);
}

TEST_F(EthWeb3ApiTest, transferTokenEncoding)
{
    QString address = "0xA4A33005b69b055Da57ec494026f2f9f389aa9Ea";
    auto payload = erc20::transferPayload(address, eth::u256{ 100000000 });
    ASSERT_EQ("0xa9059cbb000000000000000000000000a4a33005b69b055da57ec494026f2f9f389aa9ea0000000000000000000000000000000000000000000000000000000005f5e100",
        payload.toStdString());
}

TEST_F(EthWeb3ApiTest, ethOnChainTxExportBits)
{
    EthOnChainTx ethTx(60, "0xe617f63b0fb2269f83efbc07ef68ed4ae33d792b1c99bdef4451fb12a6d4be17",
        "0xacd1120b198143cb3682e51fd74d8d669694ceaea659960ae34d15c4f347ea7c", 11555378,
        eth::u256{ "0x5B48" }, 21204, 0,
        "0000000000000000000000007a250d5630b4cf539739df2c5dacb4c659f2488d", 128,
        "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed", "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359",
        QDateTime(QDate(2012, 7, 6), QTime(23, 55, 0)), {});

    ASSERT_EQ(ethTx.txId(), "0xe617f63b0fb2269f83efbc07ef68ed4ae33d792b1c99bdef4451fb12a6d4be17");
    ASSERT_EQ(
        ethTx.blockHash(), "acd1120b198143cb3682e51fd74d8d669694ceaea659960ae34d15c4f347ea7c");
    ASSERT_EQ(ethTx.from(), "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed");
    ASSERT_EQ(ethTx.to(), "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359");
    ASSERT_EQ(ethTx.blockHeight(), 11555378);
    ASSERT_EQ(ethTx.transactionDate(), QDateTime(QDate(2012, 7, 6), QTime(23, 55, 0)));
    ASSERT_EQ(ethTx.gasUsed(), 23368);
    ASSERT_EQ(ethTx.gasPrice(), 21204);
    ASSERT_EQ(ethTx.value(), 0);
}

TEST_F(EthWeb3ApiTest, ethToChecksumAddress)
{
    QString checksumAddress
        = eth::ChecksumAddress(QString("0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359"));
    ASSERT_EQ(checksumAddress.toStdString(), "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359");
}

#endif // TST_ETHAPI_HPP
