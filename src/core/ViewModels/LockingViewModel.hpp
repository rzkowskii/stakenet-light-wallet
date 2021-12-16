#ifndef LOCKINGVIEWMODEL_HPP
#define LOCKINGVIEWMODEL_HPP

#include <QObject>

class WalletDataSource;

class LockingViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isEncrypted READ isEncrypted NOTIFY isEncryptedChanged)

public:
    explicit LockingViewModel(WalletDataSource& dataSource, QObject* parent = nullptr);

    bool isEncrypted() const;

signals:
    void isEncryptedChanged();

public slots:
    void lock();

private:
    WalletDataSource& _walletDataSource;
};

#endif // LOCKINGVIEWMODEL_HPP
