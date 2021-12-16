#ifndef SYNCSTATEPROVIDER_HPP
#define SYNCSTATEPROVIDER_HPP

#include <QObject>

class SyncStateProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool syncing READ syncing WRITE setSyncing NOTIFY syncingChanged)
    Q_PROPERTY(bool scanning READ scanning WRITE setScanning NOTIFY scanningChanged)
    Q_PROPERTY(unsigned bestBlockHeight READ bestBlockHeight NOTIFY bestBlockHeightChanged)
    Q_PROPERTY(unsigned rescanProgress READ rescanProgress NOTIFY rescanProgressChanged)

public:
    explicit SyncStateProvider(QObject* parent = nullptr);
    ~SyncStateProvider() override;

    bool syncing() const;
    void setSyncing(const bool syncing);

    bool scanning() const;
    void setScanning(const bool scanning);

    unsigned bestBlockHeight() const;
    void setBestBlockHeight(unsigned bestBlockHeight);

    unsigned rescanProgress() const;
    void setRescanProgress(unsigned blocks);

signals:
    void syncingChanged();
    void scanningChanged();
    void bestBlockHeightChanged();
    void rescanProgressChanged();

public slots:

private:
    bool _syncing{ false };
    bool _scanning{ false };
    unsigned _bestBlockHeight{ 0 };
    unsigned _blocks{ 0 };
};

#endif // SYNCSTATEPROVIDER_HPP
