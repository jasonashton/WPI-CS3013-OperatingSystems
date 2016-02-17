// For semaphore based functions
#include <semaphore.h>
// For pthread_create(), pthread_join()
#include <pthread.h>
// For sleep()
#include <unistd.h>
// For standard functions
#include <stdlib.h>
#include <stdio.h>

// Semaphore typedefs
typedef sem_t semaphore;
// Constant Definitions
#define PRINT_DEBUGGING_INFO 0
#define PRINT_PRIORITY_LINE 0
#define AIRPLANES_AIRSPACE 25
#define RUNWAYS_AIRSPACE 3 
#define MIN_AIRPLANE_FUEL 300 
#define MAX_AIRPLANE_FUEL 701
#define FIXED_AIRPLANE_DEPLETION_RATE 3 
#define MIN_AIRPLANE_TIME_TILL_LANDING_TO_RUNWAY 6
#define MAX_AIRPLANE_TIME_TILL_LANDING_TO_RUNWAY 20
#define MIN_AIRSPACE_ENTER_WAIT_TIME 1
#define MAX_AIRSPACE_ENTER_WAIT_TIME 10
#define MIN_EMERGENCY_RAND 0
#define MAX_EMERGENCY_RAND 1000
#define THRESHOLD_EMERGENCY_RAND 50
#define MIN_EMERGENCY_WAIT_TIME 8
#define MAX_EMERGENCY_WAIT_TIME 20
#define BUFFER_DANGER_ZONE_FUEL 0 
#define FIXED_CLEARING_RUNWAY_TIME 10
// Structure Definitions
typedef struct {
  int id; // A unique identifier for each airplane
  int fuel; // Minimum 100 at the start of the simulation. Tending to zero means that fuel is depleting
  int rate_of_depletion; // Minimum 2. Fuel that depletes per second of the airplane
  int time_for_landing_to_runway; // Time taken for an airplane to clear the runway
  int priority; // -2: Cleared the runway already (not to be considered now); -1: Still not arrived to airspace; 0 => In landing sequence already; 1 => Highest priority
  int force_emergency_priority; // 0 => No emergency priority was assigned; 1 => An emergency priority was assigned
  int landing_in_process; // 0 => No landing/clearing in process; 1 => Airplane is queued for that runway; 2 => Airplane is landing/clearing; 3 => Airplane is done landing
  int in_airspace; // 0 => Airplane is NOT in the airspace; 1 => Airplane is in the airspace
  int blocked_runway_for_landing; // -1: Landing sequence not yet initiated; OTHER => Runway index
  int landing_time_remaining; // -1: Landing sequence not yet initiated; OTHER => Remaining time to land + clear the runway
  pthread_t plane_thread; // Holds the airplane thread
} airplane;
// Global state visible to all aircrafts
semaphore *mutex;
semaphore *airplane_lock;
semaphore *runway_lock;
airplane *all_airplanes;

// Plane functions prototypes
void *plane_invoke_function(void *args);
void planes_reset_priority();
void planes_reorder();
void plane_deplete_fuel(int airplaneIndex);
int plane_resolve_runway_available();
int plane_sleep_till_get_runway(int airplane_index, int airplaneId);
int plane_is_fuel_zero(int airplane_index);
void plane_land_sequence(int airplane_index, int runwayToLandOn, int airplaneID);
int plane_number_emergency_planes_landing();
void simulation_print_priorities();
// Some basic plane functions
void *plane_invoke_function(void *args) {
  // Extract the plane that the thread is being run on now
  airplane *an_airplane = (airplane*)args;
  int airplane_index = an_airplane->id - 1;
  // Find a random time to sleep before an airplane requests to enter airspace
  int sleepSecs = (MIN_AIRSPACE_ENTER_WAIT_TIME + (rand() % (MAX_AIRSPACE_ENTER_WAIT_TIME - MIN_AIRSPACE_ENTER_WAIT_TIME)));
  sleep((unsigned int)sleepSecs); // Make the program sleep for those many seconds now
  // Now announce arrival into the airspace and set the struct
  //  SEMAPHORE: Grab mutex and airplane lock
  sem_wait(mutex);
  sem_wait(airplane_lock + airplane_index);
  //  SEMAPHORE CODE BLOCK END
  all_airplanes[airplane_index].in_airspace = 1; // Set the airplane struct
  //  SEMAPHORE: Release mutex and airplane lock
  sem_post(airplane_lock + airplane_index);
  sem_post(mutex);
  //  SEMAPHORE CODE BLOCK END
  printf("[PLANE]: Plane with ID %d just entered the airspace with fuel %d.\n", an_airplane->id, an_airplane->fuel);
  // Now see if an emergency landing is needed or not by randomizing it
  int needsEmergencyInFuture = 0; // 0 => FALSE; 1 => TRUE
  if( THRESHOLD_EMERGENCY_RAND > (MIN_EMERGENCY_RAND + (rand() % (MAX_EMERGENCY_RAND - MIN_EMERGENCY_RAND))) )
    needsEmergencyInFuture = 1; // If we are under the threshold, then the plane needs an emergency landing in the future
  // Sleep for some random time before asking for emergency
  if( needsEmergencyInFuture ) { 
    // If it is an emergency, we gotta report it at a random time
    int sleepSecsEmer = (MIN_EMERGENCY_WAIT_TIME + (rand() % (MAX_EMERGENCY_WAIT_TIME - MIN_EMERGENCY_WAIT_TIME)));
    // Deplete the fuel till we report the emergency
    int runwayToLandOn = -1; // Will store the runway that we'll be landing on
    while( sleepSecsEmer > 0 ) {
      if(   all_airplanes[airplane_index].fuel < 0 ||                           // If we run out of fuel; or,
          ( all_airplanes[airplane_index].priority == 1 &&                      // the priority of this flight is 1 AND
	    ( runwayToLandOn = plane_resolve_runway_available() ) != (-1) &&    // we got a runway to land on, then exit this loop AND
	    ( plane_number_emergency_planes_landing() == 0 ) ) )                // we can land because no emergency plane is in the process of landing

        break; // Exit the loop

      // Decrement the number of seconds to sleep and sleep for one second
      sleepSecsEmer--;
      sleep(1);
    }
    
    // Check the reason why we exitted the while loop
    // Possible reasons: We've slept and now its time to report the emergency OR we ran out of fuel OR it is our chance to land
    // CHECKING if we've run out of fuel
    if( plane_is_fuel_zero(airplane_index) ) {
      exit(1); // We ran out of fuel and now we gotta crash
    }
    // CHECKING if it is our chance to land
    if( runwayToLandOn != (-1) ) {
      simulation_print_priorities();
      // We got a runway to land on before declaring the emergency
      plane_land_sequence(airplane_index, runwayToLandOn, an_airplane->id);
      // Exit this thread as we're done here
      pthread_exit(0); // Successfully exit the thread
    }
    // CHECKING if we've to report the emergency or not; If yes, output our intentions and move on
    // This is the last reason we could've quit the loop. So let us declare the emergency, re-organize the priorities and then cut-off for
    //  an emergency
    
    // Output our intentions to the user
    printf("[PLANE]: Plane with ID %d just requested for an emergency landing.\n", an_airplane->id);
    // Move onto setting the flag and then the priority
    //  SEMAPHORE: Grab mutex and airplane lock
    sem_wait(mutex);
    sem_wait(airplane_lock + airplane_index);
    //  SEMAPHORE CODE BLOCK END
    all_airplanes[an_airplane->id - 1].force_emergency_priority = 1;
    //  SEMAPHORE: Release mutex and airplane lock
    sem_post(airplane_lock + airplane_index);
    sem_post(mutex);
    //  SEMAPHORE CODE BLOCK END

    // Take over the airspace
    // STEP 1: Set the priority by looking at other priorities and emergency statuses
    int emergencyCount = 0; // Count of the number of emergency planes that can be tracked for now
    int ctr = 0; // Loop counter
    for( ; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
      if( all_airplanes[ctr].in_airspace && all_airplanes[ctr].force_emergency_priority )
        emergencyCount++; // If the airplane is in the airspace and it had already declared an emergency
    }
    // STEP 2: Take the IDs of all the emergency planes and store them in an array
    int *emergencyPlaneIDs = (int*)malloc(sizeof(int) * emergencyCount);
    int arrayCounter = 0;
    for( ctr = 0; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
      if( all_airplanes[ctr].in_airspace && all_airplanes[ctr].force_emergency_priority ) {
        emergencyPlaneIDs[arrayCounter++] = all_airplanes[ctr].id;
      }
    }
    // STEP 3: Count the number of airplanes already in the landing sequence and store them in an array
    int *emergencyInLandingSequencePlaneIDs = (int*)malloc(sizeof(int) * RUNWAYS_AIRSPACE);
      // As the number of planes in emergency landing sequence cannot exceed the total number of runways we got
    for( ctr = 0; ctr < RUNWAYS_AIRSPACE; ctr++ )
      emergencyInLandingSequencePlaneIDs[ctr] = -1; // Default all the values to -1
    arrayCounter = 0; // To keep track of array count
    for( ctr = 0; ctr < emergencyCount; ctr++ ) {
      if( all_airplanes[emergencyPlaneIDs[ctr]].priority == 0 ) {
        // This plane has already started its landing sequence 
	emergencyInLandingSequencePlaneIDs[arrayCounter++] = emergencyPlaneIDs[ctr];
      }
    }
    // STEP 4: Make this plane priority ID ONE and push everything back by one
    //  OUR CUT-OFF ATTEMPT
    // Grab the mutex lock and airplane lock
    //  SEMAPHORE: Grab the mutex and airplane lock
    sem_wait(mutex);
    sem_wait(airplane_lock + airplane_index);
    //  SEMAPHORE CODE BLOCK END
    all_airplanes[airplane_index].priority = 1; // Giving our current airplane top priority
    // Release the semaphore lock for our airplane and push back the rest
    //  SEMAPHORE: Release the mutex for airplane lock
    sem_post(airplane_lock + airplane_index);
    //  SEMAPHORE CODE BLOCK END

    for( ctr = 0; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
      if( all_airplanes[ctr].in_airspace && all_airplanes[ctr].force_emergency_priority 
                                         && ctr != airplane_index
					 && all_airplanes[ctr].priority > 0 ) {
        // The current airplane IS in the airspace AND has requested for emergency landing in the past
	//  AND it is not the current airplane because we've forced its priority to one
	//  AND the plane is not currently in the landing sequence (priority greater than 0)

	// Grab the airplane lock
	//  SEMAPHORE: Grab the airplane lock
	sem_wait(airplane_lock + ctr);
	//  SEMAPHORE CODE BLOCK END

	all_airplanes[ctr].priority += 1; // Bump up the priority by one

	// Release the airplane lock
	//  SEMAPHORE: Release the airplane lock
	sem_post(airplane_lock + ctr);
	//  SEMPAHORE CODE BLOCK END
      }
    }

    // Release the mutex lock now
    //  SEMAPHORE: Release the mutex lock now
    sem_post(mutex);
    //  SEMAPHORE CODE BLOCK END
  }
  // Request re-prioritizing the planes now
  planes_reset_priority();
  planes_reorder();
  // Print all the airplane information if debugging information was toggled on
  simulation_print_priorities();
  // Print our priority
  if( PRINT_PRIORITY_LINE )
    printf("[PLANE]: Plane with ID %d was given priority %d.\n", all_airplanes[airplane_index].id, all_airplanes[airplane_index].priority); 

  // Run fuel check now and wait till we have priority 1 now
  int runwayToLandOn = plane_sleep_till_get_runway(airplane_index, an_airplane->id); // Will store the runway we'll be landing on

  // Wait loop exitted => Either fuel is negative OR we are now ready for landing
  // Check if the plane has run out of fuel
  if( plane_is_fuel_zero(airplane_index) ) {
    exit(1); // If the function returned 1 means we gotta exit the function
  }
  // Run the landing sequence
  plane_land_sequence(airplane_index, runwayToLandOn, an_airplane->id);
  simulation_print_priorities();
  // Exit the thread with success
  pthread_exit(0); // Successfully exitted this thread
}

void planes_reset_priority() {
  // Grab semaphore lock on the mutex
  //  SEMAPHORE: Grab the mutex lock
  sem_wait(mutex);
  //  SEMAPHORE CODE BLOCK END

  // Don't change the priority of any emergency planes
  int airplane_loopctr = 0; // Loop counter to loop over the airplane struct
  for( ; airplane_loopctr < AIRPLANES_AIRSPACE; airplane_loopctr++ ) {
    if( all_airplanes[airplane_loopctr].landing_in_process )
      continue; // If 1 or 2 => Landing in progress or done, then skip over this plane

    // Grab the semaphore for the current airplane as we could be making changes to the airplanes properties
    //  SEMAPHORE: Grab the airplane lock for the current airplane
    sem_wait(airplane_lock + airplane_loopctr);
    //  SEMAPHORE CODE BLOCK END

    if( !all_airplanes[airplane_loopctr].force_emergency_priority && all_airplanes[airplane_loopctr].in_airspace && all_airplanes[airplane_loopctr].priority != 0 )
      all_airplanes[airplane_loopctr].priority = -1; // An emergency priority lock was not requested so change to -1 to be available to reordering
    if( !all_airplanes[airplane_loopctr].in_airspace && all_airplanes[airplane_loopctr].in_airspace != (-2) )
      all_airplanes[airplane_loopctr].priority = -1; // Plane hasn't even entered the airspace, give it -1 priority

    // Relase the airplane lock as we're done making changes to this airplane
    //  SEMAPHORE: Release the airplane lock for the current airplane
    sem_post(airplane_lock + airplane_loopctr);
    //  SEMAPHORE CODE BLOCK END
  }

  // We are done changing values in the shared region, release the lock now
  //  SEMAPHORE: Release the mutex lock
  sem_post(mutex);
  //  SEMAPHORE BLOCK END
}

void planes_reorder() {
  // Check which planes will enter the danger zone first 
  // STEP 1: Count the number of non-emergency planes and spawned airplaes AND that are NOT landing right now
  int ctr = 0; // Loop counter
  int nonEmergencyCount = 0; // Number of airplanes with no emergency on
  for( ; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
    if( !all_airplanes[ctr].force_emergency_priority && all_airplanes[ctr].in_airspace && all_airplanes[ctr].priority != 0 ) 
      nonEmergencyCount++; // The flight doesn't have an emergency
  }
  // STEP 1b: Count the number of airplanes spawned in the airspace AND that are NOT landing right now
  int spawnedInAirspaceCount = 0;
  for( ctr = 0; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
    if( all_airplanes[ctr].in_airspace && all_airplanes[ctr].priority != 0 )
      spawnedInAirspaceCount++;
  }
  // STEP 2: Make a list of all airplanes with their fuels and IDs
  int *nonEmergencyPlaneIDs = (int*)malloc(sizeof(int) * nonEmergencyCount);
  int *nonEmergencyPlaneFuel = (int*)malloc(sizeof(int) * nonEmergencyCount); // Will keep track of the amount of fuel for each plane before it enters the danger zone
  int smallerArrayCtr = 0; // Array counter for the above two defined arrays
  for( ctr = 0; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
    if( !all_airplanes[ctr].force_emergency_priority && all_airplanes[ctr].in_airspace && all_airplanes[ctr].priority != 0 ) {
      nonEmergencyPlaneIDs[smallerArrayCtr] = all_airplanes[ctr].id;
      nonEmergencyPlaneFuel[smallerArrayCtr] = all_airplanes[ctr].fuel - ((all_airplanes[ctr].rate_of_depletion * all_airplanes[ctr].time_for_landing_to_runway) + BUFFER_DANGER_ZONE_FUEL);
        // We are calculating the amount of fuel left before the airplane enters the danger zone minus the buffer danger zone fuel
      smallerArrayCtr++; // Increment the array counter
    }
  }
  // STEP 3: Now sort the lists (Bubble sort is being done on the fuel)
  int ctr2 = 0; // Loop counter 2
  for( ctr = 0; ctr < (nonEmergencyCount - 1); ctr++ ) {
    for( ctr2 = 0; ctr2 < (nonEmergencyCount - ctr - 1); ctr2++ ) {
      if(nonEmergencyPlaneFuel[ctr2] > nonEmergencyPlaneFuel[ctr2 + 1]) {
        // Swap IDs
	int swapID = nonEmergencyPlaneIDs[ctr2];
	nonEmergencyPlaneIDs[ctr2] = nonEmergencyPlaneIDs[ctr2 + 1];
	nonEmergencyPlaneIDs[ctr2 + 1] = swapID;
	// Swap the fuel now
	int swapFuel = nonEmergencyPlaneFuel[ctr2];
	nonEmergencyPlaneFuel[ctr2] = nonEmergencyPlaneFuel[ctr2 + 1];
	nonEmergencyPlaneFuel[ctr2 + 1] = swapFuel;
      }
    }
  }
  // STEP 4: Set up the priority now

  // We will down the mutex semaphore as we'll be making changes to the priorities
  //  SEMAPHORE: Grabbing the mutex lock
  sem_wait(mutex);
  //  SEMAPHORE CODE BLOCK END
  for( ctr = 0; ctr < nonEmergencyCount; ctr++ ) {
    int planeIndex = nonEmergencyPlaneIDs[ctr] - 1;
    // Grab the airplane lock for current airplane
    //  SEMAPHORE: Grab the airplane lock
    sem_wait(airplane_lock + planeIndex);
    //  SEMAPHORE CODE BLOCK END

    // Set up the priority of the plane and save it to the respective ID
    all_airplanes[planeIndex].priority = (spawnedInAirspaceCount - nonEmergencyCount) + ctr + 1;
    // We calculate this by doing this:
    // We have the count of the number of planes that have spawned in the airspace and then subtract the number of planes that are NOT in emergency
    //  state and then this fetches us the number of planes in the emergency state. Add the counter and now we get the required number for the
    //  priority. Adding 1 becuase our priorities start from 1

    // Release the semaphore lock for the current airplane
    //  SEMAPHORE: Release the airplane lock
    sem_post(airplane_lock + planeIndex);
    //  SEMAPHORE CODE BLOCK END
  }

  // Release the mutex lock as we're done changing values
  //  SEMAPHORE: Release the mutex lock
  sem_post(mutex);
  //  SEMAPHORE CODE BLOCK END
}

void plane_deplete_fuel(int airplaneIndex) {
  // We are waiting for our priority to become 1. Meanwhile, deplete the fuel
  //  SEMAPHORE: Grab the mutex and airplane lock
  sem_wait(mutex);
  sem_wait(airplane_lock + airplaneIndex);
  //  SEMPAHORE CODE BLOCK END

  all_airplanes[airplaneIndex].fuel -= all_airplanes[airplaneIndex].rate_of_depletion;
  // Deplete the fuel by its rate and then sleep for a second after releasing the mutex and airplane lock

  //  SEMAPHORE: Release the mutex and airplane lock
  sem_post(airplane_lock + airplaneIndex);
  sem_post(mutex);
  //  SEMAPHORE CODE BLOCK END
}

int plane_resolve_runway_available() {
  int ctr = 0; // Loop counter
  int available_runway = -1; // Keeps track of the available runway
  int *tempval = (int*)malloc(sizeof(int)); // Will hold the semaphore's value
  for( ; ctr < RUNWAYS_AIRSPACE; ctr++ ) {
    sem_getvalue((runway_lock + ctr), tempval); // Store the semaphore's value
    if( *tempval == 1 ) {
      // We've found an open runway. Set and break
      available_runway = ctr; // Set
      break; // Break out of the loop
    }
  }
  // Return the required value
  return available_runway;
}

int plane_sleep_till_get_runway(int airplane_index, int airplaneId) {
  // Start processing
  int runwayToLandOn = -1; // The runway that the plane will be landing on
  int printedDangerZone = 0; // Have we already told the user if this plane is in the danger zone or not
  while(   all_airplanes[airplane_index].fuel >= 0 &&                              // We run the loop as long as we've got more than zero fuel; and,
         (   all_airplanes[airplane_index].priority != 1 ||                        // it is NOT our turn to land; or,
	   ( all_airplanes[airplane_index].priority == 1 &&                        // it IS our tunr to land (as indicated by the priority); and,
	     ( ( (runwayToLandOn = plane_resolve_runway_available()) == (-1) ) ||  // we DO NOT have a runway to land on; or,
	     ( plane_number_emergency_planes_landing() != 0 ) ) ) ) )              // we do have some emergency planes in the landing process and hence we cannot land
  {
    plane_deplete_fuel(airplane_index); // Deplete the fuel of this current aircraft
    // Check if this current airplane is in the danger zone or not
    if( all_airplanes[airplane_index].fuel < ((all_airplanes[airplane_index].rate_of_depletion * all_airplanes[airplane_index].time_for_landing_to_runway) + BUFFER_DANGER_ZONE_FUEL) ) {
      // This means we are in danger zone
      if(!printedDangerZone) {
        printf("[PLANE]: Plane with ID %d is in danger zone and currently carries fuel %d.\n", airplaneId, all_airplanes[airplane_index].fuel);
	printedDangerZone = 1; // We've now warned the user. No need for future notifications
      }
    }
    // Sleep this thread for a second and re-check if we're up for landing and then fuel depletion
    sleep(1);
  }
  // Return the function
  return runwayToLandOn;
}

int plane_is_fuel_zero(int airplane_index) {
  // It is either our time to land or we ran out of fuel
  // Checking for running out of fuel
  if( all_airplanes[airplane_index].fuel < 0 ) {
    // The plane has crashed. Warn the user and exit all the other threads
    printf("[PLANE]: Plane with ID %d has crashed as it ran out of fuel!\n", all_airplanes[airplane_index].id);
    // Exit the program
    return 1; // => We gotta exit the program
  }
  // Fuel is not zero
  return 0; // => We don't exit the program. Move onto landing the plane
}

void plane_land_sequence(int airplane_index, int runwayToLandOn, int airplaneID) {
  // Grab the lock on the runway immediately
  //  SEMAPHORE: Lock on the runway that we ant to land on
  sem_wait(runway_lock + runwayToLandOn);
  //  SEMAPHORE CODE BLOCK END

  // STEP 1: Start the landing process by setting the required variables AND decrementing the priority in each of the airplanes
  // Grab the mutex
  //  SEMAPHORE: Grab a lock on the mutex variable
  sem_wait(mutex);
  //  SEMAPHORE CODE BLOCK END

  all_airplanes[airplane_index].priority = 0; // We've starte dthe landing sequence
  all_airplanes[airplane_index].landing_in_process = 2; // We're now in the process of landing/clearing
  all_airplanes[airplane_index].in_airspace = 1; // We are STILL in the airspace but descending for the landing procedure
  all_airplanes[airplane_index].blocked_runway_for_landing = runwayToLandOn; // We are landing on this runway
  all_airplanes[airplane_index].landing_time_remaining = all_airplanes[airplane_index].time_for_landing_to_runway + FIXED_CLEARING_RUNWAY_TIME;

  // Decrement the priorities
  int ctr = 0; // Loop counter
  for( ; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
    if( all_airplanes[ctr].in_airspace && all_airplanes[ctr].priority != 0 ) {
      // Grab a lock for this airplane
      //  SEMAPHORE: Grab the airplane lock
      sem_wait(airplane_lock + ctr);
      //  SEMAPHORE CODE BLOCK ENDS

      all_airplanes[ctr].priority--;
      
      // Release the semaphore lock on the airplane now
      //  SEMAPHORE: Release the airplane lock
      sem_post(airplane_lock + ctr);
      //  SEMAPHORE CODE BLOCK ENDS
    }
  }

  // Release the mutex lock
  //  SEMAPHORE: Release the mutex variable lock
  sem_post(mutex);
  //  SEMAPHORE CODE BLOCK END

  // STEP 2: Grab the landing time and landing for the given time
  int fuelBeforeLanding = all_airplanes[airplane_index].fuel;
  int timeToLand = all_airplanes[airplane_index].time_for_landing_to_runway;
  // Tell the user what is happening
  printf("[PLANE]: Plane with ID %d is landing on runway %d. Fuel before the landing sequence started was %d and time to land is %d.\n", airplaneID, runwayToLandOn + 1, fuelBeforeLanding, timeToLand);
  // Loop over the landing time
  while( timeToLand > 0 ) {
    sleep(1); // Sleep for one second
    plane_deplete_fuel(airplane_index); // Deplete the fuel of this current aircraft
    // Check if the fuel is there or not
    if( plane_is_fuel_zero(airplane_index) ) {
      // We encountered a crash during the landing sequence as we ran out of fuel
      printf("[SIMULATION]: Plane with ID %d crashed while it was attempting the descent during the landing sequence.\n", all_airplanes[airplane_index].id);
      exit(1); // Exit all the threads and the process itself
    }
    // Also update the attributes
    //  SEMAPHORE: Grab the mutex lock
    sem_wait(mutex);
    //  SEMAPHORE CODE BLOCK END

    all_airplanes[airplane_index].landing_time_remaining--;

    //  SEMAPHORE: Release the mutex lock
    sem_post(mutex);
    //  SEMAPHORE CODE BLOCK END

    // Help the next iteration
    timeToLand--; // Decrement the time to land
  }

  // STEP 3: We've touched the ground. How its time to clear the runway
  int fuelAfterTouchDown = all_airplanes[airplane_index].fuel;
  // Tell the user what is happening
  printf("[PLANE]: Plane with ID %d has touched down. Fuel after the landing sequence is %d. The plane will now be clearing runway %d.\n", airplaneID, fuelAfterTouchDown, runwayToLandOn + 1);
  // Sleep this thread for the time it takes for it clear the runway
  while( all_airplanes[airplane_index].landing_time_remaining > 0 ) {
    sleep(1); // Sleep for one second during the clearing process
    // Update the struct attributes
    //  SEMAPHORE: Grab the mutex lock
    sem_wait(mutex);
    //  SEMAPHORE CODE BLOCK END
    
    all_airplanes[airplane_index].landing_time_remaining--;

    //  SEMAPHORE: Release the mutex lock
    sem_post(mutex);
    //  SEMAPHORE CODE BLOCK END
  }

  // STEP 4: We're done clearing the runway
  printf("[PLANE]: Plane with ID %d has cleared the runway. The runway %d is now open for other airplane(s) in the airspace.\n", airplaneID, runwayToLandOn + 1);

  // STEP 5: Update some attributes in the struct
  // First grab the mutex lock
  //  SEMAPHORE: Grab the mutex lock to make changes to the struct
  sem_wait(mutex);
  //  SEMAPHORE CODE BLOCK END

  all_airplanes[airplane_index].priority = -2; // We've cleared the runway and we are no longer of concern
  all_airplanes[airplane_index].landing_in_process = 3; // We are done landing this airplane
  all_airplanes[airplane_index].in_airspace = 0; // We are no longer in the airspace and not of concern to the other airplanes
  all_airplanes[airplane_index].blocked_runway_for_landing = -2; // We haven't blocked any runway for landing as we're done landing

  // Release the mutex lock
  //  SEMAPHORE: Release the lock as we're done cleaning up the struct
  sem_post(mutex);
  //  SEMAPHORE CODE BLOCK END

  // STEP 6: Release the locks on the airplane and the runway
  // Unlock the runway for other planes to use
  //  SEMAPHORE: Unlock the runway for it to be used by other airplanes
  sem_post(runway_lock + runwayToLandOn);
  //  SEMAPHORE CODE BLOCK END
}

int plane_number_emergency_planes_landing() {
  // Count the number of emergency planes landing
  // This means we count the number of planes with priority 0 and in the emergency state
  int ctr = 0; // Loop counter
  int count = 0; // To keep track of the number of planes to be returned
  for( ; ctr < AIRPLANES_AIRSPACE; ctr++ ) {
    if( all_airplanes[ctr].priority == 0 && all_airplanes[ctr].force_emergency_priority == 1 )
      count++; // Increment the value as we found a plane meeting our requirements
  }
  // Return the required value now
  return count;
}

void simulation_print_priorities() {
  if( !PRINT_DEBUGGING_INFO ) return;
  // Print the priorities of the planes
  printf("[SIMULATION]: Priorities of landing planes: {id, fuel, priority} = ");
  // Loop over now
  int ctr = 0; // Loop counter
  for( ; ctr < AIRPLANES_AIRSPACE; ctr++ )
    printf("{%d, %d, %d}%s", ctr + 1, all_airplanes[ctr].fuel, all_airplanes[ctr].priority, ((ctr != AIRPLANES_AIRSPACE - 1) ? ", " : "\n"));
}

// The main program
int main(int argc, char **argv) {
  if(argc != 1) {
    printf("Usage: %s\n", argv[0]);
    return 1; // Indicate FAILURE
  }
  // No problems. Let us define the semaphores
  mutex = (semaphore*)malloc(sizeof(semaphore)); // Create a mutex and set it to a default value
  sem_init(mutex, 0, 1); // Visible to all threads of the process AND by default anyone should be able to lock shared region 
  airplane_lock = (semaphore*)malloc(sizeof(semaphore) * AIRPLANES_AIRSPACE); 
  runway_lock = (semaphore*)malloc(sizeof(semaphore) * RUNWAYS_AIRSPACE);
  // Initialize the semaphores
  int ctr = 0; // A loop counter
  for( ; ctr < AIRPLANES_AIRSPACE; ctr++ )
    sem_init((airplane_lock + ctr), 0, 1); // Visible to all threads AND the default value is that anybody should be able to lock the airplane
  for( ctr = 0; ctr < RUNWAYS_AIRSPACE; ctr++ )
    sem_init((runway_lock + ctr), 0, 1); // Visible to all threads AND the default value is tha tanybody should be able to lock on a runway
  // Randomly generate and fill in airplane structure now
  int airplane_loopctr = 0; // Loop counter to loop over the airplane struct
  all_airplanes = (airplane*)malloc(sizeof(airplane) * AIRPLANES_AIRSPACE);
  srand((unsigned)time(NULL)); // Randomized the seed
  // Now loop through to randomly fill in values
  for( ; airplane_loopctr < AIRPLANES_AIRSPACE; airplane_loopctr++ ) {
    all_airplanes[airplane_loopctr].id = airplane_loopctr + 1;
    all_airplanes[airplane_loopctr].fuel = (MIN_AIRPLANE_FUEL + (rand() % (MAX_AIRPLANE_FUEL - MIN_AIRPLANE_FUEL)));
    all_airplanes[airplane_loopctr].rate_of_depletion = FIXED_AIRPLANE_DEPLETION_RATE; // For the time being assuming to be fixed depletion rate
    all_airplanes[airplane_loopctr].time_for_landing_to_runway = (MIN_AIRPLANE_TIME_TILL_LANDING_TO_RUNWAY + (rand() % (MAX_AIRPLANE_TIME_TILL_LANDING_TO_RUNWAY - MIN_AIRPLANE_TIME_TILL_LANDING_TO_RUNWAY)));
    all_airplanes[airplane_loopctr].priority = -1; // Not yet arrived
    all_airplanes[airplane_loopctr].force_emergency_priority = 0; // It is not in the emergency state
    all_airplanes[airplane_loopctr].landing_in_process = 0; // This aircraft is not landing
    all_airplanes[airplane_loopctr].in_airspace = 0; // We have still not spawned in the airspace
    all_airplanes[airplane_loopctr].blocked_runway_for_landing = -1; // We've still not starte dthe landing process
    all_airplanes[airplane_loopctr].landing_time_remaining = -1; // The landing sequence has not yet been initiated
  }

  // Spawn a thread for each of the airplanes
  for( airplane_loopctr = 0; airplane_loopctr < AIRPLANES_AIRSPACE; airplane_loopctr++ ) {
    int threadStatus = pthread_create(&all_airplanes[airplane_loopctr].plane_thread, NULL, plane_invoke_function, (void*)&all_airplanes[airplane_loopctr]);
    // Check if the spawning was successful or not
    if(threadStatus) {
      printf("Failed to create a pthread, error number: %d\n", threadStatus);
      return 1; // Exit the main with a failure
    }
  }
  // Once we've spawned all the threads, let them join on with the main thread, that's me
  for( airplane_loopctr = 0; airplane_loopctr < AIRPLANES_AIRSPACE; airplane_loopctr++ ) {
    pthread_join(all_airplanes[airplane_loopctr].plane_thread, NULL);
  }
  // Exit now
  printf("[SIMULATION]: The simulation was successfully completed!\n");
}
