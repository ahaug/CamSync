#include <time.h>
#include <algorithm>
#include <FCam/TSQueue.h>

bool done = false; 

void *runAThread(void *arg) {
    FCam::TSQueue<int> *test = (FCam::TSQueue<int> *)arg;
    int counter = 0;
    while (!done) {
        test->push(counter);
        timespec tSleep; 
        tSleep.tv_sec = 0;
        tSleep.tv_nsec = 10e6; // 10e6 ns = 10 ms
        nanosleep(&tSleep, NULL);        
        counter++;
    }
}

void *runBThread(void *arg) {
    FCam::TSQueue<int> *test = (FCam::TSQueue<int> *)arg;
    int counter = 1000000;
    while (!done) {
        test->push(counter);
        timespec tSleep; 
        tSleep.tv_sec = 0;
        tSleep.tv_nsec = 5e6; // 5e6 ns = 5 ms
        nanosleep(&tSleep, NULL);        
        counter++;
    }
}    

int main(int argc, char **argv) {
    
    FCam::TSQueue<int> test;
    
    pthread_t aThread, bThread;

    pthread_create(&aThread, NULL, runAThread, (void *)&test);
    pthread_create(&bThread, NULL, runBThread, (void *)&test);

    for (int i=0; i < 200; i++) {
        int n1 = test.pull();
        int n2 = test.pull();
        printf("\t%d\t%d\n",n1,n2);
    }

    timespec tSleep; 
    tSleep.tv_sec = 1;
    tSleep.tv_nsec = 0;
    nanosleep(&tSleep, NULL);        

    {
        FCam::TSQueue<int>::locking_iterator it = test.begin();
        printf("entries before sort: %d\n", test.size());
        std::sort(it, test.end());
        for(int k=0; it != test.end(); it++, k++) {
            if (k % 4 == 0) printf("\n");
            printf("%d\t", *it);
        }
        printf("entries after sort: %d\n", test.size());
        tSleep.tv_sec = 0;
        tSleep.tv_nsec = 100e6;
        nanosleep(&tSleep, NULL);        
        printf("entries after sort, should still be locked: %d\n", test.size());
    }

    tSleep.tv_sec = 0;
    tSleep.tv_nsec = 100e6;
    nanosleep(&tSleep, NULL);        

    printf("entries after sleeping a bit (should be more): %d\n", test.size());

    done = true;
    pthread_join(aThread, NULL);
    pthread_join(bThread, NULL);
    return 1;
}
