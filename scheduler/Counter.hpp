#ifndef _COUNTER_
#define _COUNTER_

#include <map>

typedef std::pair<int, int> _processpair;
/*
 * Count information between every process pair
 */
class CountDouble {
public:
    CountDouble();
    ~CountDouble() {}

    CountDouble& operator= (const CountDouble&);

    int SndCount;
    int RecvCount;

    void debugPrint();

};

typedef std::map<_processpair, CountDouble> CountDoubleMap;
typedef std::map<int, int> WCMap;
typedef std::map<int, int>::iterator WCMapIterator;
typedef std::map<_processpair, CountDouble>::iterator CountDoubleMapIterator;

/*
 * Maintains the CountDouble information 
 * between each process pair
 */

class CountTracker {
public:
     ~CountTracker();

    CountTracker();
    CountTracker& operator= (const CountTracker&);

     CountDouble getCountDouble(int, int);
     int getWCCount(int);
     int getDiffCount(int, int);
     int getSendCount(int, int);
     void updateCount(int, int, bool);
     void addentry(_processpair, CountDouble);
     void debugPrint();
     
  
private:
  CountDoubleMap _countmap;
  WCMap _wcmap;
};


#endif
