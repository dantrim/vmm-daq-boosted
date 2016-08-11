#ifndef EVENT_BUILDER_H
#define EVENT_BUILDER_H

#include <string>
#include <iostream>

class EventBuilder
{
    public :
        EventBuilder();
        virtual ~EventBuilder(){};

        void print_data(std::string msg);
    private :
        
};

#endif
