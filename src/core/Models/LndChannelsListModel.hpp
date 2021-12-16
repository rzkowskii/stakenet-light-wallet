#ifndef LIGHTINGCHANNELSLISTMODEL_HPP
#define LIGHTINGCHANNELSLISTMODEL_HPP

#include <QAbstractListModel>
#include <QPointer>
#include <array>
#include <boost/optional.hpp>
#include <unordered_map>

#include <LndTools/LndTypes.hpp>
#include <Models/PaymentChannelsListModel.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class ApplicationViewModel;
class PaymentNodeInterface;
class AbstractChainDataSource;
class AbstractTransactionsCache;
class ChainView;
struct RentChannelDetails;
class ChannelRentalHelper;
class LnDaemonsInterface;
class AssetsTransactionsCache;
class AbstractChainManager;
class LnDaemonInterface;

class LndChannelsListModel : public PaymentChannelsListModel {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(unsigned recentPendingConfirmation READ recentPendingConfirmation NOTIFY
            recentPendingConfirmationChanged)

public:
    explicit LndChannelsListModel(AssetID assetID, AssetsTransactionsCache* assetsTxCache,
        AbstractChainDataSource* chainDataSource, AbstractChainManager* chainManager,
        ChannelRentalHelper* channelRental, PaymentNodeInterface* paymentNodeInterface,
        QObject* parent = nullptr);
    ~LndChannelsListModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    unsigned recentPendingConfirmation() const;

signals:
    void recentPendingConfirmationChanged();

public slots:
    void refresh() override;
    QVariantMap get(QString channelOutpoint);
    bool canOpenRentalChannel();
    bool canExtendRentalChannel(double expiresAtTimeSeconds);

private slots:
    void onUpdateChannels(std::vector<LndChannel> channels, ChannelType type);
    //    void onUpdateChannels(std::vector<ConnextChannel> channels, ChannelType type);
    void updateConfirmations();
    void onChainHeightChanged();
    void updateForceCloseDetails();
    void updateRentalChannels(AssetID assetID, QString channelOutpoint);

private:
    LndChannel channelByIndex(int row) const;
    ChannelType channelType(int row) const;
    QString getFundingTxId(LndChannel channel, ChannelType type) const;
    int getConfirmations(LndChannel channel, ChannelType type) const;
    void getChannelHeight(LndChannel channel, ChannelType type);
    void getChannelDate(QString outpoint, QPersistentModelIndex index);
    void checkChannelIsRental(uint64_t channelId);
    bool isRentalChannel(QString channelOutpoint) const;
    uint64_t getExpiresAt(bool isRental, QString channelOutpoint) const;
    uint64_t getRentalCreatedDate(QString channelOutpoint) const;
    void setRecentPendingConfirmation(unsigned value);
    void findRecentPendingConfirmation();
    void onAssetIDUpdated();
    void updateChannelsDate(ChannelType type = ChannelType::Undefined);
    void initPaymentNode();
    void init(AssetsTransactionsCache* txCache, AbstractChainManager* chainManager);

private:
    struct ChannelTypePairHash {
        std::size_t operator()(const std::pair<QString, ChannelType>& value) const
        {
            return std::hash<QString>()(value.first)
                ^ std::hash<size_t>()(static_cast<size_t>(value.second));
        }
    };

    using LndChannels = std::vector<LndChannel>;
    QPointer<LnDaemonInterface> _paymentNode;
    std::unordered_map<std::pair<QString, ChannelType>, uint32_t, ChannelTypePairHash>
        _confirmations;
    std::array<LndChannels, 5> _channels; // { Active, Inactive, Pending, Closing, ForceClosing }
    std::map<QString, qint64> _channelsDate;
    QPointer<AbstractChainDataSource> _chainDataSource;
    QPointer<AbstractTransactionsCache> _transactionsCache;
    std::shared_ptr<ChainView> _chainView;
    size_t _tipHeight{ 0 };
    QPointer<ChannelRentalHelper> _channelRentalHelper;
    unsigned _recentPendingConfirmation{ 0 };
};

#endif // LIGHTINGCHANNELSLISTMODEL_HPP
