#include "event_builder.h"

#include <boost/thread.hpp>
#include <iostream>

EventBuilder::EventBuilder()
{
}

void EventBuilder::print_data(std::string msg)
{
    using std::cout;
    using std::endl;

    cout << "EventBuilder::print_data    [" << boost::this_thread::get_id()
            << "]    " << msg << endl;
    return;
}
