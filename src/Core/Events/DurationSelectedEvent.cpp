#include "DurationSelectedEvent.h"

#include "Global.h"

DurationSelectedEvent::DurationSelectedEvent(double selected, double current, double fps)
    : selected(selected), current (current), fps (fps)
{
}

double DurationSelectedEvent::getSelected() const
{
    return this->selected;
}
double DurationSelectedEvent::getCurrent() const
{
    return this->current;
}
double DurationSelectedEvent::getFPS() const
{
    return this->fps;
}
