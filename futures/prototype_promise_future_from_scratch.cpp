#include<iostream>
#include<exception>
#include<random>
#include<optional>
#include<mutex>
#include<condition_variable>
#include <stdexcept>
#include<thread>
#include<chrono>
#include<memory>



template<typename T>
class SharedStateBaseline{
    private:
    std::optional<T> value_;
    bool ready_ = false;
    mutable std::mutex stateMutex_;
    std::condition_variable stateCondition_;


    public:

    void setValue(T value){
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            value_.emplace(std::move(value));
            ready_ = true;
        }
        stateCondition_.notify_one();
    }

    T get()
    {
        std::unique_lock<std::mutex> lock(stateMutex_);
        stateCondition_.wait(lock, [this]{return ready_;});
        return std::move(value_.value());
    }
};



template<typename T>
class Promise;

template<typename T>
class Future;

template<typename T>
class SharedState{

    public:
        enum class Status
        {
            pending,
            value,
            exception,
            broken
        };

        void setValue(T value){
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if(status_ != Status::pending)
                    throw std::logic_error("Result alredy published!");
                value_.emplace(std::move(value));
                status_ = Status::value;
            }
            cv_.notify_all();
        }

        void abortPromise(){
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if(status_ !=Status::pending) return;

                status_ = Status::broken;
            }
            cv_.notify_all();
        }

        void wait(){
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]{return status_ !=Status::pending; });
        }

        void setException(std::exception_ptr error)
        {
            if (!error)
                throw std::invalid_argument("Null exception");

            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (status_ != Status::pending)
                    throw std::logic_error("Result already published");

                exception_ = std::move(error);
                status_ = Status::exception;
            }

            cv_.notify_all();
        }

        T get(){

            std::unique_lock<std::mutex> lock(mutex_);

            cv_.wait(lock, [this] {
                return status_ != Status::pending;
            });

            if (status_ == Status::broken)
                throw std::runtime_error("Broken promise");

            if (consumed_)
                throw std::logic_error("Result already consumed");
            consumed_ = true;

            if (status_ == Status::exception)
                std::rethrow_exception(exception_);

            return std::move(value_.value());
        }

    private:
        std::mutex mutex_;
        std::condition_variable cv_;

        std::optional<T> value_;
        Status status_ = Status::pending;
        bool consumed_ = false;
        bool future_created_ = false;
        std::exception_ptr exception_;

        friend class Promise<T>;
        friend class Future<T>;

};

template<typename T>
class Future{

    public:
        Future(const Future&) = delete;
        Future& operator=(const Future&) = delete;

        Future(Future&&) noexcept = default;
        Future& operator=(Future&&) noexcept = default;
        explicit Future(std::shared_ptr<SharedState<T>> state) : sharedState_(std::move(state))
        {
        }

        void wait()
        {
            if (!sharedState_)
                throw std::logic_error("Invalid future");

            sharedState_->wait();
        }

        T get()
        {
            if (!sharedState_)
                throw std::logic_error("Invalid future");

            auto state = std::move(sharedState_);
            return state->get();
        }

    private:
  
        std::shared_ptr<SharedState<T>> sharedState_;
        friend class SharedState<T>;

};

template<typename T>
class Promise{
    public:
    Promise()
        : sharedState_(std::make_shared<SharedState<T>>())
    {
    }
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Promise(Promise&& other) noexcept 
     :sharedState_(std::move(other.sharedState_))
     {}

    
    Promise& operator=(Promise&& other) noexcept{
        if(this != other){
            abandon();
            this->sharedState_ = std::move(other.sharedState_);
        }
        return *this;
    }
       

    ~Promise(){
       abandon();
    }



    void setValue(T value){
        if(!sharedState_)
            throw  std::logic_error("Invalide promise");

        sharedState_->setValue(std::move(value));

        }
    

    Future<T> get_future()
    {
        if (!sharedState_)
            throw std::logic_error("Invalid promise");

        {
            std::lock_guard<std::mutex> lock(sharedState_->mutex_);

            if (sharedState_->future_created_)
                throw std::logic_error("Future already retrieved");

            sharedState_->future_created_ = true;
        }

        return Future<T>(sharedState_);
    }

    void setException(std::exception_ptr error)
    {
        if (!sharedState_)
            throw std::logic_error("Invalid promise");

        sharedState_->setException(std::move(error));
    }

    private:
        void abandon() noexcept
        {
            if (sharedState_)
                sharedState_->abortPromise();

            sharedState_.reset();
        }
        std::shared_ptr<SharedState<T>> sharedState_;
};

int main(){

    SharedStateBaseline<int> state_;

    std::thread producer ([&state_]{
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist_(10, 1000);
        state_.setValue(dist_(gen));
    });

    int result = state_.get();

    std::cout << "Consumer get result: " << result << std::endl;
 
    producer.join();


    Promise<int> promise;
    Future<int> future = promise.get_future();

    std::thread producer_(
        [p = std::move(promise)]() mutable {
            std::this_thread::sleep_for(
                std::chrono::seconds(2)
            );

            std::cout << "Publishing result...\n";
            p.setValue(42);
        }
    );

    std::cout << "Waiting result...\n";

    int result_ = future.get();

    std::cout << "Result : " << result_ << '\n';

    producer_.join();


    // test of catching exception
    Promise<int> promise_exc;
    Future<int> future_exc = promise_exc.get_future();

    std::thread producer_exc(
        [p = std::move(promise_exc)]() mutable
        {
            try
            {
                throw std::runtime_error("Computation failed!");
            }
            catch (...)
            {
                p.setException(std::current_exception());
            }
        }
    );

    try
    {
        int result_exc = future_exc.get();
        std::cout << result_exc << '\n';
    }
    catch (const std::exception& e)
    {
        std::cout << "Recieved error: " << e.what() << '\n';
    }

    producer_exc.join();

    return 0;
}