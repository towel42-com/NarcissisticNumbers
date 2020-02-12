#ifndef __NARCISSISTICNUMCALCULATOR_H
#define __NARCISSISTICNUMCALCULATOR_H
#include <algorithm>
#include <list>
#include <chrono>
#include <mutex>
#include <future>

class CNarcissisticNumCalculator
{
public:
    bool parse(int argc, char** argv);
    void run();
private:
    static int getInt(int& ii, int argc, char** argv, const char* switchName, bool& aOK);
    void dumpNumbers(const std::list< int64_t >& numbers) const;
    void report() const;
    void reportFindings() const;
    std::pair< bool, bool > checkAndAddValue(int64_t value);
    void findNarcissisticRange(int num, int64_t min, int64_t max);
    void findNarcissisticList(int num, const std::list< int64_t >& values);
    void partition();
    void reportNumThreadsRemaining(std::chrono::system_clock::time_point& prev, bool force = false);

    bool isFinished(std::chrono::system_clock::time_point& prev);
    void addNarcissisticValue(int64_t value);
    
    bool isNarcissistic(int64_t val, int base, bool& aOK);
    

    int fBase{ 10 };
    std::pair< int64_t, int64_t > fRange{ 0, 100000 };
    int fThreadMax{ 100 };
    int fReportSeconds{ 5 };
    std::list< int64_t > fNumbers;
    std::list< std::future< void > > fHandles;

    std::mutex fMutex;
    mutable std::list< int64_t > fNarcissisticNumbers;

    std::pair< std::chrono::system_clock::time_point, std::chrono::system_clock::time_point > fRunTime;
};
#endif
