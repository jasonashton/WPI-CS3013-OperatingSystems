### PROJECT 3B - Solving synchronization problems

Coded by:
Akshit (Axe) Soota - asoota

**GOAL OF THIS ASSIGNMENT**: <br />
Given a synchronization problem, we were asked to solve it using two different ways:

1. using mutexes and condition variables (e.g., `pthread_mutex_lock`, `pthread_mutex_trylock`, `pthread_mutex_unlock`, `pthread_cond_wait`, and `pthread_cond_signal`)

2. using semaphores (e.g., `sem_wait` and `sem_post`)

**ASSUMPTIONS WHILE CODING**:
- I am prioritizing based on the fuel left till a plane enters the danger zone. Danger zone is defined as the zone where the plane must have begun its landing sequence else it will crash no matter what.
- I am NOT prioritizing the planes based on the total fuel left. If two planes, one with fuel 100 and other with fuel 80 are such that plane with fuel 100 will enter its danger zone first, plane with fuel 100 will be given higher priority. Second priority would be given to the plane with fuel 80.
- When a plane declares an emergency, the plane that declared the emergency LATEST would be landing FIRST even if there are multiple emergency planes that have requested for landing earlier. This is because I blocked ALL other planes from landing. So for the lastest plane, I block all the EARLIER planes even if they are emergency or non-emergency planes. The priority of all planes gets incremented by one and the plane that just requested a priority is set with priority 1.
- If there is more than one plane that has requested an emergency, then they will be queued up with highest importance which means that first all the emergency planes will land and then we will land the normal planes which don't have an emergency situation.
- Our program is designed to assign random landing times to each plane but has a fixed clearing time once the plane has touched down.

**ANSWERING THE QUESTIONS**: <br />
**1) How can you ensure fairness? How is fairness affected by the amount of remaining fuel in each plane?** <br /><br />
**Answer**: This is done by prioritizing planes based on the amount of fuel left till that plane enters the danger zone. A plane which will enter the danger zone first (lesser fuel to entering its danger zone OR will hit the danger zone faster with the given amount of fuel it has) will be given the highest priority (lowest number - 1 in this particular example). The plane which has the most amount of fuel to the danger zone (most fuel before it enters its danger zone OR will hit the danger zone last based on the given amount of fuel it has) will get the least priority (highest number - 25 in this particular example). The planes are prioritized at the start when a new plane enters the airspace or a plane declares an emergency. <br /> <br />

**2) What needs to happen when a plane with an emergency landing arrives? Is it ever possible for an emergency landing to cause another plane to run out of fuel? Are there other scenarios in which plane crashes can occur?** <br /><br />
Answer: When a plane with an emergency landing arrives, it is given the highest priority. If there were other emergency planes waiting, they get pushed back because we were asked to block all other landings. When the emergency plane starts its descent, no other plane can land. <br />
_SCENARIO 1_: If a plane with very little fuel to its danger zone was queued to land but then an emergency plane came in to take the first priority, the plane with the less fuel is most likely to crash because it has been blocked from landing by the emergency plane. <br />
_SCENARIO 2_: Another scenario where the planes could crash is when all planes spawned have relatively close by fuel values, towards the lower end and closer to the danger zone. If there are multiple planes on the verge of entering the danger zone, with the limit on the number of runways, the planes which have low fuel are most likely going to crash.
