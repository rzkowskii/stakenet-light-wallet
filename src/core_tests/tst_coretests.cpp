// Copyright (c) %YEAR The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <Data/TransactionEntry.hpp>
#include <QSignalSpy>
#include <Tools/Common.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <boost/progress.hpp>
#include <gen-grpc/tesgrpcserver.pb.h>
#include <golomb/gcs.h>
#include <gtest/gtest.h>
#include <random>
#include <serialize.h>
#include <streams.h>
#include <utilstrencodings.h>
#include <EthCore/Encodings.hpp>

#include <QStandardPaths>

using namespace testing;
using namespace boost;

static std::string random_string(size_t length)
{
    auto randchar = []() -> char {
        const char charset[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

TEST(CoreTests, CoreTests)
{
    EXPECT_EQ(1, 1);
}

TEST(CoreTests, GolumbBuild)
{
    bitcoin::Filter::key_t key{ 0x4c, 0xb1, 0xab, 0x12, 0x57, 0x62, 0x1e, 0x41, 0x3b, 0x8b, 0x0e,
        0x26, 0x64, 0x8d, 0x4a, 0x15 };
    std::vector<std::string> contents{ "Alex", "Bob", "Charlie", "Dick", "Ed", "Frank", "George",
        "Harry", "Ilya", "John", "Kevin", "Larry", "Michael", "Nate", "Owen", "Paul", "Quentin" };

    const uint64_t M = 784931;

    bitcoin::Filter filter = bitcoin::Filter::WithKeyMP(key, M, 19);
    filter.addEntries(contents);
    filter.build();
    ASSERT_TRUE(filter.match(std::string("Dick")));
    ASSERT_FALSE(filter.match(std::string("Dic")));

    bitcoin::Filter fromSerializedData
        = bitcoin::Filter::FromNMPBytes(key, contents.size(), M, 19, filter.bytes());

    for (auto&& testV : contents) {
        ASSERT_TRUE(fromSerializedData.match(testV));
    }
}

TEST(CoreTests, GolumbBenchmark)
{
    static const bitcoin::Filter::key_t key{ 0x4c, 0xb1, 0xab, 0x12, 0x57, 0x62, 0x1e, 0x41, 0x3b,
        0x8b, 0x0e, 0x26, 0x64, 0x8d, 0x4a, 0x15 };

    static const size_t nAddressBytes = 31 * 2;

    static const uint64_t M = 784931;

    auto testFilter = [](int numberOfItems, int numberOfTries, int numberOfTestItems) {
        std::vector<std::string> addresses(numberOfTestItems);
        bitcoin::Filter filter = bitcoin::Filter::WithKeyMP(key, M, 19);
        for (size_t i = 0; i < numberOfItems; ++i) {
            auto str = random_string(nAddressBytes);
            filter.addEntry(std::vector<unsigned char>(str.begin(), str.end()));
        }

        for (size_t i = 0; i < numberOfTestItems; ++i) {
            addresses[i] = random_string(nAddressBytes);
        }

        {
            progress_timer timer;
            filter.build();
        }

        {
            progress_timer timer;
            std::mt19937 rng;
            for (size_t i = 0; i < numberOfTries; ++i) {
                filter.matchAny(addresses);
            }
        }
    };

    using Entry = std::tuple<int, int, int>;
    for (auto&& testSet : { Entry{ 100, 1, 50 }, Entry{ 100, 10, 50 }, Entry{ 20000, 1, 100 },
             Entry{ 20000, 1, 1000 } }) {
        std::cout << "Benchmarking test with params, numberOfItems = " << std::get<0>(testSet)
                  << " numberOfTries = " << std::get<1>(testSet)
                  << "numberOfTestItems = " << std::get<2>(testSet) << std::endl;
        testFilter(std::get<0>(testSet), std::get<1>(testSet), std::get<2>(testSet));
        std::cout << std::endl;
    }

    ASSERT_TRUE(true);
}

TEST(CoreTests, PromiseMultipleResolve)
{
    static auto g = [] {
        return QtPromise::QPromise<void>([](const auto& resolve, const auto& reject) {
            QTimer::singleShot(2000, [=] { resolve(); });
        });
    };
    auto f = [] {
        return QtPromise::QPromise<void>([](const auto& resolve, const auto& reject) {
            auto promise = g();
            promise
                .then([resolve] {
                    qDebug() << "Delayed promise resolved";
                    resolve();
                })
                .finally([] { qDebug() << "Delayed finally"; });
        });
    };

    f().then([] { qDebug() << "Resolved"; }).fail([] { qDebug() << "Rejected"; }).wait();

    QObject dummy;

    QSignalSpy spy(&dummy, &QObject::destroyed);
    spy.wait();
}

TEST(CoreTests, ExceptionAwait)
{

    static auto g = [] {
        return QtPromise::QPromise<void>([](const auto& resolve, const auto& reject) {
            QTimer::singleShot(2000, [=] { reject(); });
        });
    };

    auto f = [] { return g().tapFail([] { throw std::runtime_error("bla_bla"); }); };

    f().then([] { qDebug() << "Resolved"; }).tapFail([] { throw; }).wait();

    QObject dummy;

    QSignalSpy spy(&dummy, &QObject::destroyed);
    spy.wait();
}

TEST(CoreTests, PromiseBaseClassFail)
{
    const std::string text{ "bla_bla" };
    auto f = [text] {
        return QtPromise::attempt([text] {
            //            throw std::runtime_error(text);
            return QtPromise::QPromise<void>::reject(text);
        });
    };

    static auto g = [f] {
        return QtPromise::QPromise<void>([f](const auto& resolve, const auto& reject) {
            f().then([resolve] { resolve(); })
                .fail([reject](std::exception& ex) { reject(std::make_exception_ptr(ex)); })
                .fail([reject] { reject(std::current_exception()); });
        });
    };

    std::string expected;
    g().tapFail([&expected](const std::exception& ex) { expected = ex.what(); })
        .tapFail([&expected](std::string v) { expected = v; })
        .wait();
    ASSERT_EQ(expected, text);
}

TEST(CoreTests, PromiseBaseClassFailSecond)
{
    struct Base {
        virtual std::string foo() const { return "base"; }
    };

    struct Derived : Base {
        Derived() {}
        Derived(const Derived& rhs) { qDebug() << "Copy consturctor"; }
        Derived& operator=(const Derived& rhs)
        {
            qDebug() << "Assignment operator";
            return *this;
        }
        std::string foo() const override { return "derived"; }
    };

    static auto g = [] {
        return QtPromise::QPromise<void>(
            [](const auto& resolve, const auto& reject) { reject(Derived{}); });
    };

    g().fail([](const Base& base) { ASSERT_EQ(base.foo(), "derived"); }).wait();
}

static QtPromise::QPromise<void> DoSomething()
{
    return QtPromise::resolve().delay(5000);
}

TEST(CoreTests, PromiseAnotherThreadIssue)
{
    struct Foo : public QObject {
        void doSomethingAndDoSomethingElse()
        {
            QPointer<Foo> self(this);
            DoSomething().then([self] {
                if (self) {
                    // do something with self.
                    qDebug() << "Self is ok";
                } else {
                    qDebug() << "Foo already deleted";
                }
            });
        }
    };

    Foo* foo = new Foo;
    foo->doSomethingAndDoSomethingElse();
    //    foo->deleteLater();
    delete foo;

    QEventLoop loop;
    QTimer::singleShot(10000, [&loop] { loop.quit(); });
    loop.exec();
}

TEST(CoreTests, PromiseStressResolve)
{
    std::vector<std::pair<QtPromise::QPromiseResolve<void>, QtPromise::QPromiseReject<void>>>
        promises;
    for (size_t i = 0; i < 1000; ++i) {
        QtPromise::QPromise<void>(
            [&promises](auto resolve, auto reject) { promises.emplace_back(resolve, reject); });
    }

    constexpr const int nAcceptors = 1;
    constexpr const int nRejectors = 0;

    std::vector<std::thread> acceptors;
    std::vector<std::thread> rejectors;

    for (int i = 0; i < nAcceptors; ++i) {
        acceptors.emplace_back(std::thread([&promises] {
            for (auto&& promise : promises) {
                promise.first();
            }
        }));
    }

    for (int i = 0; i < nRejectors; ++i) {
        acceptors.emplace_back(std::thread([&promises] {
            for (auto&& promise : promises) {
                promise.second(QtPromise::QPromiseUndefinedException{});
            }
        }));
    }

    for (auto& acceptor : acceptors) {
        acceptor.join();
    }

    for (auto& rejector : rejectors) {
        rejector.join();
    }
}

TEST(CoreTests, CoinUserInputParsing)
{
    ASSERT_EQ(10000000, ParseUserInputCoin("0.1"));
    ASSERT_EQ(990000, ParseUserInputCoin("0.0099"));
    ASSERT_EQ(990000, ParseUserInputCoin("0.0099000"));
    ASSERT_EQ(300000000, ParseUserInputCoin("3.0"));
    ASSERT_EQ(300100000, ParseUserInputCoin("3.001"));
    ASSERT_EQ(500000000, ParseUserInputCoin("5"));
    ASSERT_EQ(500000000, ParseUserInputCoin("5.0000"));
    ASSERT_EQ(110000, ParseUserInputCoin("0.0011"));
    ASSERT_EQ(120000, ParseUserInputCoin("0.0012"));
    ASSERT_EQ(199000, ParseUserInputCoin("0.001990000"));
    ASSERT_EQ(10000000000, ParseUserInputCoin("100"));
    ASSERT_EQ(20000000000, ParseUserInputCoin("200.0"));
    ASSERT_EQ(1, ParseUserInputCoin("0.00000001"));
}

TEST(CoreTests, GenericProtobufDbTest)
{
    auto path = QString("%1/shared_db")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QDir tmpDir(path);

    if (tmpDir.exists()) {
        tmpDir.rmpath(".");
    }

    tmpDir.mkpath(".");

    auto openDb = [](auto path) {
        return std::make_shared<Utils::LevelDBSharedDatabase>(path.toStdString(), 100000);
    };

    std::vector<std::string> values;

    for (size_t i = 0; i < 10; ++i) {
        values.emplace_back(std::to_string(i));
    }

    static const std::string index{ "index" };

    {
        auto handle = openDb(tmpDir.absolutePath());
        Utils::GenericProtoDatabase<test::TestProtoStructure> db(handle, index);
        for (auto&& value : values) {
            test::TestProtoStructure proto;
            proto.set_data(value);
            ASSERT_TRUE(db.save(proto));
        }
    }

    auto handle = openDb(tmpDir.absolutePath());
    Utils::GenericProtoDatabase<test::TestProtoStructure> db(handle, index);
    db.load();

    for (auto&& value : values) {
        auto it = std::find_if(std::begin(db.values()), std::end(db.values()),
            [value](const auto& it) { return it.second.data() == value; });

        ASSERT_NE(it, std::end(db.values()));
    }
}

TEST(CoreTests, EthExp10DecimalCache)
{
    ASSERT_EQ(eth::u256(1), eth::exp10(0));
    ASSERT_EQ(eth::u256(10), eth::exp10(1));
    ASSERT_EQ(eth::u256(100000000), eth::exp10(8));
    ASSERT_EQ(eth::ether, eth::exp10(18));
}
