#include "Timecode.h"

#include <QtCore/QString>

QString Timecode::fromTime(const QTime& time, bool useDropFrameNotation)
{
    QString result = time.toString("hh:mm:ss").append((useDropFrameNotation == true) ? "." : ":");

    int msec = time.msec() / 10;
    if (msec < 10)
        result.append("0");

    return result.append(QString("%1").arg(msec));
}

// convert time in seconds -> String Timecode
QString Timecode::fromTime(double time, double fps, bool useDropFrameNotation)
{
    int hour;
    int minutes;
    int seconds;
    int frames;

    hour = (int)(time / 3600);
    minutes = (int)((time - hour * 3600) / 60);
    seconds = (int)(time - hour * 3600 - minutes * 60);
    frames = (int)((time - hour * 3600 - minutes * 60 - seconds) * fps);

    return QString("%1:%2:%3%4%5").arg(hour, 2, 10, QChar('0'))
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0'))
                                .arg(useDropFrameNotation ? "." : ":")
                                .arg(frames, 2, 10, QChar('0'));
}

//convert Timecode String to time in seconds
double Timecode::toTime(QString timecode, double fps)
{
    double time = 0;
    if (timecode.length() == 11)
    {
    time =timecode[0].digitValue()*10*60*60 + timecode[1].digitValue()*60*60 + timecode[3].digitValue()*10*60 + timecode[4].digitValue()*60 + timecode[6].digitValue()*10 + timecode[7].digitValue() + (timecode[9].digitValue()*10 + timecode[10].digitValue())/fps;
    }
    return time;
}

// convert time in frames -> String Timecode
QString Timecode::fromFrames(double NumOfFrames, double fps, bool useDropFrameNotation)
{
    int hour;
    int minutes;
    int seconds;
    int frames;

    hour = (int)(NumOfFrames / (3600*fps));
    minutes = (int)((NumOfFrames/fps - hour * 3600) / 60);
    seconds = (int)(NumOfFrames/fps - hour * 3600 - minutes * 60);
    frames = (int)(NumOfFrames - hour * 3600*fps - minutes * 60*fps - seconds*fps);


    return QString("%1:%2:%3%4%5").arg(hour, 2, 10, QChar('0'))
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0'))
                                .arg(useDropFrameNotation ? "." : ":")
                                .arg(frames, 2, 10, QChar('0'));
}

double Timecode::toFrames(QString Time, double fps)
{
    double TCfrm = 0;
    if (Time.length() == 11)
    {
    TCfrm =Time[0].digitValue()*10*60*60*fps + Time[1].digitValue()*60*60*fps + Time[3].digitValue()*10*60*fps + Time[4].digitValue()*60*fps + Time[6].digitValue()*10*fps + Time[7].digitValue()*fps + Time[9].digitValue()*10 + Time[10].digitValue();
    }
    return TCfrm;
}
