#include "FCam/TagValue.h"
#include "FCam/Time.h"

#include <iostream>
#include <sstream>

template<typename T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v) {
    out << "[";
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out << ", ";
        out << v[i];
    }
    out << "]";
    return out;
}

template<typename T>
void test(T val) {
    FCam::TagValue t;
    t = val;
    T y = t;
    std::cout << std::endl;
    std::cout.precision(5);
    std::cout << "Input    = " << val << std::endl;
    std::cout << "TagValue = " << t << std::endl;
    std::cout.precision(5);
    std::cout << "Output   = " << y << std::endl;
}

template<typename S, typename T>
void testCast(S val) {
    FCam::TagValue t = val;
    T casted = t;
    std::cout << "Input = " << val << std::endl;
    std::cout << "TagValue = " << t << std::endl;
    std::cout << "Output = " << casted << std::endl;
}

template<typename S>
void testAllCasts(S val) {
    testCast<S, int>(val);
    testCast<S, float>(val);
    testCast<S, double>(val);
    testCast<S, FCam::Time>(val);
    testCast<S, std::string>(val);
    testCast<S, std::vector<int> >(val);
    testCast<S, std::vector<float> >(val);
    testCast<S, std::vector<double> >(val);
    testCast<S, std::vector<FCam::Time> >(val);
    testCast<S, std::vector<std::string> >(val);
}

void testParse(std::string str) {
    std::istringstream isstr(str);
    FCam::TagValue t;
    isstr >> t;
    std::string theRest;
    isstr >> theRest;
    std::cout << str << " -> " << t << " + " << theRest << std::endl;
}

template<typename T>
void testBinary(T val) {
    FCam::TagValue t;
    t = val;
    std::string blob = t.toBlob();
    FCam::TagValue newt = FCam::TagValue::fromString(blob);
    T newVal = newt;
    std::cout << t << " -> " << newt << std::endl;
    if (newVal != val) {
        std::cout << "ERROR! Value did not survive the round trip" << std::endl;
    }
}

int main() {

    std::cout << "Testing null" << std::endl;

    FCam::TagValue none;
    std::cout << none << ", " << none.toString() << std::endl;

    std::cout << std::endl << "Testing scalar types" << std::endl;

    test(42);
    test(42.0f);
    test(42.0);

    std::string str = "Hello, world!";
    test(str);

    FCam::Time time = FCam::Time::now();
    test(time);

    std::string silly(200, ' ');
    for (int i = 0; i < 200; i++) silly[i] = (char)(i+56);
    test(silly);

    std::cout << std::endl << "Testing vector types" << std::endl;
    
    std::vector<int> vi;
    for (int i = 0; i < 5; i++) {
        vi.push_back(rand());
    }
    test(vi);
    
    std::vector<float> vf;
    for (int i = 0; i < 5; i++) {
        vf.push_back(rand()/1000.0f);
    }
    test(vf);    

    std::vector<double> vd;
    for (int i = 0; i < 5; i++) {
        vd.push_back(rand()/1000.0);
    }
    test(vd);    


    std::vector<FCam::Time> vt;
    for (int i = 0; i < 5; i++) {
        usleep(1000);
        vt.push_back(FCam::Time::now() + i*1000000);
    }
    test(vt);    

    std::vector<std::string> vs;
    vs.push_back(std::string("one"));
    vs.push_back(std::string("two"));
    vs.push_back(std::string("three"));
    vs.push_back(std::string("four"));
    vs.push_back(std::string("five"));
    test(vs);    

    std::cout << std::endl << "Testing bad casts" << std::endl;
    
    testAllCasts(42);
    testAllCasts(42.0f);
    testAllCasts(42.0);
    testAllCasts(str);
    testAllCasts(time);
    testAllCasts(vi);
    testAllCasts(vf);
    testAllCasts(vd);
    testAllCasts(vs);
    testAllCasts(vt);
    
    std::cout << std::endl << "Testing good parses" << std::endl;
    testParse("None");
    testParse("123");
    testParse("123.0");
    testParse("123.0f");
    testParse("-1");
    testParse("-1.");
    testParse("-1.0");
    testParse("-1.0f");
    testParse("-1.0e+3");
    testParse("1.0e-3");
    testParse("1.0e-3f");
    testParse("1.0e-30f");
    testParse("5eggs");
    testParse("5.2eggs");
    testParse("5.2e+fggs");
    testParse("\"Hello, world!\"");
    testParse("\"Hello, world! \\\" \\n \n\a\b\t\\a\\b\\t\"");
    FCam::TagValue sillyTag = silly;
    testParse(sillyTag.toString());
    FCam::TagValue currentTime = FCam::Time::now();
    testParse(currentTime.toString());
    testParse("[1,2  , 3 , 5 ] la la la");
    testParse("[  -1.34,2.23423e+4  , 3.9 , 5 ] la la la");
    testParse("[  -1.34f,2.23423e+4  , 3.9 , 5 ] la la la");
    testParse("[\"Hello\", \",\", \"world!\"]");
    testParse("[\"He,,,,llo\", \",\", \"wo]]rld!\"]");
    testParse("[(1,2), (3,4), (123, 43)]");


    std::cout << std::endl << "Testing bad parses" << std::endl;
    testParse("--3");
    testParse("Nonf");
    testParse("[None,None,None]");
    testParse("[3,3.4,3.65]");
    testParse("[3.2,4.6,2.arglebargle]");
    testParse("\"askjlasdjlkdasjlkdas");
    testParse("\"askjlasdjlkdasjlkdas\\");
    testParse("(1.3,5.3)");
    testParse("(1,2,3)");
    testParse("(1,2");

    std::cout << std::endl << "Testing good parses of the binary format" << std::endl;
    testBinary(123);
    testBinary(123.0);
    testBinary(123.0f);
    testBinary(str);
    testBinary(FCam::Time::now());
    testBinary(vi);
    testBinary(vf);
    testBinary(vd);
    testBinary(vs);
    testBinary(vt);

    std::cout << std::endl << "Testing bad parses of the binary format" << std::endl;    
    std::string bad = std::string("b   ") + silly;
    bad[1] = (char)5;
    bad[2] = bad[3] = '\0';
    testParse(bad);
    bad[2] = 'x';
    bad[3] = 'y';
    testParse(bad);

    std::vector<double> v;
    FCam::TagValue bigVec = v;
    std::vector<double> &bv = bigVec;
    bv.resize(100000);
    for (int i = 0; i < 100000; i++) {
        bv[i] = (double)rand();
    }
    FCam::Time t1 = FCam::Time::now();
    FCam::TagValue newVec = bigVec.fromString(bigVec.toString());
    FCam::Time t2 = FCam::Time::now();
    FCam::TagValue newVec2 = bigVec.fromString(bigVec.toBlob());
    FCam::Time t3 = FCam::Time::now();

    std::cout << "Encoding a decoding a huge double array using human readable format: " << (t2-t1) << std::endl;
    std::cout << "Encoding a decoding a huge double array using binary format: " << (t3-t2) << std::endl;
    std::cout << "Speedup: " << ((t2-t1))/(t3-t2) << "x" << std::endl;
    return 0;

}
