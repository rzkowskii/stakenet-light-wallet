#ifndef AUTOPILOTMODEL_HPP
#define AUTOPILOTMODEL_HPP

#include <Data/AssetSettings.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

#include <QObject>

class WalletAssetsModel;
class LndGrpcClient;

class AutopilotModel : public QObject {
    Q_OBJECT
public:
    explicit AutopilotModel(AssetID assetID, const WalletAssetsModel& assetsModel, LndGrpcClient* client,
        QObject* parent = nullptr);
    ~AutopilotModel();

    void setActive(bool active);
    bool isActive() const;

    void setDetails(double allocation, unsigned maxChannels);
    double allocation() const;
    unsigned maxChannels() const;

signals:
    bool isActiveChanged();

public slots:

private:
    void evaluateState();
    void evaluateDetails();
    Promise<bool> autopilotStatus();
    Promise<void> modifyAutopilotStatus(bool active);
    Promise<void> restartWithNewConstraints(
        double allocation, uint32_t maxChannels, uint32_t feeRate);
    void init();
    unsigned satsPerByte() const;

private:
    QPointer<LndGrpcClient> _client;
    QPointer<AssetSettings> _assetSettings;
    QTimer* _evaluationTimer{ nullptr };
    AssetID _assetID;
    std::pair<unsigned, AutopilotDetails> _autopilotDefaultSettings;
};

#endif // AUTOPILOTMODEL_HPP
