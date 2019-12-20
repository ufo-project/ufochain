// Copyright 2019 The UFO Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "swap_tx_object.h"
#include "wallet/swaps/common.h"
#include "viewmodel/qml_globals.h"

using namespace ufo;
using namespace ufo::wallet;

SwapTxObject::SwapTxObject(QObject* parent)
        : TxObject(parent),
          m_isUfoSide(boost::none),
          m_swapCoin(boost::none)
{
}

SwapTxObject::SwapTxObject(const TxDescription& tx, QObject* parent/* = nullptr*/)
        : TxObject(tx, parent),
          m_isUfoSide(m_tx.GetParameter<bool>(TxParameterID::AtomicSwapIsUfoSide)),
          m_swapCoin(m_tx.GetParameter<AtomicSwapCoin>(TxParameterID::AtomicSwapCoin))
{
}

auto SwapTxObject::isUfoSideSwap() const -> bool
{
    if (m_isUfoSide)
    {
        return *m_isUfoSide;
    }
    else return false;
}

auto SwapTxObject::getSwapCoinName() const -> QString
{
    if (m_swapCoin)
    {
        switch (*m_swapCoin)
        {
            case AtomicSwapCoin::Bitcoin:   return toString(ufoui::Currencies::Bitcoin);
            case AtomicSwapCoin::Litecoin:  return toString(ufoui::Currencies::Litecoin);
            case AtomicSwapCoin::Qtum:      return toString(ufoui::Currencies::Qtum);
            case AtomicSwapCoin::Unknown:   return toString(ufoui::Currencies::Unknown);
        }
    }
    return QString("unknown");
}

QString SwapTxObject::getSentAmount() const
{
    if (m_type == TxType::AtomicSwap)
    {
        return getSwapAmount(true);
    }
    return m_tx.m_sender ? getAmount() : "";
}

ufo::Amount SwapTxObject::getSentAmountValue() const
{
    if (m_type == TxType::AtomicSwap)
    {
        return getSwapAmountValue(true);
    }

    return m_tx.m_sender ? m_tx.m_amount : 0;
}

QString SwapTxObject::getReceivedAmount() const
{
    if (m_type == TxType::AtomicSwap)
    {
        return getSwapAmount(false);
    }
    return !m_tx.m_sender ? getAmount() : "";
}

ufo::Amount SwapTxObject::getReceivedAmountValue() const
{
    if (m_type == TxType::AtomicSwap)
    {
        return getSwapAmountValue(false);
    }

    return !m_tx.m_sender ? m_tx.m_amount : 0;
}

QString SwapTxObject::getSwapAmount(bool sent) const
{
    if (!m_isUfoSide)
    {
        return "";
    }

    bool s = sent ? !*m_isUfoSide : *m_isUfoSide;
    if (s)
    {
        auto swapAmount = m_tx.GetParameter<Amount>(TxParameterID::AtomicSwapAmount);
        if (swapAmount)
        {
            return AmountToString(*swapAmount, ufoui::convertSwapCoinToCurrency(*m_swapCoin));
        }
        return "";
    }
    return getAmount();
}

ufo::Amount SwapTxObject::getSwapAmountValue(bool sent) const
{
    if (!m_isUfoSide)
    {
        return 0;
    }

    bool s = sent ? !*m_isUfoSide : *m_isUfoSide;
    if (s)
    {
        auto swapAmount = m_tx.GetParameter<Amount>(TxParameterID::AtomicSwapAmount);
        if (swapAmount)
        {
            return *swapAmount;
        }
        return 0;
    }
    return m_tx.m_amount;
}

QString SwapTxObject::getFeeRate() const
{
    auto feeRate = m_tx.GetParameter<ufo::Amount>(TxParameterID::Fee, *m_isUfoSide ? SubTxIndex::REDEEM_TX : SubTxIndex::LOCK_TX);

    if (feeRate && m_swapCoin)
    {
        QString value = AmountToString(*feeRate, ufoui::Currencies::Unknown);

        QString rateMeasure;
        switch (*m_swapCoin)
        {
        case AtomicSwapCoin::Bitcoin:
            rateMeasure = QMLGlobals::btcFeeRateLabel();
            break;

        case AtomicSwapCoin::Litecoin:
            rateMeasure = QMLGlobals::ltcFeeRateLabel();
            break;

        case AtomicSwapCoin::Qtum:
            rateMeasure = QMLGlobals::qtumFeeRateLabel();
            break;
        
        default:
            break;
        }
        return value + " " + rateMeasure;
    }
    return QString();
}

namespace
{
    template<typename T>
    void copyParameter(TxParameterID id, const TxParameters& source, TxParameters& dest)
    {
        if (auto p = source.GetParameter<T>(id); p)
        {
            dest.SetParameter(id, *p);
        }
    }

    void copyParameter(TxParameterID id, const TxParameters& source, TxParameters& dest, bool inverse = false)
    {
        if (auto p = source.GetParameter<bool>(id); p)
        {
            dest.SetParameter(id, inverse ? !*p : *p);
        }
    }

    template<size_t V>
    QString getSwapCoinTxId(const TxParameters& source)
    {
        if (auto res = source.GetParameter<std::string>(TxParameterID::AtomicSwapExternalTxID, V))
        {
            return QString(res->c_str());
        }
        else return QString();
    }
    
    template<size_t V>
    QString getSwapCoinTxConfirmations(const TxParameters& source)
    {
        if (auto res = source.GetParameter<uint32_t>(TxParameterID::Confirmations, V))
        {
            auto n = std::to_string(*res);
            return QString::fromStdString(n);
        }
        else return QString();
    }

    template<size_t V>
    QString getUfoTxKernelId(const TxParameters& source)
    {
        if (auto res = source.GetParameter<Merkle::Hash>(TxParameterID::KernelID, V))
        {
            return QString::fromStdString(to_hex(res->m_pData, res->nBytes));
        }
        else return QString();
    }
}

QString SwapTxObject::getToken() const
{
    if (m_type != TxType::AtomicSwap)
    {
        return "";
    }

    TxParameters tokenParams(m_tx.m_txId);

    auto isInitiator = m_tx.GetParameter<bool>(TxParameterID::IsInitiator);
    if (*isInitiator == false) 
    {
        if (auto p = m_tx.GetParameter<WalletID>(TxParameterID::MyID); p)
        {
            tokenParams.SetParameter(TxParameterID::PeerID, *p);
        }
    }
    else
    {
        copyParameter<WalletID>(TxParameterID::PeerID, m_tx, tokenParams);
    }

    tokenParams.SetParameter(TxParameterID::IsInitiator, true);

    copyParameter(TxParameterID::IsSender, m_tx, tokenParams, !*isInitiator);
    copyParameter(TxParameterID::AtomicSwapIsUfoSide, m_tx, tokenParams, !*isInitiator);

    tokenParams.SetParameter(ufo::wallet::TxParameterID::TransactionType, m_type);
    copyParameter<Height>(TxParameterID::MinHeight, m_tx, tokenParams);
    copyParameter<Height>(TxParameterID::PeerResponseTime, m_tx, tokenParams);
    copyParameter<Timestamp>(TxParameterID::CreateTime, m_tx, tokenParams);
    copyParameter<Height>(TxParameterID::Lifetime, m_tx, tokenParams);

    copyParameter<Amount>(TxParameterID::Amount, m_tx, tokenParams);
    copyParameter<Amount>(TxParameterID::AtomicSwapAmount, m_tx, tokenParams);
    copyParameter<AtomicSwapCoin>(TxParameterID::AtomicSwapCoin, m_tx, tokenParams);

    return QString::fromStdString(std::to_string(tokenParams));
}

bool SwapTxObject::isProofReceived() const
{
    Height proofHeight;
    if (m_tx.GetParameter(TxParameterID::KernelProofHeight, proofHeight, SubTxIndex::UFO_LOCK_TX))
    {
        return true;
    }
    else return false;
}

QString SwapTxObject::getSwapCoinLockTxId() const
{
    return getSwapCoinTxId<SubTxIndex::LOCK_TX>(m_tx);
}

QString SwapTxObject::getSwapCoinRedeemTxId() const
{
    return getSwapCoinTxId<SubTxIndex::REDEEM_TX>(m_tx);
}

QString SwapTxObject::getSwapCoinRefundTxId() const
{
    return getSwapCoinTxId<SubTxIndex::REFUND_TX>(m_tx);
}

QString SwapTxObject::getSwapCoinLockTxConfirmations() const
{
    return getSwapCoinTxConfirmations<SubTxIndex::LOCK_TX>(m_tx);
}

QString SwapTxObject::getSwapCoinRedeemTxConfirmations() const
{
    return getSwapCoinTxConfirmations<SubTxIndex::REDEEM_TX>(m_tx);
}

QString SwapTxObject::getSwapCoinRefundTxConfirmations() const
{
    return getSwapCoinTxConfirmations<SubTxIndex::REFUND_TX>(m_tx);
}

QString SwapTxObject::getUfoLockTxKernelId() const
{
    return getUfoTxKernelId<SubTxIndex::UFO_LOCK_TX>(m_tx);
}

QString SwapTxObject::getUfoRedeemTxKernelId() const
{
    return getUfoTxKernelId<SubTxIndex::REDEEM_TX>(m_tx);
}

QString SwapTxObject::getUfoRefundTxKernelId() const
{
    return getUfoTxKernelId<SubTxIndex::REFUND_TX>(m_tx);
}
