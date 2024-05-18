//Project managing virtual pages and physical blocks using page tables.

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h> 
#include <stdint.h>
#include <stdbool.h>

//sets pageNum & pageOffset to correct values based of given int n (number 0-65,536)
void intToBin(int n, int *pageNum, int *pageOffset) 
{ 
    // just doing 16 because first 16 don't matter
    int binaryNum[16]; 
   
    // filling array starting from least significant 
    int i = 0; 
    while (n > 0 && i < 16) { 
        // storing remainder in binary array 
        binaryNum[15-i] = n % 2; 
        n = n / 2; 
        i++; 
    } 
    //fill remaining bits to left with 0
    while(i < 16){
        binaryNum[15-i] = 0;
        i++;
    }
    //now binaryNum is 16 bit binary that is equal to n

    //set pageNum to int based off most significant 8 bits
    *pageNum = binaryNum[0]*128+binaryNum[1]*64+binaryNum[2]*32+binaryNum[3]*16+binaryNum[4]*8+binaryNum[5]*4+binaryNum[6]*2+binaryNum[7];
    // printf("%d ", pageNum); 
    
    //set pageOffset to int based off least significant 8 bits
    *pageOffset = binaryNum[8]*128+binaryNum[9]*64+binaryNum[10]*32+binaryNum[11]*16+binaryNum[12]*8+binaryNum[13]*4+binaryNum[14]*2+binaryNum[15];
    // printf("%d", pageOffset); 
} 

//just fills physicalMemory at frame given with corresponding page given in backingStore
void readFromBacking(int pageNum, int frameNum, char physicalMemory[256][256]){
    FILE* backingStore;
    backingStore = fopen("BACKING_STORE.bin", "r");
    if (backingStore == NULL){
		printf("Backing Store file failed to open\n");
		exit(0);
	}

    fseek(backingStore,(pageNum*256),SEEK_SET);
    fread(physicalMemory[frameNum],sizeof(char),256,backingStore);

    fclose(backingStore);
}



int main(int argc, char* argv[])
{
    int numOfAddresses = 0; //how many addresses were read
    int nextFreeFrame = 0; //what frame is currently free
    int nextTLB = 0; //what TLB is next up
    int pageFaults = 0; //num of page faults
    int TLBHits = 0; //num of TLB hits

    char physicalMemory[256][256]; 

    int pageAtFrameZero = -1; //This is so I can say if pageTable[num] = 0 and not pageAtFrameZero then not exists
    unsigned char pageTable[256];
	memset(pageTable, 0, sizeof(pageTable));

    int TLB[2][16]; //TLB and below makes all values -1 so page 0 doesn't hit
    for(int i = 0; i < 16; i++){
        TLB[0][i] = -1; //sets all of pageNum collumn to -1
        TLB[1][i] = -1; //sets all of frameNum collumn to -1
    }

    FILE* filePointer;
    filePointer = fopen(argv[1], "r");
	if (filePointer == NULL){
		printf("Addresses file failed to open\n");
		exit(0);
	}
   
   //while more addresses
    while (!feof(filePointer)) {
        numOfAddresses++; //read an address so increment

        int number = 0; //actual input int
        fscanf(filePointer, "%d", &number);

        int pageNum = 0;  
        int pageOffset = 0;   
        intToBin(number,&pageNum,&pageOffset); //updates global pageNum & pageOffset with values corresponding to address

        //now pageNum and pageOffset are set to correct values

        printf("%s %d ","Virtual address:",number);

        int value;  //value at physical address
        int physicalAddress;    //physical address
        bool TLBHit = false;   //false is not hit, true is hit
        int frame;  //frame number corresponding to pageNum

        //check if in TLB
        for(int i = 0; i < 16; i++){
            if(TLB[0][i]==pageNum){
                //hit
                TLBHit = true;
                TLBHits++;

                value = physicalMemory[TLB[1][i]][pageOffset];
                physicalAddress = TLB[1][i]*256+pageOffset;

                break;
            }
        }

        //if runs when it wasn't in TLB
        if(!TLBHit){
            frame = pageTable[pageNum]; //will either be correct frame or 0 (if 0 need to see if it's the page at frame 0)
            if(frame==0 && pageNum!=pageAtFrameZero){
                //was not in pageTable so need to read backing
                readFromBacking(pageNum,nextFreeFrame,physicalMemory); //this doesn't update any values, just loads physicalMemory
                pageTable[pageNum] = nextFreeFrame; //set pageTable for next time that page is called
                frame =  nextFreeFrame;
                if(nextFreeFrame==0) pageAtFrameZero = pageNum; 
                pageFaults++;   //was a page fault so increment
                nextFreeFrame++;     //increment next free frame
            }

            //load TLB
            TLB[0][nextTLB] = pageNum;
            TLB[1][nextTLB] = frame;
            nextTLB = (nextTLB+1)%16; //increment

            value = physicalMemory[frame][pageOffset];
            physicalAddress = frame*256+pageOffset;
        }
        
        printf("%s %d ","Physical address:",physicalAddress);
        printf("%s %d\n","Value:",value);
    }
 
    fclose(filePointer);

    //Stats
    float faultRate = 1.0*pageFaults/numOfAddresses;
    float tlbRate = 1.0*TLBHits/numOfAddresses;

    printf("%s %d\n","Number of Translated Addresses =",numOfAddresses);
    printf("%s %d\n","Page Faults =",pageFaults);
    printf("%s ","Page Fault Rate =");
    printf("%.3f\n",faultRate);
    printf("%s %d\n","TLB Hits =",TLBHits);
    printf("%s ","TLB Hit Rate =");
    printf("%.3f\n",tlbRate);

    return 0;
}
