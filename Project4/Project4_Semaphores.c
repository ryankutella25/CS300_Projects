//This project has to use sleep and mutex locks to balance waiting time for a teachers office hours. 
//This part uses semaphores to know when the student and teachers are ready.

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytime.h"
#include <semaphore.h>

int chairs;
int studentNum = 1;
int left;
int right;
pthread_mutex_t lockStudentNum = PTHREAD_MUTEX_INITIALIZER; //lock consumers variable

int *buffer; //chairs 

sem_t studentSemaphore;  //semaphore for student waiting
sem_t teacherSemaphore;  //semaphore for teacher waiting

int frontOfQueue = 0;
int getFront() { 
    int returnVal = -1;

    //if is true when frontOfQueue is occupied so return student, mark chair as empty, and increment
    if(buffer[frontOfQueue] != -1){
        returnVal = buffer[frontOfQueue];
        buffer[frontOfQueue] = -1;
        frontOfQueue = (frontOfQueue+1)%chairs;
    }; 
    
    return returnVal; //returns -1 if chair not occupied, returns <student_number> if occupied
} 

pthread_mutex_t lockBackOfQueuePointer = PTHREAD_MUTEX_INITIALIZER; //mutex for the int backOfQueue
int backOfQueue = 0;
int putInBack (int c, pthread_t x) { 	
    int returnVal = 0;

    printf("Student %lu will call mutex_lock lockBackOfQueuePointer\n", x);
    pthread_mutex_lock(&lockBackOfQueuePointer); 

    //if is true when backOfQueue is empty so add and increment
    if(buffer[backOfQueue] == -1){
        buffer[backOfQueue] = c;
        backOfQueue = (backOfQueue+1)%chairs;
        returnVal = 1; //successful
        printf("Student %lu will call sem_post teacherSemaphore\n", x);
        sem_post(&teacherSemaphore);
    }
    
    printf("Student %lu will call mutex_unlock lockBackOfQueuePointer\n", x);
    pthread_mutex_unlock(&lockBackOfQueuePointer);
    return returnVal; //1 if succesful, 0 if not
} 

/*int mysleep (int left, int right)
{
	int mytime = 0;
	int n = 0;

	n = rand()%(right - left);	
	mytime = left + n;	
	printf("random time is %d sec\n", mytime);
	return  mytime;
}
*/

void *teacher(void *arg) {
    int sleepTime;

    while(studentNum>0){
        sem_wait(&teacherSemaphore);
        if (studentNum==0) return NULL;
        int currStudent = getFront(); 
        if(currStudent == -1){  
            // printf("Should not be here");
            currStudent = getFront(); 
        }
        
        sleepTime = mytime (left, right);
        printf("Teacher %lu to sleep %d sec\n", pthread_self(), sleepTime);
        sleep(sleepTime);
        printf("Teacher %lu wake up\n", pthread_self());

        printf("Teacher %lu will call sem_post studentSemaphore\n", pthread_self());
        sem_post(&studentSemaphore);
    }

	return NULL;
}

void *student(void *arg) {
    int count = 2; //times helped so far
    int cidNum = (long long int) arg; //cid num
    int sleepTime;    //random time

    //needs to be helped twice total
    while(count > 0){
        //first study
        sleepTime = mytime (left, right);    //random time
        printf("Student %lu to sleep %d sec\n", pthread_self(), sleepTime);
        sleep(sleepTime);
        printf("Student %lu wake up\n", pthread_self());

        //check if chairs are open
        int tryToPutInQueue = putInBack(cidNum, pthread_self());

        //if is called when succesfully inserted
        if(tryToPutInQueue!=0){

            printf("Student %lu will call sem_wait studentSemaphore\n", pthread_self());
            sem_wait(&studentSemaphore);   //waiting until teacher finished helping somone
            count--;

        }

        //go back to studying because chairs full or got helped already
    
    }

    //this student is done
    printf("Student %lu will call mutex_lock lockStudentNum\n", pthread_self());
    pthread_mutex_lock(&lockStudentNum); 
    studentNum--;
    if(studentNum==0) sem_post(&teacherSemaphore);
    printf("Student %lu will call mutex_unlock lockStudentNum\n", pthread_self());
    pthread_mutex_unlock(&lockStudentNum); 

    // printf("Student %d will call cond_signal alertTeacher\n", pthread_self());
    // pthread_cond_signal(&alertTeacher); //make sure teacher is not waiting for no one

	return NULL;
}


 int main(int argc, char *argv[]) {
    if (argc != 5) {
		fprintf(stderr, "usage: %s <studentNum> <chairs> <left> <right>\n", argv[0]);
		exit(1);
    }
    studentNum = atoi(argv[1]);
	chairs = atoi(argv[2]);
    left = atoi(argv[3]);
	right = atoi(argv[4]);
	
    buffer = (int *) malloc(chairs * sizeof(int));
    if (buffer == NULL) {printf("Allocation error!\n"); return 0;};

    sem_init(&studentSemaphore, 0, 0);
    sem_init(&teacherSemaphore, 0, 0);

    // studentMutexs = (int *) malloc(studentNum * sizeof(int));
    // if (studentMutexs == NULL) {printf("Allocation error!\n"); return 0;};
   
    int i;
    for (i = 0; i < chairs; i++) {
		buffer[i] = -1;
	}
    // for (i = 0; i < studentNum; i++) {
    //     studentMutexs[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	// }
	
   pthread_t cid[studentNum];
   pthread_t p1;

   srand (time (NULL));   
	
   printf("main: begin\n");
   pthread_create(&p1, NULL, teacher, NULL); 
   for (i = 0; i < studentNum; i++) {
		pthread_create(&cid[i], NULL, student, (void *) (long long int) i); 
   }
	
   pthread_join(p1, NULL);
   for (i = 0; i < studentNum; i++) {
		pthread_join(cid[i], NULL); 
   }
   printf("main: end\n");
   return 0;
 }