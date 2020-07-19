#pragma once

#include "Shared.h"

#include <QtCore/QString>
#include <QtCore/QTime>

class COMMON_EXPORT Timecode
{
    public:
        static QString fromTime(const QTime& time, bool useDropFrameNotation);
        static QString fromTime(double time, double fps, bool useDropFrameNotation);
        static double toTime(QString timecode, double fps);
        static QString fromFrames(double NumOfFrames, double fps, bool useDropFrameNotation);
        static double toFrames(QString Time, double fps);
    private:
        Timecode() {}
};
