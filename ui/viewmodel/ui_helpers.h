#pragma once
#include <QObject>
#include "wallet/common.h"

Q_DECLARE_METATYPE(ufo::wallet::TxID)
Q_DECLARE_METATYPE(ufo::wallet::TxParameters)

namespace ufoui
{
    enum class Currencies
    {
        Ufo,
        Bitcoin,
        Litecoin,
        Qtum,
        Unknown
    };

    QString toString(Currencies currency);
    std::string toStdString(Currencies currency);
    QString toString(const ufo::wallet::WalletID&);
    QString toString(const ufo::Merkle::Hash&);
    // convert amount to ui string with "." as a separator
    QString AmountToString(const ufo::Amount& value, Currencies coinType = Currencies::Unknown);
    // expects ui string with a "." as a separator
    ufo::Amount StringToAmount(const QString& value);
    QString toString(const ufo::Timestamp& ts);
    Currencies convertSwapCoinToCurrency(ufo::wallet::AtomicSwapCoin coin);

    class Filter
    {
    public:
        Filter(size_t size = 12);
        void addSample(double value);
        double getAverage() const;
        double getMedian() const;
    private:
        std::vector<double> _samples;
        size_t _index;
        bool _is_poor;
    };
    QDateTime CalculateExpiresTime(ufo::Height currentHeight, ufo::Height expiresHeight);
}  // namespace ufoui
