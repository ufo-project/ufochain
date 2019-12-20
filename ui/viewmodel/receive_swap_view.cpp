// Copyright 2018 The UFO Team
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

#include "receive_swap_view.h"
#include "ui_helpers.h"
#include "model/app_model.h"
#include "wallet/swaps/common.h"
#include "wallet/swaps/swap_transaction.h"
#include "wallet/bitcoin/bitcoin_side.h"
#include "wallet/litecoin/litecoin_side.h"
#include "wallet/qtum/qtum_side.h"
#include <QClipboard>
#include "qml_globals.h"

namespace {
    enum {
        OfferExpires12h = 0,
        OfferExpires6h  = 1
    };

    uint16_t GetHourCount(int offerExpires)
    {
        switch (offerExpires)
        {
        case OfferExpires12h:
            return 12;
        case OfferExpires6h:
            return 6;
        default:
        {
            assert(false && "Unexpected value!");
            return 0;
        }
        }
    }

    ufo::Height GetBlockCount(int offerExpires)
    {
        return GetHourCount(offerExpires) * 60;
    }

    // TODO: remove after tests
    ufo::Height GetTestBlockCount(int offerExpires)
    {
        switch (offerExpires)
        {
        case OfferExpires12h:
            return 20;
        case OfferExpires6h:
            return 10;
        default:
        {
            assert(false && "Unexpected value!");
            return 0;
        }
        }
    }
}

ReceiveSwapViewModel::ReceiveSwapViewModel()
    : _amountToReceiveGrothes(0)
    , _amountSentGrothes(0)
    , _receiveFeeGrothes(0)
    , _sentFeeGrothes(0)
    , _receiveCurrency(Currency::CurrUfo)
    , _sentCurrency(Currency::CurrBtc)
    , _offerExpires(OfferExpires12h)
    , _walletModel(*AppModel::getInstance().getWallet())
    , _txParameters(ufo::wallet::CreateSwapParameters()
        .SetParameter(ufo::wallet::TxParameterID::AtomicSwapCoin, ufo::wallet::AtomicSwapCoin::Bitcoin)
        .SetParameter(ufo::wallet::TxParameterID::AtomicSwapIsUfoSide, true)
        .SetParameter(ufo::wallet::TxParameterID::IsInitiator, true))
{
    connect(&_walletModel, &WalletModel::generatedNewAddress, this, &ReceiveSwapViewModel::onGeneratedNewAddress);
    connect(&_walletModel, SIGNAL(newAddressFailed()), this, SIGNAL(newAddressFailed()));
    connect(&_walletModel, &WalletModel::stateIDChanged, this, &ReceiveSwapViewModel::updateTransactionToken);

    generateNewAddress();

    _walletModel.getAsync()->getWalletStatus();
    updateTransactionToken();
}

void ReceiveSwapViewModel::onGeneratedNewAddress(const ufo::wallet::WalletAddress& addr)
{
    _receiverAddress = addr;
    emit receiverAddressChanged();
    updateTransactionToken();
}

QString ReceiveSwapViewModel::getAmountToReceive() const
{
    return ufoui::AmountToString(_amountToReceiveGrothes);
}

void ReceiveSwapViewModel::setAmountToReceive(QString value)
{
    auto amount = ufoui::StringToAmount(value);
    if (amount != _amountToReceiveGrothes)
    {
        _amountToReceiveGrothes = amount;
        emit amountToReceiveChanged();
        updateTransactionToken();
    }
}

QString ReceiveSwapViewModel::getAmountSent() const
{
    return ufoui::AmountToString(_amountSentGrothes);
}

unsigned int ReceiveSwapViewModel::getReceiveFee() const
{
    return _receiveFeeGrothes;
}

void ReceiveSwapViewModel::setAmountSent(QString value)
{
    auto amount = ufoui::StringToAmount(value);
    if (amount != _amountSentGrothes)
    {
        _amountSentGrothes = amount;
        emit amountSentChanged();
        updateTransactionToken();
    }
}

unsigned int ReceiveSwapViewModel::getSentFee() const
{
    return _sentFeeGrothes;
}

void ReceiveSwapViewModel::setSentFee(unsigned int value)
{
    if (value != _sentFeeGrothes)
    {
        _sentFeeGrothes = value;
        emit sentFeeChanged();
    }
}

Currency ReceiveSwapViewModel::getReceiveCurrency() const
{
    return _receiveCurrency;
}

void ReceiveSwapViewModel::setReceiveCurrency(Currency value)
{
    assert(value > Currency::CurrStart && value < Currency::CurrEnd);

    if (value != _receiveCurrency)
    {
        _receiveCurrency = value;
        emit receiveCurrencyChanged();
        updateTransactionToken();
    }
}

Currency ReceiveSwapViewModel::getSentCurrency() const
{
    return _sentCurrency;
}

void ReceiveSwapViewModel::setSentCurrency(Currency value)
{
    assert(value > Currency::CurrStart && value < Currency::CurrEnd);

    if (value != _sentCurrency)
    {
        _sentCurrency = value;
        emit sentCurrencyChanged();
        updateTransactionToken();
    }
}

void ReceiveSwapViewModel::setReceiveFee(unsigned int value)
{
    if (value != _receiveFeeGrothes)
    {
        _receiveFeeGrothes = value;
        emit receiveFeeChanged();
    }
}

int ReceiveSwapViewModel::getOfferExpires() const
{
    return _offerExpires;
}

void ReceiveSwapViewModel::setOfferExpires(int value)
{
    if (value != _offerExpires)
    {
        _offerExpires = value;
        emit offerExpiresChanged();
        updateTransactionToken();
    }
}

QString ReceiveSwapViewModel::getReceiverAddress() const
{
    return ufoui::toString(_receiverAddress.m_walletID);
}

void ReceiveSwapViewModel::generateNewAddress()
{
    _receiverAddress = {};
    emit receiverAddressChanged();

    setAddressComment("");
    _walletModel.getAsync()->generateNewAddress();
}

void ReceiveSwapViewModel::setTransactionToken(const QString& value)
{
    if (_token != value)
    {
        _token = value;
        emit transactionTokenChanged();
    }
}

QString ReceiveSwapViewModel::getTransactionToken() const
{
    return _token;
}

QString ReceiveSwapViewModel::getAddressComment() const
{
    return _addressComment;
}

void ReceiveSwapViewModel::setAddressComment(const QString& value)
{
    auto trimmed = value.trimmed();
    if (_addressComment != trimmed)
    {
        _addressComment = trimmed;
        emit addressCommentChanged();
        emit commentValidChanged();
    }
}

bool ReceiveSwapViewModel::getCommentValid() const
{
    return !_walletModel.isAddressWithCommentExist(_addressComment.toStdString());
}

bool ReceiveSwapViewModel::isEnough() const
{
    if (_amountSentGrothes == 0)
        return true;

    switch (_sentCurrency)
    {
    case Currency::CurrUfo:
    {
        auto total = _amountSentGrothes + _sentFeeGrothes;
        return _walletModel.getAvailable() >= total;
    }
    case Currency::CurrBtc:
    {
        // TODO sentFee is fee rate. should be corrected
        ufo::Amount total = _amountSentGrothes + _sentFeeGrothes;
        return AppModel::getInstance().getBitcoinClient()->getAvailable() > total;
    }
    case Currency::CurrLtc:
    {
        ufo::Amount total = _amountSentGrothes + _sentFeeGrothes;
        return AppModel::getInstance().getLitecoinClient()->getAvailable() > total;
    }
    case Currency::CurrQtum:
    {
        ufo::Amount total = _amountSentGrothes + _sentFeeGrothes;
        return AppModel::getInstance().getQtumClient()->getAvailable() > total;
    }
    default:
    {
        assert(false);
        return true;
    }
    }
}

bool ReceiveSwapViewModel::isGreatThanFee() const
{
    if (_amountSentGrothes == 0)
        return true;

    switch (_sentCurrency)
    {
    case Currency::CurrUfo:
    {
        const auto total = _amountSentGrothes + _sentFeeGrothes;
        return total > QMLGlobals::minFeeUfo();
    }
    case Currency::CurrBtc:
    {
        return ufo::wallet::BitcoinSide::CheckAmount(_amountSentGrothes, _sentFeeGrothes);
    }
    case Currency::CurrLtc:
    {
        return ufo::wallet::LitecoinSide::CheckAmount(_amountSentGrothes, _sentFeeGrothes);
    }
    case Currency::CurrQtum:
    {
        return ufo::wallet::QtumSide::CheckAmount(_amountSentGrothes, _sentFeeGrothes);
    }
    default:
    {
        assert(false);
        return true;
    }
    }
}

void ReceiveSwapViewModel::saveAddress()
{
    using namespace ufo::wallet;

    if (getCommentValid()) {
        _receiverAddress.m_label = _addressComment.toStdString();
        _receiverAddress.m_duration = GetHourCount(_offerExpires) * WalletAddress::AddressExpiration1h;
        _walletModel.getAsync()->saveAddress(_receiverAddress, true);
    }
}

void ReceiveSwapViewModel::startListen()
{
    using namespace ufo::wallet;

    bool isUfoSide = (_sentCurrency == Currency::CurrUfo);
    auto ufoFee = isUfoSide ? _sentFeeGrothes : _receiveFeeGrothes;
    auto swapFee = isUfoSide ? _receiveFeeGrothes : _sentFeeGrothes;
    auto txParameters = ufo::wallet::TxParameters(_txParameters);

    txParameters.DeleteParameter(TxParameterID::PeerID);
    txParameters.SetParameter(TxParameterID::IsInitiator, !*_txParameters.GetParameter<bool>(TxParameterID::IsInitiator));
    txParameters.SetParameter(TxParameterID::MyID, *_txParameters.GetParameter<ufo::wallet::WalletID>(ufo::wallet::TxParameterID::PeerID));
    txParameters.SetParameter(TxParameterID::Fee, ufo::Amount(ufoFee));
    txParameters.SetParameter(TxParameterID::Fee, ufo::Amount(swapFee), isUfoSide ? SubTxIndex::REDEEM_TX : SubTxIndex::LOCK_TX);
    txParameters.SetParameter(TxParameterID::AtomicSwapIsUfoSide, isUfoSide);
    txParameters.SetParameter(TxParameterID::IsSender, isUfoSide);
    if (!_addressComment.isEmpty())
    {
        std::string localComment = _addressComment.toStdString();
        txParameters.SetParameter(TxParameterID::Message, ufo::ByteBuffer(localComment.begin(), localComment.end()));
    }

    _walletModel.getAsync()->startTransaction(std::move(txParameters));
}

void ReceiveSwapViewModel::publishToken()
{
    auto txId = _txParameters.GetTxID();
    auto publisherId = _txParameters.GetParameter<ufo::wallet::WalletID>(ufo::wallet::TxParameterID::PeerID);
    auto coin = _txParameters.GetParameter<ufo::wallet::AtomicSwapCoin>(ufo::wallet::TxParameterID::AtomicSwapCoin);
    if (publisherId && txId && coin)
    {
        ufo::wallet::SwapOffer offer(*txId);
        offer.m_txId = *txId;
        offer.m_publisherId = *publisherId;
        offer.m_status = ufo::wallet::SwapOfferStatus::Pending;
        offer.m_coin = *coin;
        offer.SetTxParameters(_txParameters.Pack());
        
        _walletModel.getAsync()->publishSwapOffer(offer);
    }
}

namespace
{
    ufo::wallet::AtomicSwapCoin convertCurrencyToSwapCoin(Currency currency)
    {
        switch (currency)
        {
        case Currency::CurrBtc:
            return ufo::wallet::AtomicSwapCoin::Bitcoin;
        case Currency::CurrLtc:
            return ufo::wallet::AtomicSwapCoin::Litecoin;
        case Currency::CurrQtum:
            return ufo::wallet::AtomicSwapCoin::Qtum;
        default:
            return ufo::wallet::AtomicSwapCoin::Unknown;
        }
    }
}

void ReceiveSwapViewModel::updateTransactionToken()
{
    emit enoughChanged();
    emit lessThanFeeChanged();
    _txParameters.SetParameter(ufo::wallet::TxParameterID::MinHeight, _walletModel.getCurrentHeight());
    // TODO: uncomment after tests
    // _txParameters.SetParameter(ufo::wallet::TxParameterID::PeerResponseTime, GetBlockCount(_offerExpires));
    // TODO: remove after tests
    _txParameters.SetParameter(ufo::wallet::TxParameterID::PeerResponseTime, GetTestBlockCount(_offerExpires));

    // All parameters sets as if we were on the recipient side (mirrored)
    bool isUfoSide = (_receiveCurrency == Currency::CurrUfo);
    auto swapCoin   = convertCurrencyToSwapCoin(isUfoSide ? _sentCurrency : _receiveCurrency);
    auto ufoAmount = isUfoSide ? _amountToReceiveGrothes : _amountSentGrothes;
    auto swapAmount = isUfoSide ? _amountSentGrothes : _amountToReceiveGrothes;

    _txParameters.SetParameter(ufo::wallet::TxParameterID::AtomicSwapIsUfoSide, isUfoSide);
    _txParameters.SetParameter(ufo::wallet::TxParameterID::Amount, ufoAmount);
    _txParameters.SetParameter(ufo::wallet::TxParameterID::AtomicSwapCoin, swapCoin);
    _txParameters.SetParameter(ufo::wallet::TxParameterID::AtomicSwapAmount, swapAmount);
    _txParameters.SetParameter(ufo::wallet::TxParameterID::PeerID, _receiverAddress.m_walletID);
    _txParameters.SetParameter(ufo::wallet::TxParameterID::IsSender, isUfoSide);

    setTransactionToken(QString::fromStdString(std::to_string(_txParameters)));
}
