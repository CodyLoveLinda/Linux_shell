#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/sysinfo.h>
#include<time.h>
#define __USE_GNU
#include<sched.h>
#include<ctype.h>
#include<string.h>
#define COUNT 300000
#define MILLION 1000000L
#define NANOSECOND 1000
void test_func()
{
    int i = 0;
    unsigned long long result = 0;;
    for(i = 0; i<8000 ;i++)
    {
        result += 2;
    }
}
int main(int argc,char* argv[])
{
    int i;
    struct timespec sleeptm;
    long interval;
    struct timeval tend,tstart;
    struct tm lcltime = {0};
    struct sched_param param;
    int ret = 0;
    if(argc != 3)
    {
        fprintf(stderr,"usage:./test sched_method sched_priority\n");
        return -1;
    }
    cpu_set_t mask ;
    CPU_ZERO(&mask);
    CPU_SET(1,&mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) 
    { 
        printf("warning: could not set CPU affinity, continuing...\n");
    } 
    int sched_method = atoi(argv[1]);
    int sched_priority = atoi(argv[2]);
    /*    if(sched_method > 2 || sched_method < 0)
        {
        fprintf(stderr,"sched_method scope [0,2]\n");
        return -2;
        }
        if(sched_priority > 99 || sched_priority < 1)
        {
        fprintf(stderr,"sched_priority scope [1,99]\n");
        return -3;
        }
        if(sched_method == 1 || sched_method == 2)*/
    {
        param.sched_priority = sched_priority;
        ret = sched_setscheduler(getpid(),sched_method,&param);
        if(ret)
        {
            fprintf(stderr,"set scheduler to %d %d failed %m\n");
            return -4;
        }
    }
    int scheduler = sched_getscheduler(getpid());
    fprintf(stderr,"the scheduler of PID(%ld) is %d, priority (%d),BEGIN time is :%ld\n",
            getpid(),scheduler,sched_priority,time(NULL));
    sleep(2);
    sleeptm.tv_sec = 0;
    sleeptm.tv_nsec = NANOSECOND;
    for(i = 0;i<COUNT;i++)
    {
        test_func();
    }
 
    interval = MILLION*(tend.tv_sec - tstart.tv_sec)
               +(tend.tv_usec-tstart.tv_usec);
    fprintf(stderr," PID = %d\t priority: %d\tEND TIME is %ld\n",getpid(),sched_priority,time(NULL));
    return 0;
}
