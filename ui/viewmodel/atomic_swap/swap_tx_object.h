// Copyright 2019 The Ufo Team
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
#pragma once

#include "viewmodel/wallet/tx_object.h"

class SwapTxObject : public TxObject
{
    Q_OBJECT

public:
    SwapTxObject(QObject* parent = nullptr);
    SwapTxObject(const ufo::wallet::TxDescription& tx, QObject* parent = nullptr);

    auto getSentAmount() const -> QString;
    auto getSentAmountValue() const -> ufo::Amount;
    auto getReceivedAmount() const -> QString;
    auto getReceivedAmountValue() const -> ufo::Amount;
    auto getToken() const -> QString;
    auto getSwapCoinLockTxId() const -> QString;
    auto getSwapCoinLockTxConfirmations() const -> QString;
    auto getSwapCoinRedeemTxId() const -> QString;
    auto getSwapCoinRedeemTxConfirmations() const -> QString;
    auto getSwapCoinRefundTxId() const -> QString;
    auto getSwapCoinRefundTxConfirmations() const -> QString;
    auto getUfoLockTxKernelId() const -> QString;
    auto getUfoRedeemTxKernelId() const -> QString;
    auto getUfoRefundTxKernelId() const -> QString;
    auto getSwapCoinName() const -> QString;
    auto getFeeRate() const -> QString;

    bool isProofReceived() const;
    bool isUfoSideSwap() const;

signals:

private:
    auto getSwapAmountValue(bool sent) const -> ufo::Amount;
    auto getSwapAmount(bool sent) const -> QString;

    boost::optional<bool> m_isUfoSide;
    boost::optional<ufo::wallet::AtomicSwapCoin> m_swapCoin;
};
