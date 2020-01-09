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

#include "external_pow.h"
#include "block_crypt.h"
#include "stratum.h"
#include "p2p/line_protocol.h"
#include "utility/io/tcpstream.h"
#include "utility/io/timer.h"
#include "utility/helpers.h"
#include "x17r/x17r.h"
#include "sstream"
#include <boost/program_options.hpp>

#define LOG_VERBOSE_ENABLED 0
#include "utility/logger.h"

namespace po = boost::program_options;

namespace ufo {

static const unsigned RECONNECT_TIMEOUT = 1000;

class PoolStratumClient : public stratum::ParserCallback {
    std::unique_ptr<IExternalPOW2> _miner;
    io::Reactor& _reactor;
    io::Address _serverAddress;
    std::string _minerAddress;
    std::string _workerName;
    LineProtocol _lineProtocol;
    io::TcpStream::Ptr _connection;
    io::Timer::Ptr _timer;
    std::string _lastJobID;
    Merkle::Hash _lastJobPrev;
    Merkle::Hash _lastJobInput;
    Block::PoW _lastFoundShare;
    bool _tls;
    bool _fakeSolver;
    uint32_t _enonce_len;
    uint64_t _shareSubmitIndex;
    Difficulty _setDifficulty;

public:
    PoolStratumClient(io::Reactor& reactor, const io::Address& serverAddress, std::string minerAddress, std::string workerName) :
        _reactor(reactor),
        _serverAddress(serverAddress),
        _minerAddress(std::move(minerAddress)),
        _workerName(std::move(workerName)),
        _lineProtocol(
            BIND_THIS_MEMFN(on_raw_message),
            BIND_THIS_MEMFN(on_write)
        ),
        _timer(io::Timer::create(_reactor)),
        _lastJobID(""),
        _tls(false),
        _fakeSolver(false),
        _shareSubmitIndex(1000),
        _setDifficulty(ufo::Rules().DA.Difficulty0),
        _enonce_len(0)
    {
        _timer->start(0, false, BIND_THIS_MEMFN(on_reconnect));
        _miner = IExternalPOW2::create_local_solver(false);
    }

private:
    bool on_raw_message(void* data, size_t size) {
        LOG_DEBUG() << "got " << std::string((char*)data, size - 1);
        return stratum::parse_json_msg(data, size, *this);
    }

    bool on_message(const stratum::MiningAuthorizeResult& authorize_result) override {
        if (authorize_result.code < 0) {
            LOG_ERROR() << "mining_authorize_result, login to " << _serverAddress << " failed, try again later";
            return false;
        }

        LOG_DEBUG() << "mining_authorize_result, code=" << authorize_result.code;
        return true;
    }

    bool on_message(const stratum::MiningSubscribeResult& subscribe_result) override {
        if (subscribe_result.code < 0) {
            LOG_ERROR() << "mining_subscribe_result failed, try again later";
            return false;
        }

        std::string enonce = subscribe_result.enonce;
        std::transform(enonce.begin(), enonce.end(), enonce.begin(), ::tolower);

        if (enonce.length() != 4 && enonce.length() != 6 && enonce.length() != 8) {
            LOG_ERROR() << "mining_subscribe_result failed, invalid enonce " << enonce;
            return false;
        }

        for (auto iter = enonce.begin(); iter != enonce.end(); ++iter) {
            if ((*iter) < '0' ||
                ((*iter) > '9' && (*iter) < 'a') ||
                (*iter) > 'f') {
                LOG_ERROR() << "mining_subscribe_result failed, invalid enonce " << enonce;
                return false;
            }
        }

        _enonce_len = enonce.length() / 2;
        _miner->set_enonce(enonce.c_str());

        LOG_DEBUG() << "mining_subscribe_result, code=" << subscribe_result.code << ", enonce=" << enonce;
        return true;
    }

    bool on_message(const stratum::MiningSubmitResult& submit_result) override {
        std::string share_submit_id = submit_result.id;
        if (submit_result.code != IExternalPOW2::ShareFoundResultCode::solution_accepted) {
            LOG_WARNING() << "mining_submit, share_submit_id " << share_submit_id << " was refused, code: " << submit_result.code;
            return true;
        }

        LOG_DEBUG() << "mining_submit, share_submit_id " << share_submit_id << " was accepted";
        return true;
    }

    bool fill_job_info(const stratum::MiningNotify& notify) {
        bool ok = false;
        std::vector<uint8_t> buf = from_hex(notify.prev, &ok);
        if (!ok || buf.size() != 32) return false;
        memcpy(_lastJobPrev.m_pData, buf.data(), 32);
        std::vector<uint8_t> buf2 = from_hex(notify.input, &ok);
        if (!ok || buf2.size() != 32) return false;
        memcpy(_lastJobInput.m_pData, buf2.data(), 32);
        _lastJobID = notify.jobid;
        return true;
    }

    bool on_message(const stratum::MiningNotify& notify) override {
        LOG_INFO() << "mining_notify here: job.id=" << notify.jobid;

        if (!fill_job_info(notify)) return false;

        _miner->new_job(
            _lastJobID, _lastJobPrev, _lastJobInput, _setDifficulty,
            BIND_THIS_MEMFN(on_share_found),
            []() { return false; }
        );
        LOG_INFO() << "new job: jobid " << _lastJobID << ", jobPrev=" << _lastJobPrev << ", jobinput=" << _lastJobInput << ", difficulty=" << _setDifficulty;

        return true;
    }

    bool on_message(const stratum::MiningSetDifficulty& set_difficulty) override {
        arith_uint256 target;
        Difficulty diff_1 = ufo::Rules().DA.Difficulty0;
        diff_1.Unpack(target);

        double d = atof(set_difficulty.difficulty.c_str());
        if (d == 0.0) {
            LOG_ERROR() << "mining_set_difficulty failed, invalid difficulty " << set_difficulty.difficulty;
            return false;
        }

        if (d > 1) {
            target /= int(ceil(d));
        }
        else {
            double dd = 1 / d;
            target *= int(floor(dd));
        }
        _setDifficulty.Pack(target);
        LOG_DEBUG() << "mining_set_difficulty to " << set_difficulty.difficulty << ", difficulty=" << _setDifficulty;
        
        if (_lastJobID != "") {
            _miner->new_job(
                _lastJobID, _lastJobPrev, _lastJobInput, _setDifficulty,
                BIND_THIS_MEMFN(on_share_found),
                []() { return false; }
            );
            LOG_INFO() << "new job(set_difficulty): jobid " << _lastJobID << ", jobPrev=" << _lastJobPrev << ", jobinput=" << _lastJobInput << ", difficulty=" << _setDifficulty;
        }

        return true;
    }

    IExternalPOW2::ShareFoundResult on_share_found() {
        std::string jobID;
        _miner->get_last_found_share(jobID, _lastFoundShare);
        if (jobID != _lastJobID) {
            LOG_INFO() << "solution expired" << TRACE(jobID);
            return IExternalPOW2::solution_expired;
        }

        unsigned char pDataIn[80];
        unsigned char pDataOut[32];

        memset(pDataIn, 0x0, 4);
        memcpy(pDataIn + 4, (unsigned char*)_lastJobPrev.m_pData, _lastJobPrev.nBytes);
        memset(pDataIn + 36, 0x0, 4);

        memcpy(pDataIn + 40, (unsigned char*)_lastJobInput.m_pData, _lastJobInput.nBytes);
        memcpy(pDataIn + 72, (unsigned char*)_lastFoundShare.m_Nonce.m_pData, _lastFoundShare.m_Nonce.nBytes);

        x17r_hash(pDataOut, pDataIn, 80);

        LOG_DEBUG() << "pDataIn=" << to_hex(pDataIn, 80);
        LOG_DEBUG() << "pDataOut=" << to_hex(pDataOut, 32);

        Block::PoW sharePow;
        sharePow.m_Difficulty = _setDifficulty;
        if (!_fakeSolver && !sharePow.IsValid(pDataOut, 32, 0)) {
            LOG_ERROR() << "share is invalid, jobid=" << _lastJobID;
            return IExternalPOW2::solution_rejected;
        }
        LOG_INFO() << "_setDifficulty=" << _setDifficulty;
        LOG_INFO() << "share found jobid=" << _lastJobID;
        LOG_INFO() << "prev=" << _lastJobPrev;
        LOG_INFO() << "jobinput=" << _lastJobInput;
        LOG_INFO() << "nonce=" << _lastFoundShare.m_Nonce;

        // ignore reduplicative calculation in a job
        _miner->reset_seed();

        send_last_found_share();
        return IExternalPOW2::solution_accepted;
    }

    void send_last_found_share() {
        if (!_connection || !_connection->is_connected()) return;
        _shareSubmitIndex += 1;
        std::string submit_id = std::to_string(_shareSubmitIndex);
        
        std::stringstream s;
        s << _lastFoundShare.m_Nonce;
        std::string nonceStr;
        s >> nonceStr;
        nonceStr = nonceStr.substr((size_t)_enonce_len * 2, nonceStr.length());

        stratum::MiningSubmit submit(submit_id, _lastJobID, nonceStr);
        if (!stratum::append_json_msg(_lineProtocol, submit)) {
            LOG_ERROR() << "Internal error";
            _reactor.stop();
            return;
        }
        _lineProtocol.finalize();
    }

    bool on_stratum_error(stratum::ResultCode code) override {
        if (code == stratum::login_failed) {
            LOG_ERROR() << "login to " << _serverAddress << " failed, try again later";
            return false;
        }

        // TODO what to do with other errors
        LOG_ERROR() << "got stratum error: " << code << " " << stratum::get_result_msg(code);
        return true;
    }

    bool on_unsupported_stratum_method(stratum::Method method) override {
        LOG_INFO() << "ignoring unsupported stratum method: " << stratum::get_method_str(method);
        return true;
    }

    void on_write(io::SharedBuffer&& msg) {
        if (_connection) {
            LOG_VERBOSE() << "writing " << std::string((const char*)msg.data, msg.size - 1);
            auto result = _connection->write(msg);
            if (!result) {
                on_disconnected(result.error());
            } else {
                ;
            }
        } else {
            LOG_DEBUG() << "ignoring message, no connection";
        }
    }

    void on_disconnected(io::ErrorCode error) {
        LOG_INFO() << "disconnected, error=" << io::error_str(error) << ", rescheduling";
        _connection.reset();
        // cancel original mining job
        LOG_INFO() << "stop original mining job and try to reconnect";
        _miner->stop_current();
        // try to reconnect
        _timer->start(RECONNECT_TIMEOUT, false, BIND_THIS_MEMFN(on_reconnect));
    }

    void on_reconnect() {
        LOG_INFO() << "connecting to " << _serverAddress;
        if (!_reactor.tcp_connect(_serverAddress, 1, BIND_THIS_MEMFN(on_connected), 10000, _tls)) {
            LOG_ERROR() << "connect attempt failed, rescheduling";
            _timer->start(RECONNECT_TIMEOUT, false, BIND_THIS_MEMFN(on_reconnect));
        }
    }

    void on_connected(uint64_t, io::TcpStream::Ptr&& newStream, io::ErrorCode errorCode) {
        if (errorCode != 0) {
            on_disconnected(errorCode);
            return;
        }

        LOG_INFO() << "connected to " << _serverAddress;
        _connection = std::move(newStream);
        _connection->enable_keepalive(2);
        _connection->enable_read(BIND_THIS_MEMFN(on_stream_data));

        if (!stratum::append_json_msg(_lineProtocol, stratum::MiningSubscribe("1"))) {
            LOG_ERROR() << "Internal error";
            _reactor.stop();
        }
        _lineProtocol.finalize();

        if (!stratum::append_json_msg(_lineProtocol, stratum::MiningAuthorize("1", _minerAddress, _workerName))) {
            LOG_ERROR() << "Internal error";
            _reactor.stop();
        }
        _lineProtocol.finalize();
    }

    bool on_stream_data(io::ErrorCode errorCode, void* data, size_t size) {
        if (errorCode != 0) {
            on_disconnected(errorCode);
            return false;
        }
        if (!_lineProtocol.new_data_from_stream(data, size)) {
            LOG_ERROR() << "closing connection";
            _reactor.stop();
            return false;
        }
        return true;
    }
};

} //namespace

struct Options {
    std::string serverAddress;
    std::string minerAddress;
    std::string workerName;
    int logLevel=LOG_LEVEL_DEBUG;
    unsigned logRotationPeriod = 3*60*60*1000; // 3 hours
};

static bool parse_cmdline(int argc, char* argv[], Options& o);

int main(int argc, char* argv[]) {
    using namespace ufo;

    Options options;
    if (!parse_cmdline(argc, argv, options)) {
        return 1;
    }
    std::string logFilePrefix("minepool_client_");
    logFilePrefix += std::to_string(uv_os_getpid());
    logFilePrefix += "_";
    auto logger = Logger::create(LOG_LEVEL_INFO, options.logLevel, options.logLevel, logFilePrefix, "logs");
    int retCode = 0;
    try {
        io::Reactor::Ptr reactor = io::Reactor::create();
        io::Address connectTo;
        if (!connectTo.resolve(options.serverAddress.c_str())) {
            throw std::runtime_error(std::string("cannot resolve server address ") + options.serverAddress);
        }
        io::Reactor::Scope scope(*reactor);
        io::Reactor::GracefulIntHandler gih(*reactor);
        io::Timer::Ptr logRotateTimer = io::Timer::create(*reactor);
        logRotateTimer->start(
            options.logRotationPeriod, true, []() { Logger::get()->rotate(); }
        );
        PoolStratumClient client(*reactor, connectTo, options.minerAddress, options.workerName);
        reactor->run();
        LOG_INFO() << "stopping...";
    } catch (const std::exception& e) {
        LOG_ERROR() << "EXCEPTION: " << e.what();
        retCode = 255;
    } catch (...) {
        LOG_ERROR() << "NON_STD EXCEPTION";
        retCode = 255;
    }
    return retCode;
}

bool parse_cmdline(int argc, char* argv[], Options& o) {

    po::options_description cliOptions("Remote miner options");
    cliOptions.add_options()
    ("help", "list of all options")
    ("server", po::value<std::string>(&o.serverAddress)->required(), "server address")
    ("miner-address", po::value<std::string>(&o.minerAddress)->required(), "miner address")
    ("worker-name", po::value<std::string>(&o.workerName)->required(), "worker name")
    ;

#ifdef NDEBUG
    o.logLevel = LOG_LEVEL_DEBUG;
#else
#if LOG_VERBOSE_ENABLED
    o.logLevel = LOG_LEVEL_VERBOSE;
#else
    o.logLevel = LOG_LEVEL_DEBUG;
#endif
#endif

    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv) // value stored first is preferred
                  .options(cliOptions)
                  .style(po::command_line_style::default_style ^ po::command_line_style::allow_guessing)
                  .run(), vm);

        if (vm.count("help")) {
            std::cout << cliOptions << std::endl;
            return false;
        }

        vm.notify();

        return true;
    } catch (const po::error& ex) {
        std::cerr << ex.what() << "\n" << cliOptions;
    } catch (const std::exception& ex) {
        std::cerr << ex.what();
    } catch (...) {
        std::cerr << "NON_STD EXCEPTION";
    }

    return false;
}
