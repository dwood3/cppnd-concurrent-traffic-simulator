#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    //std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    _condition.notify_one(); // notify client after pushing new Vehicle into vector
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto message = _msgQueue->receive();
        //std::cout << "   Message #" << message << " has been removed from the queue" << std::endl;
        if (message == green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    const int LO = 4;
    const int HI = 6;
    float randomTime = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));

    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    auto lastTime = std::chrono::system_clock::now();
    while(true)
    {
        long timeSinceLastTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now()
                                                                                  - lastTime).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if(timeSinceLastTime >= randomTime)
        {
            // toggle phase
            if (_currentPhase == red)
                _currentPhase = green;
            else if (_currentPhase == green)
                _currentPhase = red;

            auto msg = _currentPhase;
            auto sentCondition = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _msgQueue, std::move(msg));
            sentCondition.wait();
            randomTime = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
            lastTime = std::chrono::system_clock::now();
        }
    }
}
