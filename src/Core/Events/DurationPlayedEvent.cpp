#include "DurationPlayedEvent.h"

#include "Global.h"

DurationPlayedEvent::DurationPlayedEvent(double current, double fps, int channel, int layer)
    : current (current), fps (fps), channel (channel), layer (layer)
{
}

double DurationPlayedEvent::getCurrent() const
{
    return this->current;
}
double DurationPlayedEvent::getFPS() const
{
    return this->fps;
}
int DurationPlayedEvent::getChannel() const
{
    return this->channel;
}
int DurationPlayedEvent::getLayer() const
{
    return this->layer;
}
