#include "external_pow.h"
#include "utility/helpers.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "utility/logger.h"

namespace ufo {

class ExternalPOWStub : public IExternalPOW {
public:
    explicit ExternalPOWStub(bool fakeSolver) : _seed(0), _changed(false), _stop(false), _fakeSolver(fakeSolver) {
        ECC::GenRandom(&_seed, 8);
        _thread.start(BIND_THIS_MEMFN(thread_func));
    }

    ~ExternalPOWStub() override {
        stop();
        _thread.join();
        LOG_INFO() << "Done";
    }

private:
    struct Job {
        std::string jobID;
        Merkle::Hash prev;
        Merkle::Hash input;
        Height height;
        Block::PoW pow;
        BlockFound callback;
    };

    void new_job(
        const std::string& jobID,
        const Merkle::Hash& prev,
        const Merkle::Hash& input,
        const Block::PoW& pow,
        const Height& height,
        const BlockFound& callback,
        const CancelCallback& cancelCallback
    ) override
    {
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if (_currentJob.input == input) {
                return;
            }
            _currentJob.jobID = jobID;
            _currentJob.prev = prev;
            _currentJob.input = input;
            _currentJob.pow = pow;
            _currentJob.height = height;
            _currentJob.callback = callback;
            _changed = true;
        }
        _cond.notify_one();
    }

    void get_last_found_block(std::string& jobID, Height& jobHeight, Block::PoW& pow) override {
        std::lock_guard<std::mutex> lk(_mutex);
        jobID = _lastFoundBlockID;
		jobHeight = _lastFoundBlockHeight;
        pow = _lastFoundBlock;
    }

    void stop() override {
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _stop = true;
            _changed = true;
        }
        _cond.notify_one();
    }

    void stop_current() override {
        // TODO do we need it?
    }

    bool get_new_job(Job& job) {
        std::unique_lock<std::mutex> lk(_mutex);
        _cond.wait(lk, [this]() { return _changed.load(); });

        if (_stop) return false;

        _changed = false;
        job = _currentJob;
        job.pow.m_Nonce = ++_seed;
        return true;
    }

    void thread_func() {
        auto SolveFn = &Block::PoW::Solve;

        Job job;
        Merkle::Hash hv;

        auto cancelFn = [this, &job](bool)->bool {
            if (_changed.load()) {
                LOG_INFO() << "job id=" << job.jobID << " cancelled";
                return true;
            }
            return false;
        };

        while (get_new_job(job)) {
            LOG_INFO() << "solving job id=" << job.jobID
                       << " with nonce=" << job.pow.m_Nonce 
                       << " and difficulty=" << job.pow.m_Difficulty 
                       << " and height=" << job.height;

            if (_fakeSolver) {
				/*
                ECC::GenRandom(&job.pow.m_Indices[0], (uint32_t)job.pow.m_Indices.size());
				{
					std::lock_guard<std::mutex> lk(_mutex);
					_lastFoundBlock = job.pow;
					_lastFoundBlockID = job.jobID;
					_lastFoundBlockHeight = job.height;
				}
				job.callback();
				*/

            } else if ( (job.pow.*SolveFn) (job.prev.m_pData, Merkle::Hash::nBytes, job.input.m_pData, Merkle::Hash::nBytes, cancelFn)) {
                {
                    std::lock_guard<std::mutex> lk(_mutex);
                    _lastFoundBlock = job.pow;
                    _lastFoundBlockID = job.jobID;
                    _lastFoundBlockHeight = job.height;
                }
                job.callback();
            }
        }
    }

    Job _currentJob;
    std::string _lastFoundBlockID;
    Height _lastFoundBlockHeight;
    Block::PoW _lastFoundBlock;
    uint64_t _seed;
    std::atomic<bool> _changed;
    bool _stop;
    Thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
    bool _fakeSolver;
};

std::unique_ptr<IExternalPOW> IExternalPOW::create_local_solver(bool fakeSolver) {
    return std::make_unique<ExternalPOWStub>(fakeSolver);
}


class ExternalPOWStub2 : public IExternalPOW2 {
public:
    explicit ExternalPOWStub2(bool fakeSolver) : _seed(0), _never_getjob(true), _changed(false), _stop(false), _fakeSolver(fakeSolver) {
        _thread.start(BIND_THIS_MEMFN(thread_func));
    }

    ~ExternalPOWStub2() override {
        stop();
        _thread.join();
        LOG_INFO() << "Done";
    }

private:
    struct Job {
        std::string jobID;
        Merkle::Hash prev;
        Merkle::Hash input;
        Block::PoW  pow;
        ShareFound callback;
    };

    void new_job(
        const std::string& jobID,
        const Merkle::Hash& prev,
        const Merkle::Hash& input,
        const Difficulty& setDiff,
        const ShareFound& callback,
        const CancelCallback& cancelCallback
    ) override
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_currentJob.input == input) {
            return;
        }
        _currentJob.jobID = jobID;
        _currentJob.prev = prev;
        _currentJob.input = input;
        _currentJob.pow = Block::PoW();
        _currentJob.pow.m_Difficulty = setDiff;
        _currentJob.callback = callback;
        _never_getjob = false;
        _changed = true;
    }

    void set_seed(uint64_t seed) override {
        std::lock_guard<std::mutex> lk(_mutex);
        _seed = seed;
    }

    void get_last_found_share(std::string& jobID, Block::PoW& pow) override {
        std::lock_guard<std::mutex> lk(_mutex);
        jobID = _lastFoundJobID;
        pow = _lastFoundShare;
    }

    void stop() override {
        std::lock_guard<std::mutex> lk(_mutex);
        _stop = true;
        _changed = true;
    }

    void stop_current() override {
        // TODO do we need it?
    }

    bool get_new_job(Job& job) {
        std::unique_lock<std::mutex> lk(_mutex);
       
        if (_stop) return false;

        _changed = false;
        job = _currentJob;
        job.pow.m_Nonce = ++_seed;
        return true;
    }

    void thread_func() {
        auto SolveFn = &Block::PoW::Solve;

        Job job;
        Merkle::Hash hv;

        auto cancelFn = [this, &job](bool)->bool {
            if (_changed.load()) {
                LOG_INFO() << "job id=" << job.jobID << " cancelled";
                return true;
            }
            return false;
        };

        while (!_stop) {
            {
                std::lock_guard<std::mutex> lk(_mutex);
                if (_never_getjob) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                if (_changed) {
                    get_new_job(job);
                }
            }

            LOG_INFO() << "solving job id=" << job.jobID
                << " with nonce=" << job.pow.m_Nonce
                << " and difficulty=" << job.pow.m_Difficulty;

            if (_fakeSolver) {
                ;
            }
            else if ((job.pow.*SolveFn) (job.prev.m_pData, Merkle::Hash::nBytes, job.input.m_pData, Merkle::Hash::nBytes, cancelFn)) {
                {
                    std::lock_guard<std::mutex> lk(_mutex);
                    _lastFoundJobID = job.jobID;
                    _lastFoundShare = job.pow;
                }
                job.callback();
            }
        }
    }

    Job _currentJob;
    std::string _lastFoundJobID;
    Block::PoW _lastFoundShare;
    uint64_t _seed;
    std::atomic<bool> _never_getjob;
    std::atomic<bool> _changed;
    bool _stop;
    Thread _thread;
    std::mutex _mutex;
    bool _fakeSolver;
};

std::unique_ptr<IExternalPOW2> IExternalPOW2::create_local_solver(bool fakeSolver) {
    return std::make_unique<ExternalPOWStub2>(fakeSolver);
}



} //namespace
