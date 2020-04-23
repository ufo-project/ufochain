#include "ui_helpers.h"
#include <QDateTime>
#include <QLocale>
#include <numeric>
#include "3rdparty/libbitcoin/include/bitcoin/bitcoin/formats/base_10.hpp"

using namespace std;
using namespace ufo;

namespace ufoui
{
    QString toString(const ufo::wallet::WalletID& walletID)
    {
        if (walletID != Zero)
        {
            auto id = std::to_string(walletID);
            return QString::fromStdString(id);
        }
        return "";
    }

    QString toString(const ufo::Merkle::Hash& walletID)
    {
        auto id = std::to_string(walletID);
        return QString::fromStdString(id);
    }
    
    QString AmountToString(const Amount& value, Currencies coinType)
    {
        // TODO implement for another currencies
        // TODO implement to support locale
        std::string btc = libbitcoin::satoshi_to_btc(value);
        QString amount = QString::fromStdString(btc);

        QString coinSign;
        switch (coinType)
        {
            case Currencies::UFO:
                //coinSign = QString::fromUtf16(u" \uEAFB");
                coinSign = QString(" UFO");
                break;

            case Currencies::Bitcoin:
                coinSign = QString::fromUtf16(u" \u20BF");
                break;

            case Currencies::Litecoin:
                coinSign = QString::fromUtf16(u" \u0141");
                break;

            case Currencies::Qtum:
                coinSign = QString::fromUtf16(u" \uEAFD");
                break;

            case Currencies::Unknown:
                coinSign = "";
                break;
        }
        return amount + coinSign;
    }

    ufo::Amount StringToAmount(const QString& value)
    {
        // TODO implement to support locale
        ufo::Amount amount = 0;
        libbitcoin::btc_to_satoshi(amount, value.toStdString());
        return amount;
    }

    QString toString(const ufo::Timestamp& ts)
    {
        QDateTime datetime;
        datetime.setTime_t(ts);

        return datetime.toString(Qt::SystemLocaleShortDate);
    }

    Currencies convertSwapCoinToCurrency(wallet::AtomicSwapCoin coin)
    {
        switch (coin)
        {
        case wallet::AtomicSwapCoin::Bitcoin:
            return ufoui::Currencies::Bitcoin;
        case wallet::AtomicSwapCoin::Litecoin:
            return ufoui::Currencies::Litecoin;
        case wallet::AtomicSwapCoin::Qtum:
            return ufoui::Currencies::Qtum;
        case wallet::AtomicSwapCoin::Unknown:
        default:
            return ufoui::Currencies::Unknown;
        }
    }

    Filter::Filter(size_t size)
        : _samples(size, 0.0)
        , _index{0}
        , _is_poor{true}
    {
    }
    
    void Filter::addSample(double value)
    {
        _samples[_index] = value;
        _index = (_index + 1) % _samples.size();
        if (_is_poor)
        {
            _is_poor = _index + 1 < _samples.size();
        }
    }

    double Filter::getAverage() const
    {
        double sum = accumulate(_samples.begin(), _samples.end(), 0.0);
        return sum / (_is_poor ? _index : _samples.size());
    }

    double Filter::getMedian() const
    {
        vector<double> temp(_samples.begin(), _samples.end());
        size_t medianPos = (_is_poor ? _index : temp.size()) / 2;
        nth_element(temp.begin(),
                    temp.begin() + medianPos,
                    _is_poor ? temp.begin() + _index : temp.end());
        return temp[medianPos];
    }

    QDateTime CalculateExpiresTime(ufo::Height currentHeight, ufo::Height expiresHeight)
    {
        auto currentDateTime = QDateTime::currentDateTime();
        QDateTime expiresTime = currentDateTime;

        if (currentHeight <= expiresHeight)
        {
            expiresTime = currentDateTime.addSecs((expiresHeight - currentHeight) * 60);
        }
        else
        {
            auto dateTimeSecs = currentDateTime.toSecsSinceEpoch() - (currentHeight - expiresHeight) * 60;
            expiresTime.setSecsSinceEpoch(dateTimeSecs);
        }

        return expiresTime;
    }

    QString toString(Currencies currency)
    {
        switch(currency)
        {
            case Currencies::UFO: return "ufo";
            case Currencies::Bitcoin: return "btc";
            case Currencies::Litecoin: return "ltc";
            case Currencies::Qtum: return "qtum";
            default: return "unknown";
        }
    }

    std::string toStdString(Currencies currency)
    {
        return toString(currency).toStdString();
    }
}  // namespace ufoui
