// For malloc() and free()
#include <stdlib.h>
// For usleep()
#include <unistd.h>
// For uint32_t data type
#include <stdint.h>
// For time()
#include <time.h>
// For gettimeofday()
#include <sys/time.h>
// For printf()
#include <stdio.h>
// For semaphore functionality
#include <semaphore.h>

// Some constant definitions
#define TOTAL_SYSTEM_MEM_STORAGE 1000
#define PAGE_NUMBER_LIMIT 45000

#define RAM_STORAGE_COUNT 25
#define SSD_STORAGE_COUNT 100
#define HDD_STORAGE_COUNT 1000
#define RAM_ACCESS_SLEEP_TIME 0.01
#define SSD_ACCESS_SLEEP_TIME 0.1
#define HDD_ACCESS_SLEEP_TIME 2.5

#define THRESHOLD_RANDOM 50
#define MAXIMUM_RANDOM 1000

#define DEBUG 1

// Some FIXED constant definitions
#define TRUE 1
#define FALSE 0
// Type definitions
typedef signed short vAddr;

// Struct definition for PTE
typedef struct {
  vAddr pageNumber; // Page number in the table
  unsigned int presentBit:1; // 1 bit; 0 => Not present or filled in, 1 => It is present
  unsigned int level:3; // 3 bits; 0 => RAM; 1 => SSD; 2 => Hard disk
  unsigned int index; // Index into the level to find the object
  uint32_t *memory; // Underlying memory level
  struct timeval lastAccessTime; // Keeps track of when this page was last accessed or created
} PageTableEntry;
// Global variables declaration
int __initialized = 0; // 1 => All the structs are properly initialized. 0 => All structs are NOT yet initialized
int overallMemoryUsed = 0; // By default, 0 entries in the PTE will be filled
int ramStoredHere[RAM_STORAGE_COUNT]; // Memory bitmap for the RAM initially everything is set to 1 indicating that it IS free
int ssdStoredHere[SSD_STORAGE_COUNT]; // Memory bitmap for the SSD initially everything is set to 1 indicating that it IS free
int hddStoredHere[HDD_STORAGE_COUNT]; // Memory bitmap for the HDD initially everything is set to 1 indicating that it IS free

PageTableEntry pageTable[TOTAL_SYSTEM_MEM_STORAGE]; // Page table entries array
uint32_t ramStorage[RAM_STORAGE_COUNT]; // RAM Emulated Storage
uint32_t ssdStorage[SSD_STORAGE_COUNT]; // SSD Emulated Storage
uint32_t hddStorage[HDD_STORAGE_COUNT]; // HDD Emulated Storage

sem_t *memory_lock;

// Helper functions prototypes
void __initialize();
int limitedRandom(int min, int max);
unsigned long long msecSinceEpoch(struct timeval timeVal);
vAddr producePageNumberWithoutClash();
void updateBitmap(int level, int index, int bitmapValue);
int *addNewPage(int pageNumber);
int numberOfFreePages(int level);
int numberOfUsedPages(int level);
int *findEmptySlotAt(int level);
int *findFirstEmptySlot();
int *pickAPageToEvict(int level);
void switchToMemoryLocation(int *startLocation, int *destinationLocation);
int getPageTableNumber(int level, int index);
void updatePageTable(int sLevel, int sIndex, int fLevel, int fIndex);

// Some macros as we made significant code changes
#define RAMMemoryUsed numberOfUsedPages(0)
#define SSDMemoryUsed numberOfUsedPages(1)
#define HDDMemoryUsed numberOfUsedPages(2)

// Necessary function declarations
vAddr create_page() {
  sem_wait(memory_lock); // Wait for the semaphore lock
  // See if we've initialized our stuff or not
  if( !__initialized ) 
    __initialize(); // Perform the initialization
  // See if we've got memory to be given away
  if( overallMemoryUsed == TOTAL_SYSTEM_MEM_STORAGE ) {
    // We've no more memory to be given out
    return -1; // No memory error
  }
  // Else, create a new page number and let us initialize the stuff
  vAddr availablePageNumber = producePageNumberWithoutClash();
  // Solve for pageTableIndex and find the appropriate index
  int ctr = 0; // Loop counter
  int pageTableIndex = -1; // Index to browse
  for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    if( pageTable[ctr].presentBit == 0 ) {
      pageTableIndex = ctr; // We found an index to save the page in
      break;
    } 
  }
  // Make a page table entry
  pageTable[pageTableIndex].pageNumber = availablePageNumber; // We are now setting the page number
  pageTable[pageTableIndex].level = 0; // We are initially storing it in the RAM
  pageTable[pageTableIndex].index = addNewPage(pageTableIndex)[1]; // Fetch a new index into the page table
  pageTable[pageTableIndex].memory = &ramStorage[pageTable[pageTableIndex].index]; // Grab the memory storage
  gettimeofday( &pageTable[pageTableIndex].lastAccessTime , NULL ); // Save the time of last access, in this case the creation time of the page
  pageTable[overallMemoryUsed].presentBit = 1; // We are now adding a page
  // Finally, move onto incrementing the overall memory iser
  overallMemoryUsed++;
  // Return the required memory address
  sem_post(memory_lock); // Release semaphore lock
  return availablePageNumber;
}

uint32_t get_value(vAddr address, int *valid) {
  sem_wait(memory_lock); // Wait for the semaphore lock
  // Lookup the entire page table to find the necessary page
  int ctr = 0; // Loop counter
  for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    if( pageTable[ctr].pageNumber == address && pageTable[ctr].presentBit ) {
      // We've found the required page 
      *valid = 1; // Set that the fetch was value
      // Set the new access time for this page
      gettimeofday( &pageTable[ctr].lastAccessTime, NULL );
      sem_post(memory_lock); // Release semaphore lock
      // After setting that the fetch was valid, return the required value
      if( pageTable[ctr].level == 0 )
        return ramStorage[pageTable[ctr].index]; // Fetch and return RAM value
      else if( pageTable[ctr].level == 1 )
        return ssdStorage[pageTable[ctr].index]; // Fetch and return SSD value
      else if( pageTable[ctr].level == 2 )
        return hddStorage[pageTable[ctr].index]; // Fetch and return HDD value
    }
  }
  // We didn't get a hit, so we return 0
  *valid = 0; // We didn't hit a page number with that number
  sem_post(memory_lock); // Release semaphore lock
  return 0; // Return default 0
}

void store_value(vAddr address, uint32_t *value) {
  sem_wait(memory_lock); // Wait for the semaphore lock
  // Lookup the entire page table to find the necessary page
  int ctr = 0; // Loop counter
  for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    if( pageTable[ctr].pageNumber == address && pageTable[ctr].presentBit ) {
      // We've found the right page. First update its access time then look at its level
      gettimeofday( &pageTable[ctr].lastAccessTime, NULL );
      // Now look at its level
      if( pageTable[ctr].level == 0 ) {
        // It is stored in the RAM. Update the value and we're done
	ramStorage[pageTable[ctr].index] = *value; // Store the required value
	// Exit the loop as we found a match
	break;
      } else if( pageTable[ctr].level == 1 ) {
        // It is stored in the SSD. Swap it in.
	// Check if the RAM is empty or not
	if( RAMMemoryUsed < RAM_STORAGE_COUNT ) {
          // RAM is free. We can directly swap it in
	  // STEP 1: Find the free location in the RAM
	  int *toLevelInRAM = findEmptySlotAt(0);
          // STEP 2: Set up from swap location
	  int *ssdLocation = (int*)malloc(sizeof(int) * 2);
	  ssdLocation[0] = 1; // Location is from the SSD
	  ssdLocation[1] = pageTable[ctr].index; // This is the index to be looked up in the array of SSD Values
	  // STEP 3: Perform the swap
	  switchToMemoryLocation(ssdLocation, toLevelInRAM);
	  // STEP 4: We're done
          sem_post(memory_lock); // Release semaphore lock
	  return;
	} else {
	  // The RAM is NOT empty. We gotta evict a page from RAM and essentially swap with SSD
	  // STEP 1: Find a page to be evicted from RAM
          int levelToBeSwappedOutInRAM = pickAPageToEvict(0)[1]; // Pick an index to be evicted from the RAM
	  // STEP 2: Swap out the level to the SSD and bring in the SSD one
	  int *ssdLocation = (int*)malloc(sizeof(int) * 2);
	  ssdLocation[0] = 1; // Location is in the SSD
	  ssdLocation[1] = pageTable[ctr].index; // This is the index to be looked up in the array of SSD Values
	  // STEP 3: Set up the new memory level
          int *ramLocation = (int*)malloc(sizeof(int) * 2);
	  ramLocation[0] = 0; // Location is in the RAM
	  ramLocation[1] = levelToBeSwappedOutInRAM; // This is the level we are targetting to be swapped out
	  // STEP 4: Switch out the values
          switchToMemoryLocation(ramLocation, ssdLocation);
	  // STEP 5: Update the value of the entry
	  usleep(RAM_ACCESS_SLEEP_TIME * 1000000); // Sleep before adding the value
          ramStorage[levelToBeSwappedOutInRAM] = *value; // Store the required value in the RAM storage just populated
	  // STEP 6: Update the page table for the entry that will be brought into the RAM
	  updatePageTable(pageTable[ctr].level, pageTable[ctr].index, 0, levelToBeSwappedOutInRAM);
	}
	// Exit the loop as we found a match
	break;
      } else if( pageTable[ctr].level == 2 ) {
        // It is stored in the HDD. Swap it in.
	// Check if RAM is empty or not
	if( RAMMemoryUsed < RAM_STORAGE_COUNT ) {
          // RAM is free. We can directly swap it in
	  // STEP 1: Find the free location in the RAM
	  int *toLocationInRAM = findEmptySlotAt(0);
	  // STEP 2: Set up the from swap location
	  int *hddLocation = (int*)malloc(sizeof(int) * 2);
	  hddLocation[0] = 2; // Location is from the SSD
	  hddLocation[1] = pageTable[ctr].index; // This is the index to be looked up in the array of HDD values
	  // STEP 3: Perform the swap
	  switchToMemoryLocation(hddLocation, toLocationInRAM);
	  // STEP 4: We're done
          sem_post(memory_lock); // Release semaphore lock
	  return;
	} else if( SSDMemoryUsed < SSD_STORAGE_COUNT ) {
	  // The RAM is NOT empty and the SSD is empty. We gotta evict a page from the RAM to the SSD and then
	  //  from the HDD to the RAM.
	  // STEP 0: Read the value from the HDD into a malloced storage
	  int *valueInHDD = (int*)malloc(sizeof(int));
	  usleep(HDD_ACCESS_SLEEP_TIME * 1000000); // Sleep before reading in the value
	  *valueInHDD = hddStorage[pageTable[ctr].index]; // Read in the value into the valueInHDD pointer
	  // STEP 1: Find a page to be evicted from the RAM
	  int fromRAMSwap = pickAPageToEvict(0)[1]; // Pick an index to be evicted from the RAM
	  // STEP 2: Resolve a location to swap out this RAM page to
	  int *ssdLocation = findEmptySlotAt(1); // Find an empty slot in the SSD to swap out to
	  // STEP 3: Setup the RAM swap out location
	  int *ramFromSwapLocation = (int*)malloc(sizeof(int) * 2);
	  ramFromSwapLocation[0] = 0; // Location is in the RAM
	  ramFromSwapLocation[1] = fromRAMSwap;
	  // STEP 4: Perform the actual swap
	  switchToMemoryLocation(ramFromSwapLocation, ssdLocation);
	  // STEP 5: Now we got a free spot in the RAM. Write the value by reading from the HDD
	  //  We've already read from the memory and we've got a free spot in the RAM. Write to it.
	  usleep(RAM_ACCESS_SLEEP_TIME * 1000000); // Sleep before adding the value
	  ramStorage[fromRAMSwap] = *valueInHDD; // Store the required value into the RAM
	  // STEP 10: Update the page table for the entry just brought into the RAM
	  updatePageTable(pageTable[ctr].level, pageTable[ctr].index, 0, fromRAMSwap);
	} else {
          // The RAM AND SSD is not empty. We gotta evict a page from the SSD to the HDD and then from the 
	  //  RAM to the SSD and then store the value in the RAM
	  // STEP 0: Read the value from the HDD into a malloced storage
	  int *valueInHDD = (int*)malloc(sizeof(int));
	  usleep(HDD_ACCESS_SLEEP_TIME * 1000000); // Sleep before reading in the value
	  *valueInHDD = hddStorage[pageTable[ctr].index]; // Read in the value into the valueInHDD pointer
	  // STEP 1: Find a page to be evicted from the SSD
	  int fromSSDSwap = pickAPageToEvict(1)[1]; // Pick an index to be evicted from the SSD
	  // STEP 2: Resolve a location to swap out this SSD page to
	  int *hddLocation = (int*)malloc(sizeof(int) * 2);
	  hddLocation[0] = 2; // Location is in the HDD
	  hddLocation[1] = pageTable[ctr].index; // This is the index to be looked up in the array of HDD Values
	  // STEP 3: Setup the SSD swap out memory location
	  int *ssdFromSwapLocation = (int*)malloc(sizeof(int) * 2);
	  ssdFromSwapLocation[0] = 1; // Location is in the SSD
	  ssdFromSwapLocation[1] = fromSSDSwap;
	  // STEP 4: Perform the SSD to HDD page swap
	  switchToMemoryLocation(ssdFromSwapLocation, hddLocation);
	  // STEP 5: Find a page to be evicted from the RAM
	  int fromRAMSwap = pickAPageToEvict(0)[1]; // Pick an index to be evicted from RAM
	  // STEP 6: Setup the RAM swap out location
	  int *ramFromSwapLocation = (int*)malloc(sizeof(int) * 2);
	  ramFromSwapLocation[0] = 0; // Location is in the RAM
	  ramFromSwapLocation[1] = fromRAMSwap;
	  // STEP 7: Setup destination location in the SSD
	  int *ssdLocation = ssdFromSwapLocation; // Essentially we should swap to the SSD Location just freed up
	  // STEP 8: Peform the swap with the destination to the freed up SSD slot
	  switchToMemoryLocation(ramFromSwapLocation, ssdLocation);
	  // STEP 9: Now we got a free spot in the RAM. Write the value by reading from the HDD
	  // We've already read from the memory and we've got a free spot in the RAM. Write to it.
	  usleep(RAM_ACCESS_SLEEP_TIME * 1000000); // Sleep before adding the value
	  ramStorage[fromRAMSwap] = *valueInHDD; // Store the required value into the RAM
	  // STEP 10: Update the page table for the entry just brought into the RAM
	  updatePageTable(pageTable[ctr].level, pageTable[ctr].index, 0, fromRAMSwap);
	}
	// Exit the loop as we found a match
	break;
      }
    }
  }
  sem_post(memory_lock); // Release semaphore lock
}

void free_page(vAddr address) {
  sem_wait(memory_lock); // Wait for the semaphore lock
  // Look up for the page table entry
  int ctr = 0; // Loop counter
  for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    if( pageTable[ctr].pageNumber == address ) {
      // We found the page
      // Tell the user
      if(DEBUG) printf("Page #%d was freed from the paging simulator.\n", getPageTableNumber(pageTable[ctr].level, pageTable[ctr].index));
      // Clean up now
      if( pageTable[ctr].level == 0 )
        ramStorage[pageTable[ctr].index] = 0;
      else if( pageTable[ctr].level == 1 )
        ssdStorage[pageTable[ctr].index] = 0;
      else if( pageTable[ctr].level == 2 )
        hddStorage[pageTable[ctr].index] = 0;
      // Free this page's bitmap before clearing all its necessary data
      updateBitmap( pageTable[ctr].level, pageTable[ctr].index, 1); // This memory location is now AVAILABLE
      // Clear up the page table entry
      pageTable[ctr].pageNumber = -1; // We don't have a page here no more
      pageTable[ctr].level = -1; // We don't want to point anywhere
      pageTable[ctr].index = -1; // We don't want to point anywhere
      pageTable[ctr].memory = NULL; // No memory location will be stored here
      pageTable[ctr].presentBit = 0; // This page is no longer present in the memory
      gettimeofday( &pageTable[ctr].lastAccessTime , NULL ); // Save the time when the page was deleted into last access time
                                                             // Doesn't matter what time is in there as either ways, we shouldn't
						             //  be accessing the time stored on a page with presentBit == 0
      // Decrement overallMemoryUsed
      overallMemoryUsed--;
      // Exit the loop
      sem_post(memory_lock); // Release semaphore lock
      return;
    }
  }
  // We didn't find the desired page
  sem_post(memory_lock); // Release semaphore lock
  return; // We still gotta return
}

// Helper functions declarations
int limitedRandom(int min, int max) {
  // ASSUMPTION: The user has called
  //  srand((unsigned)time(NULL));
  // before calling on this function.
  return (min + (rand() % (max - min)));
}

unsigned long long msecSinceEpoch(struct timeval timeVal) {
  unsigned long long msecSinceEpochVal = ( (unsigned long long)(timeVal.tv_sec) * 1000) +
                                         ((unsigned long long)(timeVal.tv_usec) / 1000);
  // Return the required value
  return msecSinceEpochVal;
}

void __initialize() {
  // Initialize seamphore
  memory_lock = (sem_t*)malloc(sizeof(sem_t));
  sem_init(memory_lock, 0, 1);
  // Welcome the user
  if(DEBUG) printf("====== PAGE FAULT SIMULATOR ======\n");
  // Initialize all our stuff
  // STEP 1: Initialize our bitmaps
  int ctr = 0; // Loop counter
  for( ; ctr < RAM_STORAGE_COUNT; ctr++ )
    ramStoredHere[ctr] = 1; // This is a free spot in the RAM
  for( ctr = 0; ctr < SSD_STORAGE_COUNT; ctr++ )
    ssdStoredHere[ctr] = 1; // This is a free spot in the SSD
  for( ctr = 0; ctr < HDD_STORAGE_COUNT; ctr++ )
    hddStoredHere[ctr] = 1; // This is a free spot in the HDD
  // STEP 2: Initialize values in the storage areas itself
  for( ctr = 0; ctr < RAM_STORAGE_COUNT; ctr++ )
    ramStorage[ctr] = -1; // Storing -1 as our default value
  for( ctr = 0; ctr < SSD_STORAGE_COUNT; ctr++ )
    ssdStorage[ctr] = -1; // Storing -1 as our defualt value
  for( ctr = 0; ctr < HDD_STORAGE_COUNT; ctr++ )
    hddStorage[ctr] = -1; // Storing -1 as our default value
  // STEP 3: Finally, initialize entries in the Page Table
  for( ctr = 0; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    pageTable[ctr].pageNumber = -1; // Setting default page number to -1
    pageTable[ctr].presentBit = 0; // Initially this page is not present or populated for that fact
    pageTable[ctr].level = -1; // We cannot access a level for this page
    pageTable[ctr].index = -1; // We cannot index into the level for this page
    pageTable[ctr].memory = NULL; // We don't point to anything for the time being
    gettimeofday( &pageTable[ctr].lastAccessTime , NULL ); // Set the default value as time of initialization
                                                           // We shouldn't be too concerned with this value either
						           //  ways.
  }
  // Finally, set __initialized to 1
  __initialized = 1; // We're done initializing our stuff
}

vAddr producePageNumberWithoutClash() {
  // AIM: Produce a page number without any clashes
  vAddr pageNumber = 0;
  // Loop over till we find no clashes
  while(TRUE) {
    // Produce a limitedRandom page number
    pageNumber = limitedRandom(0, PAGE_NUMBER_LIMIT);
    // Loop over all the page entries to see if we have a clash
    int found = 0; // We haven't found a clash by default
    int ctr = 0; // Loop counter
    for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
      if(pageTable[ctr].presentBit == 1 && pageTable[ctr].pageNumber == pageNumber) {
        // We found the same page number
	found = 1; // We found a clash
	break; // Exit this for loop
      }
    }
    // See if we clashed
    if(!found && pageNumber > 0) 
      break; // If we didn't find a clash break out and return the required page number
  }
  // Return the required page number
  return pageNumber;
}

void updateBitmap(int level, int index, int bitmapValue) {
  // Start processing the bitmap update as requested
  if( level == 0 )
    ramStoredHere[index] = bitmapValue;
  else if( level == 1 )
    ssdStoredHere[index] = bitmapValue;
  else if( level == 2 )
    hddStoredHere[index] = bitmapValue;
  // Return now
  return;
}

int *addNewPage(int pageNumber) {
  int *toReturn = (int*)malloc(sizeof(int) * 2);
  // Set some default values
  toReturn[0] = -1; // We couldn't add the page as requested on any level
  toReturn[1] = -1; // The page cannot be accessed at the given level of storage
  // See if we've got space in the RAM
  if( RAMMemoryUsed < RAM_STORAGE_COUNT ) {
    // We've got storage in the RAM
    toReturn[0] = 0; // Level is RAM
    toReturn[1] = findEmptySlotAt(0)[1]; // Index into the first free empty slot got in the RAM
    // Increase memory usage by updating the RAM bitmap
    updateBitmap(0, toReturn[1], 0); // The RAM location is NO longer free
    // Tell the user
    if(DEBUG) printf("A new page with page #%d was directly created in the RAM.\n", pageNumber);
    // Return the requested item
    return toReturn;
  } else if( SSDMemoryUsed < SSD_STORAGE_COUNT ) {
    // We've got no storage in the RAM
    // STEP 1: Find the page to be evicted at RAM level
    int pageIndexToEvictAtRAM = pickAPageToEvict(0)[1];
    // STEP 2: Find a suitable index at level 1 to move it to
    int *levelToMoveTo = findEmptySlotAt(1); 
    // STEP 3: Create levels to execute the swap
    toReturn[0] = 0; // To be evicted from RAM
    toReturn[1] = pageIndexToEvictAtRAM; // Evict index
    // STEP 4: Execute the swap
    switchToMemoryLocation(toReturn, levelToMoveTo);
    // STEP 5: Increase memory usage by updating the RAM and SSD bitmaps
    updateBitmap(0, pageIndexToEvictAtRAM, 0); // The RAM location is NO longer free
    updateBitmap(1, levelToMoveTo[1], 0); // The SSD location is NO longer free
    // STEP 6: Tell the user
    if(DEBUG) printf("A new page with page #%d was added to the RAM causing eviction of a page to the SSD with Page #%d\n", pageNumber, getPageTableNumber(1, levelToMoveTo[1])); 
    // STEP 7: Return the level which was freed up
    return toReturn;
  } else if( HDDMemoryUsed < HDD_STORAGE_COUNT ) {
    // We've got no storage in RAM
    // STEP 1: Find the page to be evicted at the SSD level
    int *pageEvictedFromSSD = pickAPageToEvict(1);
    // STEP 2: Find slot in the HDD level to place the SSD page
    int *destinationInHDD = findEmptySlotAt(2);
    // STEP 3: Perform the swap
    switchToMemoryLocation(pageEvictedFromSSD, destinationInHDD);
    // STEP 4: Find the RAM page to be moved out now
    int *pageEvictedFromRAM = pickAPageToEvict(0);
    // STEP 5: Resolve the location to place this RAM page
    int *destinationInSSD = pageEvictedFromSSD;
    // STEP 6: Perform the swap
    switchToMemoryLocation(pageEvictedFromRAM, destinationInSSD);
    // STEP 7: Resolve the empty spot
    toReturn[0] = 0; // Space freed up in RAM
    toReturn[1] = pageEvictedFromRAM[1]; // Index of free location in the RAM
    // STEP 8: Update the bitmaps
    updateBitmap(0, pageEvictedFromRAM[1], 0); // This RAM location is NO longer free
    updateBitmap(1, destinationInSSD[1], 0); // This SSD location is NO longer free
    updateBitmap(2, destinationInHDD[1], 0); // This location in HDD is NO longer free as well
    // STEP 9: Tell the user
    if(DEBUG) printf("A new page with page #%d was added to the RAM causing eviction of a page from the RAM to the SSD with Page #%d and a page from SSD to the HDD with page #%d\n", pageNumber, getPageTableNumber(1, destinationInSSD[1]), getPageTableNumber(2, destinationInHDD[1])); 
    // STEP 10: Return the level which was freed up
    return toReturn;
  }
  // If nothing matches, return the default values
  return toReturn;
}

int numberOfFreePages(int level) {
  // Start by processing the specific level
  if( level == 0 ) {
    int count = 0; // Keeps track of the desired count
    int ctr = 0; // Loop counter
    for( ; ctr < RAM_STORAGE_COUNT; ctr++ )
      if( ramStoredHere[ctr] == 1 )
        count++; // We found a free page, increment the counter
    // Return the desired value
    return count;
  } else if( level == 1 ) {
    int count = 0; // Keeps track of the desired count
    int ctr = 0; // Loop counter
    for( ; ctr < SSD_STORAGE_COUNT; ctr++ )
      if( ssdStoredHere[ctr] == 1 )
        count++; // We found a free page, increment the counter
    // Return the desired value
    return count;
  } else if( level == 2 ) {
    int count = 0; // Keeps track of the desired count
    int ctr = 0; // Loop counter
    for( ; ctr < HDD_STORAGE_COUNT; ctr++ )
      if( hddStoredHere[ctr] == 1 )
        count++; // We found a free page, increment the counter
    // Return the desired value
    return count;
  }
  // Else, return -1
  return -1;
}

int numberOfUsedPages(int level) {
  // Start by calling numberOfFreePages(int level)
  if( level == 0 )
    return (RAM_STORAGE_COUNT - numberOfFreePages(0));
  else if( level == 1 )
    return (SSD_STORAGE_COUNT - numberOfFreePages(1));
  else if( level == 2 )
    return (HDD_STORAGE_COUNT - numberOfFreePages(2));
  // Else, return -1
  return -1;
}

int *findFirstEmptySlot() {
  int *toReturn = (int*)malloc(sizeof(int) * 2);  
  // Setup some default values
  toReturn[0] = -1; // Nothing is empty (shouldn't be the case)
  toReturn[1] = -1; // Couldn't index into the storage option (shouldn't be the case either)
  // Let us call a helper function
  int *tempStore = findEmptySlotAt(0); // Let us attempt to find a slot in RAM
  if( tempStore[0] != (-1) && tempStore[1] != (-1) )
    return tempStore; // We found the required slot in the RAM
  free(tempStore); // Empty up some space
  tempStore = findEmptySlotAt(1); // Let us attempt to find a slot in the SSD
  if( tempStore[0] != (-1) && tempStore[1] != (-1) )
    return tempStore; // We found the required slot in the SSD 
  free(tempStore); // Empty up some space
  tempStore = findEmptySlotAt(2); // Let us attempt to find a slot in the HDD
  if( tempStore[0] != (-1) && tempStore[1] != (-1) )
    return tempStore; // We found the required slot in the HDD
  free(tempStore); // Empty up some space
  // Else return the default value
  return toReturn;
}

int *findEmptySlotAt(int level) {
  int *toReturn = (int*)malloc(sizeof(int) * 2);
  // Set up some default values
  toReturn[0] = -1; // We couldn't find any storage at the given level
  toReturn[1] = -1; // We failed to index into the storage to find you a free slot
  // Start processing, going through the RAM
  if( level == 0 && RAMMemoryUsed < RAM_STORAGE_COUNT ) {
    // We've got some needed storage in the RAM
    toReturn[0] = 0; // To indicate RAM
    // Loop over the bitmap to find this location
    int ctr = 0; // Loop counter
    for( ; ctr < RAM_STORAGE_COUNT; ctr++ ) {
      if( ramStoredHere[ctr] == 1 ) {
        toReturn[1] = ctr; // We found an empty slot; Next available index is this in the RAM
	break;
      }
    }
    // Return the required values
    return toReturn;
  } else if( level == 1 && SSDMemoryUsed < SSD_STORAGE_COUNT ) {
    // We've got a slot in SSD
    toReturn[0] = 1; // To indicate SSD
    // Loop over the bitmap to find this location
    int ctr = 0; // Loop counter
    for( ; ctr < SSD_STORAGE_COUNT; ctr++ ) {
      if( ssdStoredHere[ctr] == 1 ) {
        toReturn[1] = ctr; // We found an empty slot; Next available index is this in the SSD
	break;
      }
    }
    // Exit the function and return the required value
    return toReturn;
  } else if( level == 2 && HDDMemoryUsed < HDD_STORAGE_COUNT ) {
    // We've got a slot in HDD
    toReturn[0] = 2; // To indicate HDD
    // Loop over the bitmap to find this location
    int ctr = 0; // Loop counter
    for( ; ctr < HDD_STORAGE_COUNT; ctr++ ) {
      if( hddStoredHere[ctr] == 1 ) {
        toReturn[1] = ctr; // We found an empty slot; Next available index is this in the HDD
	break;
      }
    }
    // Exit the function and return the required value
    return toReturn;
  }
  // Else, return the default values
  return toReturn;
}

int *pickAPageToEvict(int level) {
  // Set up some default values
  int *toReturn = (int*)malloc(sizeof(int) * 2);
  toReturn[0] = -1; // We couldn't locate a page to evict
  toReturn[1] = -1; // We couldn't find an index in that level to be evicted
  // Check which level the user is looking for
  if( level == 0 ) {
    // If the user is looking for RAM:
    // Page Eviction Policy: Pick a random page
    toReturn[0] = 0; // Level is RAM
    // Loop over all the pages and see which page has to be evicted
    int ctr = 0; // Loop Counter
    unsigned long long minimumEpochMsec = 99999999999999; // Minimum epoch time will be stored here (maximized) initially
    int minimumEpochIndex = 0; // Index where the minimum epoch time exists
    for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
      if( pageTable[ctr].presentBit && ( pageTable[ctr].level == 0 )
                                    && ( !ramStoredHere[pageTable[ctr].index] ) )
      {
        // Page table entry is valid and bitmap shows that RAM is occupied at this space and level is 0 (RAM)
	// Now, we compare to find a minimum: 
	if( msecSinceEpoch(pageTable[ctr].lastAccessTime) < minimumEpochMsec ) {
          // We found a new minimum 
	  minimumEpochMsec = msecSinceEpoch(pageTable[ctr].lastAccessTime);
	  minimumEpochIndex = pageTable[ctr].index;
	  // Save a new one:
	  toReturn[1] = minimumEpochIndex;
	}
      }
    }
  } else if( level == 1 ) {
    // If the user is looking for SSD:
    // Page Eviction Policy: Pick a random page
    toReturn[0] = 1; // Level is SSD
    // Loop over all the pages and see which page has to be evicted
    int ctr = 0; // Loop Counter
    unsigned long long minimumEpochMsec = 99999999999999; // Minimum epoch time will be stored here (maximized) initially
    int minimumEpochIndex = 0; // Index where the minimum epoch time exists
    for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
      if( pageTable[ctr].presentBit && ( pageTable[ctr].level == 1 )
                                    && ( !ssdStoredHere[pageTable[ctr].index] ) )
      {
        // Page table entry is valid and bitmap shows that SSD is occupied at this space and level is 1 (SSD)
	// Now, we compare to find a minimum: 
	if( msecSinceEpoch(pageTable[ctr].lastAccessTime) < minimumEpochMsec ) { 
          // We found a new minimum 
	  minimumEpochMsec = msecSinceEpoch(pageTable[ctr].lastAccessTime);
	  minimumEpochIndex = pageTable[ctr].index;
	  // Save a new one:
	  toReturn[1] = minimumEpochIndex;
	}
      }
    }
  } else if( level == 2 ) {
    // If the user is looking for HDD:
    // Page Eviction Policy: Pick a random page
    toReturn[0] = 2; // Level is HDD
    // Loop over all the pages and see which page has to be evicted
    int ctr = 0; // Loop Counter
    unsigned long long minimumEpochMsec = 99999999999999; // Minimum epoch time will be stored here (maximized) initially
    int minimumEpochIndex = 0; // Index where the minimum epoch time exists
    for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
      if( pageTable[ctr].presentBit && ( pageTable[ctr].level == 2 )
                                    && ( !hddStoredHere[pageTable[ctr].index] ) )
      {
        // Page table entry is valid and bitmap shows that HDD is occupied at this space and level is 2 (HDD)
	// Now, we compare to find a minimum: 
	if( msecSinceEpoch(pageTable[ctr].lastAccessTime) < minimumEpochMsec ) {
          // We found a new minimum 
	  minimumEpochMsec = msecSinceEpoch(pageTable[ctr].lastAccessTime);
	  minimumEpochIndex = pageTable[ctr].index;
	  // Save a new one:
	  toReturn[1] = minimumEpochIndex;
	}
      }
    }
  }
  // Tell the user
  if( level == 0 )
    printf("Least Recently Used algorithm dictates eviction of page with #%d from the RAM level\n", getPageTableNumber(0, toReturn[1]));
  else if( level == 1 )
    printf("Least Recently Used algorithm dictates eviction of page with #%d from the SSD level\n", getPageTableNumber(1, toReturn[1]));
  else if( level == 2 )
    printf("Least Recently Used algorithm dictates eviction of page with #%d from the HDD level\n", getPageTableNumber(2, toReturn[1]));
  // Return the required struct
  return toReturn;
}

void switchToMemoryLocation(int *startLocation, int *destinationLocation) {
  // Figure out the level of swapping needed
  int levelOfSwappingNeeded = destinationLocation[0] - startLocation[0];
  // Start processing this level and figure out the swapping mechanism
  if( levelOfSwappingNeeded == 1 ) {
    // Might be RAM -> SSD or SSD -> HDD
    if( startLocation[0] == 0 && destinationLocation[0] == 1 ) {
      // Sleep for the required time
      usleep((RAM_ACCESS_SLEEP_TIME + SSD_ACCESS_SLEEP_TIME) * 1000000);
      // Requested operation: RAM -> SSD
      ssdStorage[destinationLocation[1]] = ramStorage[startLocation[1]];
      ramStorage[startLocation[1]] = 0;
      // Update the page table
      updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
    } else if( startLocation[0] == 1 && destinationLocation[0] == 2 ) {
      // Sleep for the required time
      usleep((SSD_ACCESS_SLEEP_TIME + HDD_ACCESS_SLEEP_TIME) * 1000000);
      // Requested operation: SSD -> HDD
      hddStorage[destinationLocation[1]] = ssdStorage[startLocation[1]];
      ssdStorage[startLocation[1]] = 0;
      // Update the page table
      updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
    }
  } else if( levelOfSwappingNeeded == (-1) ) {
    // Might be SSD -> RAM or HDD -> SSD
    if( startLocation[0] == 1 && destinationLocation[0] == 0 ) {
      // Sleep for the required time
      usleep((RAM_ACCESS_SLEEP_TIME + SSD_ACCESS_SLEEP_TIME) * 1000000);
      // Requested operation: SSD -> RAM
      ramStorage[destinationLocation[1]] = ssdStorage[startLocation[1]];
      ssdStorage[startLocation[1]] = 0;
      // Update the page table
      updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
    } else if( startLocation[0] == 2 && destinationLocation[0] == 1 ) {
      // Sleep for the required time
      usleep((SSD_ACCESS_SLEEP_TIME + HDD_ACCESS_SLEEP_TIME) * 1000000);
      // Requested operation: HDD -> SSD 
      ssdStorage[destinationLocation[1]] = hddStorage[startLocation[1]];
      hddStorage[startLocation[1]] = 0;
      // Update the page table
      updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
    }
  } else if( levelOfSwappingNeeded == 2 ) {
    // Could only be RAM -> HDD
    if( startLocation[0] == 0 && destinationLocation[0] == 2 ) {
      // We've got the operation: RAM -> HDD
      // TODO: Check if SSD is empty or not
      // STEP 1: Sleep for the required time
      usleep((RAM_ACCESS_SLEEP_TIME + HDD_ACCESS_SLEEP_TIME) * 1000000);
      // STEP 2: Make the swap
      hddStorage[destinationLocation[1]] = ramStorage[startLocation[1]];
      // TODO: Update Page Table
    }
  } else if( levelOfSwappingNeeded == (-2) ) {
    // Could only be HDD -> RAM
    if( startLocation[0] == 2 && destinationLocation[0] == 0 ) {
      if( RAMMemoryUsed == RAM_STORAGE_COUNT && SSDMemoryUsed == SSD_STORAGE_COUNT ) {
        // Requested Operation is HDD -> RAM and we've a full RAM and SSD
	// STEP 1: Find out a location to swap out a page from SSD to HDD
	// STEP 1a: Find location in HDD
	int *toLocationHDD = findEmptySlotAt(2);
	// STEP 1b: Find a page to evict in SSD
	int *fromLocationEvictSSD = pickAPageToEvict(1);
	// STEP 1c: Perform the swap
	switchToMemoryLocation(fromLocationEvictSSD, toLocationHDD);
	// STEP 2: Find out something to swap out of the RAM
	// STEP 2a: Find out what to evict from RAM
	// We essentially WANT to evict from the location that user wants to swap to 
	int *fromLocationEvictRAM = destinationLocation; 
	// STEP 2b: Resolve location to save to in SSD
	int *toLocationSSD = fromLocationEvictSSD;
	// STEB 2c: Perform the swap
	switchToMemoryLocation(fromLocationEvictRAM, toLocationSSD);
	// STEP 3: Perform the desired swap
	// STEP 3a: Fetch the value at the HDD
	usleep(HDD_ACCESS_SLEEP_TIME * 1000000);
        uint32_t valueAtHDD = hddStorage[startLocation[1]];
	// STEP 3b: Save the location on the RAM
	usleep(RAM_ACCESS_SLEEP_TIME * 1000000);
	ramStorage[destinationLocation[1]] = valueAtHDD;
	// STEP 3c: Update the page table
	updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
      } else if( RAMMemoryUsed == RAM_STORAGE_COUNT && SSDMemoryUsed < SSD_STORAGE_COUNT ) {
        // Requested operation is HDD -> RAM and we've a full RAM but slots in SSD
	// STEP 1: Free up a spot in the RAM by swapping something out to the SSD
	// STEP 1a: Find an empty spot in the SSD
	int *freeSpotSSD = findEmptySlotAt(1);
	// STEP 1b: Find a page to evict from RAM
	// We essentially WANT to evict from the location that user wants to swap to 
	int *evictFromRAM = destinationLocation; 
	// STEP 1c: Perform the swap
	switchToMemoryLocation(evictFromRAM, freeSpotSSD);
	// STEP 2: Perform the actual swap
	// STEP 2a: Fetch the actual value from the HDD
	usleep(HDD_ACCESS_SLEEP_TIME * 1000000);
	uint32_t valueAtHDD = hddStorage[startLocation[1]];
	// STEP 2b: Save the location on the RAM
	usleep(RAM_ACCESS_SLEEP_TIME * 1000000);
	ramStorage[destinationLocation[1]] = valueAtHDD;
	// STEP 2c: Update the page table
        updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
      } else if( RAMMemoryUsed < RAM_STORAGE_COUNT && SSDMemoryUsed < SSD_STORAGE_COUNT ) {
        // We've got slots in RAM and SSD and HDD -> RAM has been requested
	// STEP 1: Directly populate the requested page from HDD to the RAM
	// STEP 1a: Fetch the actual value from the HDD
	usleep(HDD_ACCESS_SLEEP_TIME * 1000000);
	uint32_t valueAtHDD = hddStorage[startLocation[1]];
	// STEP 1b: Save the location on the RAM
	usleep(RAM_ACCESS_SLEEP_TIME * 1000000);
	ramStorage[destinationLocation[1]] = valueAtHDD;
	// STEP 1c: Update the page table
        updatePageTable(startLocation[0], startLocation[1], destinationLocation[0], destinationLocation[1]);
      }
    }
  }
}

int getPageTableNumber(int level, int index) {
  // Loop over all the page table entries to locate this page
  int ctr = 0; // Loop counter
  for( ; ctr < TOTAL_SYSTEM_MEM_STORAGE; ctr++ ) {
    if( pageTable[ctr].level == level && pageTable[ctr].index == index )
      return ctr; // We found a match and now return it
  }
  // Else, return -1
  return -1;
}

void updatePageTable(int sLevel, int sIndex, int fLevel, int fIndex) {
  // STEP 1: Find the required page number
  int pageTableNumber = getPageTableNumber(sLevel, sIndex);
  // STEP 2: See if we have a valid page number
  if(pageTableNumber == (-1)) return; // Exit without attempting to make any changes
  // STEP 3: Update the page table
  pageTable[pageTableNumber].level = fLevel; // Udpate the level of storage
  pageTable[pageTableNumber].index = fIndex; // Update the index location at which the item can be found
  // STEP 4: Update the memory reference as well
  if(fLevel == 0)
    pageTable[pageTableNumber].memory = &ramStorage[fIndex];
  else if(fLevel == 1)
    pageTable[pageTableNumber].memory = &ssdStorage[fIndex];
  else if(fLevel == 2)
    pageTable[pageTableNumber].memory = &hddStorage[fIndex];
}

// Testing functions and some helper functions
void memoryMaxer() {
  vAddr indexes[1000];
  uint32_t i = 0;
  for ( ; i < 1000; ++i) {
    indexes[i] = create_page();
    int valid = 0; 
    uint32_t value = get_value(indexes[i], &valid);
    if(!valid) return; // Address not found
    value = (i * 3);
    store_value(indexes[i], &value);
  }
  for (i = 0; i < 1000; ++i) {
    free_page(indexes[i]);
  }
}

void memoryTester() {
  vAddr indexes[1000];
  uint32_t i = 0;
  for( ; i < 1000; i++ )
    indexes[i] = create_page();
  for( i = 0; i < 1000; ++i ) {
    uint32_t value = i;
    store_value(indexes[i], &value);
  }
  for( i = 0; i < 1000; i++ )
    free_page(indexes[i]);
}

long timedifference(struct timeval endTime, struct timeval startTime) {
  return ( ( ( (long)endTime.tv_sec * 1000) +   ((long)endTime.tv_usec / 1000)) -
           (((long)startTime.tv_sec * 1000) + ((long)startTime.tv_usec / 1000)) );
}

// Main to test this algorithm
int main() {
  // Setup timevals
  struct timeval startTime, endTime;
  // Run a variety of functions
  __initialize(); // Initialize the page fault simulator
  printf("=-=-=-=-= MEMORY MAXER\n\n");
  gettimeofday(&startTime, NULL); // Measure time performance
  memoryMaxer();
  gettimeofday(&endTime, NULL); // Measure time performace
  printf("=-=-=-=-= Memory Maxer Statistics:\n");
  printf(" Time taken for execution with random algorithm for picking pages = %ld msec\n\n", timedifference(endTime, startTime));
  printf("=-=-=-=-= PERSONAL FUNCTION 1\n\n");
  gettimeofday(&startTime, NULL); // Measure time performance
  memoryTester(); 
  gettimeofday(&endTime, NULL); // Measure time performace
  printf("=-=-=-=-= Personal Function 1 Statistics:\n");
  printf(" Time taken for execution with random algorithm for picking pages = %ld msec\n\n", timedifference(endTime, startTime));
  return 0;
}
