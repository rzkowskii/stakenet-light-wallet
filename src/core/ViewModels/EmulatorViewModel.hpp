#ifndef EMULATORVIEWMODEL_HPP
#define EMULATORVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <memory>

class WalletDataSource;
class ApplicationViewModel;
class EmulatorWalletDataSource;

class EmulatorViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)

public:
    EmulatorViewModel(QObject* parent = nullptr);
    ~EmulatorViewModel();

    int maxHeight() const;
    void setMaxHeight(int newHeight);

public slots:
    void addTransaction(AssetID currentModel, int count);
    void clearTransactions(AssetID currentModel);
    void initialize(ApplicationViewModel* applicationViewModel);
    void requestNewBlock(AssetID assetID, int count, QString addressTo);

signals:
    void maxHeightChanged();
    void newBlockRequested(AssetID assetID, int count, QString addressTo);
    void reorgRequested(AssetID assetID, int disconnectCount, int connectCount, QString addressTo);

private:
    QPointer<EmulatorWalletDataSource> _walletDataSource;
    int _maxHeight{ 581575 };
};

#endif // EMULATORVIEWMODEL_HPP
