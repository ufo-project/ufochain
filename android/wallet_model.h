// Copyright 2018 The Ufo Team
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

#include "wallet/wallet_client.h"

class WalletModel
    : public ufo::wallet::WalletClient
{
public:

    using Ptr = std::shared_ptr<WalletModel>;

    WalletModel(ufo::wallet::IWalletDB::Ptr walletDB, ufo::wallet::IPrivateKeyKeeper::Ptr keyKeeper, const std::string& nodeAddr, ufo::io::Reactor::Ptr reactor);
    ~WalletModel() override;

private:
    void onStatus(const ufo::wallet::WalletStatus& status) override;
    void onTxStatus(ufo::wallet::ChangeAction, const std::vector<ufo::wallet::TxDescription>& items) override;
    void onSyncProgressUpdated(int done, int total) override;
    void onChangeCalculated(ufo::Amount change) override;
    void onAllUtxoChanged(const std::vector<ufo::wallet::Coin>& utxos) override;
    void onAddresses(bool own, const std::vector<ufo::wallet::WalletAddress>& addrs) override;
    void onSwapOffersChanged(ufo::wallet::ChangeAction action, const std::vector<ufo::wallet::SwapOffer>& offers) override;
    void onGeneratedNewAddress(const ufo::wallet::WalletAddress& walletAddr) override;
    void onNewAddressFailed() override;
    void onChangeCurrentWalletIDs(ufo::wallet::WalletID senderID, ufo::wallet::WalletID receiverID) override;
    void onNodeConnectionChanged(bool isNodeConnected) override;
    void onWalletError(ufo::wallet::ErrorType error) override;
    void FailedToStartWallet() override;
    void onSendMoneyVerified() override;
    void onCantSendToExpired() override;
    void onPaymentProofExported(const ufo::wallet::TxID& txID, const ufo::ByteBuffer& proof) override;
    void onCoinsByTx(const std::vector<ufo::wallet::Coin>& coins) override;
    void onAddressChecked(const std::string& addr, bool isValid) override;
    void onImportRecoveryProgress(uint64_t done, uint64_t total) override;
    void onNoDeviceConnected() override {}
    void onShowKeyKeeperMessage() override {}
    void onHideKeyKeeperMessage() override {}
    void onShowKeyKeeperError(const std::string&) override {}
};