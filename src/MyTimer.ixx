module;

export module MyTimer;

// MyTimer and TimerManager are currently disabled (#if 0).
// The module is a placeholder so that any future reference compiles.
#if 0
export class MyTimer
{
public:
    enum Unit { Seconds, Frames };
    MyTimer();

    void setInterval(double sec);
    void start();

    double interval;

    bool isStopped() const;
    double getSeconds() const;

    static void singleShot(double intervalSec, std::function<void()> callback);

private:
    Unit unit;
    int startedOnFrame;
    friend class TimerManager;
};

export class TimerManager {
public:
    static void update();
private:
    static void registerTimer(MyTimer* timer);
    static std::vector<MyTimer*> timers;
    static std::vector<MyTimer*> newTimers;
    static int elapsedFrames;
    friend class MyTimer;
};
#endif
