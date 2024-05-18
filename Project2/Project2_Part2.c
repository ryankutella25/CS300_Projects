//This project managing virtual pages and physical blocks using page tables. This part also implements a least recently used cache to decrease page faults. 

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
void readFromBacking(int pageNum, int frameNum, char physicalMemory[][256]){
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

//Nodes are the elements in pageTable, LRU, & 2nd column in TBR
typedef struct s_node Node;
struct s_node
{
    Node *prev;
    Node *next;
    int pageValue;
    int frameValue;
};

//make new node at head with page and frame value
Node *insertValuesToLRU( Node *head, int page, int frame, int *LRUSize)
{
    // allocate memory for the new node
    Node *node = malloc( sizeof(Node) );
    if ( node == NULL ) return NULL;

    // insert the node at the beginning of the list
    Node *temp = head->next;
    head->next = node;
    temp->prev = node;

    // fill in the fields of the node
    node->prev = head;
    node->next = temp;
    node->pageValue = page;
    node->frameValue = frame;

    *LRUSize = *LRUSize+1;

    return node;
}

//move a node already in LRU to top
Node *moveNodeToTop( Node *head, Node *node)
{
    //even though it was in LRU we need to move it to top
    Node* tempPrev = node->prev;
    Node* tempNext = node->next;
    tempPrev->next = tempNext;
    tempNext->prev = tempPrev;
    //now node is out of linked list

    // insert the node at the beginning of the list
    Node *temp = head->next;
    head->next = node;
    temp->prev = node;

    // fill in the fields of the node
    node->prev = head;
    node->next = temp;

    return node;
}

//remove the last node in LRU and return to use page and frame value
Node *removeLastNode( Node *tail, int *LRUSize ){
    Node *toRemove = tail->prev;
    Node *beforeRemove = toRemove->prev;
    beforeRemove->next = tail;
    tail->prev = beforeRemove;

    *LRUSize = *LRUSize-1;

    return toRemove; //need to return so main can use page and frame value
}

int main(int argc, char* argv[])
{
    //Part 2 Variables
    Node head = { NULL , NULL, -1, -1 };    //will always point to node with page & frame value = -1
    Node tail = { &head, NULL, -1, -1 };    //will always point to node with page & frame value = -1
    head.next = &tail;
    int LRUSize = 0;
    int frameSize = atoi(argv[2]);   
    if (frameSize < 16){
		printf("Frame size must be larger than TLB\n");
		exit(0);
	} 

    int numOfAddresses = 0; //how many addresses were read
    int frameToAddTo = 0; //what frame is currently free
    int nextTLB = 0; //what TLB is next up
    int pageFaults = 0; //num of page faults
    int TLBHits = 0; //num of TLB hits

    Node* pageTable[256] = {0};

    char physicalMemory[frameSize][256]; 

    //int array is basically column 1 and node* array is column 2
    int TLBPage[16] = {0};
    Node* TLBNode[16] = {0}; //Node* instead of just frame number so that it links to LRU and easy to move to top of stack (get frame from node->frameValue)

    FILE* filePointer;
    filePointer = fopen(argv[1], "r");
	if (filePointer == NULL){
		printf("Addresses file failed to open\n");
		exit(0);
	}

    printf("%s%d \n","# of frames: ",frameSize);
   
   //while there are more addresses
    while (!feof(filePointer)) {
        numOfAddresses++; //read an address so increment

        int number = 0; //actual input int as is
        fscanf(filePointer, "%d", &number);

        int pageNum = 0;  
        int pageOffset = 0;   
        intToBin(number,&pageNum,&pageOffset); //updates pageNum & pageOffset with values corresponding to address

        //now pageNum and pageOffset are set to correct values

        printf("%s %d ","Virtual address:",number);

        int physicalAddress;    //physical address
        int value;  //value at physical address
        bool TLBHit = false;   //false is not hit, true is hit
        int frame;  //frame number corresponding to pageNum

        //check if in TLB
        for(int i = 0; i < 16; i++){
            if(TLBPage[i]==pageNum){
                //hit
                TLBHit = true;
                TLBHits++;

                Node* currNode = TLBNode[i];    //curr node holds corresponding frame number

                frame = currNode->frameValue;

                moveNodeToTop(&head,currNode);  //move node to top of LRU

                break;
            }
        }

        //if runs when it wasn't in TLB
        if(!TLBHit){
            Node *currNode = pageTable[pageNum]; //will either be correct node or 0 (meaning unset)
            if(currNode==0){
                //since it is unset in pageTable it isn't in LRU so add and increment size
                if(LRUSize>=frameSize){
                    // get last content node out of linked list
                    Node *toRemove = removeLastNode(&tail, &LRUSize);
                    
                    // unset the node from the pageTable at page value from removed node
                    pageTable[toRemove->pageValue] = 0;
                    frame = toRemove->frameValue;   //we want to put new info at frame old info was at

                    free(toRemove);  //unallocate toRemove
                }else{
                    frame = frameToAddTo;
                    frameToAddTo++; //since LRU not full do frames like part 1
                }

                //was not in pageTable so need to read backing
                readFromBacking(pageNum, frame, physicalMemory); //this doesn't update any values, just loads physicalMemory
                
                currNode = insertValuesToLRU(&head, pageNum, frame, &LRUSize); //set pageTable to new node at beginning of LRU;
                pageTable[pageNum] = currNode;

                pageFaults++;   //was a page fault so increment
            }else{
                //currNode was a valid node and thus in LRU
                frame = currNode->frameValue;

                //move node to front of LRU
                moveNodeToTop(&head,currNode);
            }

            //load TLB
            TLBPage[nextTLB] = pageNum;
            TLBNode[nextTLB] = currNode;
            nextTLB = (nextTLB+1)%16; //increment (FIFO)

        }

        value = physicalMemory[frame][pageOffset];
        physicalAddress = (frame*256)+pageOffset;
        
        printf("%s %d%s ","Physical address:",physicalAddress,",");
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
