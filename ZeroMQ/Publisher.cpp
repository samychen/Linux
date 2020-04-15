#include "zmq.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include <iostream>

#define within(num) (int) ((float) num * random () / (RAND_MAX + 1.0))


char* topics[] = {
        "order",
        "realplay",
        "advertising",
        "search",
        "authority",
        "log"
};

char* msgs[] = {
        "100 dozens of durex",
        "Kim Kardashianw underwear promotion live",
        "Invisible extreme thin",
        "Aircraft Cup",
        "lee login",
        "20140217T104528.495629|fnen|k3r=testcases/ClockSelection.ttcn3:1426|components.f_copy_logs(testcase_name="
};

int main () {

    //  Prepare our context and publisher
    zmq::context_t context (1);
    zmq::socket_t publisher (context, ZMQ_PUB);
    publisher.bind("tcp://*:9002");
    std::cout << "try to publish msg" << std::endl;
    
    //  Initialize random number generator
    srandom ((unsigned) time (NULL));
    int count = 0;
    while (1) {

        int index = random() % (sizeof(topics) / sizeof(topics[0]));
  
        //  Send message to all subscribers
        zmq::message_t message(128);
        snprintf ((char *) message.data(), 128 , "%s %s , count = %d", topics[index], msgs[index], count);
        std::cout << "send message is : " << (char*)message.data() << std::endl;
        publisher.send(message);
        count ++;
        sleep(1);
    }
    return 0;
}

