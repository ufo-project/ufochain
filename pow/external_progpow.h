

bool progpow_hash(uint32_t block_number, const std::string& in, uint64_t nonce, std::string& out_result, std::string& out_mixhash);
	

bool progpow_hash_verify(uint32_t block_number, const std::string& in, const std::string mix, uint64_t nonce, const std::string target);

