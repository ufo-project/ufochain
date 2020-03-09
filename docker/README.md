# Docker usage

## usage

docker run -tid ufochain-ubuntu-18.04:latest bash -c "/root/ufochain/pow/minepool_client --server mainnet-pool01.ufo.link:8080 --miner-address $YOUR_WALLET_ADDRESS --worker-name $YOUR_WORKER_NAME"

## notice

+ if you build ufo in docker, please give enough memory for docker runtime
