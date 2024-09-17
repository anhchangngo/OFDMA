#include <iostream>
#include <list>
#include <vector>
#include <random>
#include "packet.h"
#include "destination.h"
#include "event.h"
#include "transmission.h"

/* A faire
 * - traiter les evenements 1 a 1 --> transmission (schedule next Transmission) arrival (schedule nex arrival)
 */ 

using namespace std;

//Global variables 
double currentTime=0.0;
double startTxTime=0.0, stopTxTime=0.0; 

list<event> events;					//The events (discrete event simulator) - it is not a queue because it is not excatly FIFO (we dequeue at the fornt but insertion of event is not neceessarily at the back/end)
vector<destination> destinations;	//The destinations
list<packet> buffer;				//The buffer

mt19937 engine;						//The random number generator (the discrete one)

//Functions


// double getSampleExpDistribution(double lambda)//lambda is 1/E[X]   
// {
//   std::exponential_distribution<> randomFlot(lambda);
//   return(randomFlot(engine));
// }


int getSampleExpDistribution(double lambda) // lambda is the mean of the Poisson distribution
{
    std::poisson_distribution<> randomInt(lambda);
    return randomInt(engine);
}


// print an event's attribute
void printEvents()
{
  std::list<event>::iterator it;

  for(it=events.begin(); it!=events.end(); it++) cout << *it;   // get output stream, print data of each event instance
}



void insertEvent(class event anEvent)
{
  std::list<event>::iterator it;

  it=events.begin();

  // sort the event
  while(anEvent.m_time > it->m_time && it!=events.end()) it++;  

  cout << "DEBUG: An event is added: " << anEvent << endl;
  events.insert(it,anEvent);      // insert an new event element to the list, with corresponding iterator
}

//Add the next arrival to the list of events for a given destination


// For 1 packet
void addNextArrival(class destination dest)  // given destination
{
	double timeCalculate;
//  event newArrival;	

//  newArrival.m_type=0; 		//arrival
//  newArrival.m_dest=dest;

  switch(dest.m_arrivalDistribution)    // Deterministic or Poisson
  {
    case 0: timeCalculate=currentTime+1.0/dest.m_arrivalRate; break;      // case 0 is deterministic, time = current + 1/ numbers of packet
    case 1:                                                               // Poisson distribution
    {//Les accolades sont necessaires car elle fixe la portee de la declaration de la variable ci-dessous
	    double value=getSampleExpDistribution(dest.m_arrivalRate)/(dest.m_arrivalRate*13);    // time based of arrival rate
	    timeCalculate=currentTime+value;      // time = current time + arrival time
	    cout << "DEBUG: rand exp value: " << value << endl;   // random time caculated
	    break;
    }
    default: cerr << "Error in addNextArrival(): the destination m_arrivalDistribution member is incorrect (=" << dest.m_arrivalDistribution << ")" << endl;
	     exit(1);
  }
 
  //Insert this event in the list
  event newArrival(0, dest, timeCalculate);  // create new event that arrival, given destination, time for delivery 1 packet
  insertEvent(newArrival);                   // insert into event list (sorted)
}




//Schedule the next transmission 
void scheduleNextTransmission(double time)    // Function for plan the next transmission (given time that transmission occured)
{
  event newTrans(1,-1,time);	  // transmission event (1), no destination => -1, given time

//  newTrans.m_type=1; 		//transmission  // the transmission is occured
//  newTrans.m_dest=-1;		//it is not relevent here // no destination
//  newTrans.m_time=time;

  //Insert this event in the list
  insertEvent(newTrans);      // insert event to the list (both sort)

}





// Notable function: STARTING SIMULATION

void simulateQueue(list<packet> &buffer, int discipline)      // given packet list and discipline
{
  int nbOfTransmissions=0;
  int maxTransmission=1000;
  double timeToTransmit;


  //Start of the simulation: we add an arrival for each destination
  if(events.empty())    // the begin, list is empty cause there are no event occur
  {
     for(auto it = std::begin(destinations); it != std::end(destinations); ++it)    // iterate all destination and add arrival to the event list based on time
     {
       cout << "DEBUG: add new arrival\n";
       addNextArrival(*it);               // all arrival is added (starting simulation)
     } 
  }

  printEvents();                    // Print all the event added (all sorted)


  cout<<"Starting while loop" <<endl;
  //The simulator that processes the events
  while (nbOfTransmissions < maxTransmission)
  {
    std::list<event>::iterator it;              // iterate each element on event list

    //What is the next event
	cout<<"[INFO]: Queue component, current list of events"<<endl;        //print current list (after each transmission)
	printEvents();
    it=events.begin();
    currentTime=it->m_time;     
    event thisEvent=*it;		//We copy this event as we remove it from the list      // Copy the current event
    events.pop_front();			//We remove this event as it is process	                // Remove the each first event
	cout<<"[INFO]: Queue component, we are processing "<<endl;
	thisEvent.print();        // Print the info of event that just removed
	printEvents();            // Print all events after remove the first element

    switch(thisEvent.m_type)  
    {
	//arrival : for scheduling
	case 0: 
	{                       // current time greater transmission time before // timestamp
		if(buffer.empty() && (currentTime > stopTxTime)) 
      scheduleNextTransmission(currentTime);                    // If sending all packet, and current time is greater than scheduled for each transmission time => schedule the next transmission is the current time
    else scheduleNextTransmission(stopTxTime);                  // Else, the transmission is occur in stop time
    cout<<"if current time < stop Transmission time => use stopTxTime to complete the previous tranmission"
    <<"\nstop transmission time = current time + transmitNextPackets(discipline, buffer, currentTime" <<endl;
    cout<<"current time = " <<currentTime<< "\nStop transmission time = " <<stopTxTime<<endl;
		//If the buffer is empty we transmits at the arrival of this packet, but we have to check for are we transmitting or not?
		cout<<"[INFO]: Queue component, an Arrival event comes. Before addNextArrival(), number of Packet in Buffer = "<<buffer.size()<<endl; // Check if the buffer is not empty (that we lose the packet)
		addNextArrival(thisEvent.m_dest);//schedule the next arrival for this destination => cause infinitty
  		class packet newPacket(1000,currentTime,thisEvent.m_dest);//1000 bytes (needs to be changed) // create new packet
      // class packet newPacket1(2000, currentTime,1);
      // add buffer for the transmission is scheduled
  		buffer.push_back(newPacket);                              // add to end of the packets list (if arrival)
      // buffer.push_back(newPacket1);                              // add to end of the packets list (if arrival)
		cout<<"[INFO]: Queue component, Current number of Packet in Buffer = "<<buffer.size()<<endl;    // print current packet
		break;
	}
	//transmission: sending
	case 1: if(buffer.empty()) break;//If the buffer is empty there is no transmission (no packet for sending)
			if(currentTime < stopTxTime) { scheduleNextTransmission(stopTxTime); break; } //We have to defer this transmission because we are being in another transmission  
			else {
		cout<<"[INFO]: Queue component, buffer size ="<<buffer.size()<<" start tx time = "<<currentTime<<endl;  // if buffer is not empty and we in another transmission
		startTxTime = currentTime;  
		timeToTransmit = currentTime + transmitNextPackets(discipline, buffer, currentTime);

		// scheduleNextTransmission(timeToTransmit);
	 	nbOfTransmissions++;                          // transmission done
		stopTxTime = timeToTransmit;                  // time to done a transmisstion
		cout<<"INFO: Queue component, a packet has been transmitted, buffer size ="<<buffer.size()<<" stop tx time = "<<stopTxTime<<endl;
		break; }
	default: cerr << "Error in simulateQueue(): the event type is incorrect (=" << thisEvent.m_type <<")\n";
		 exit(1);
    }

  }

  printEvents();
}

int main(int argc, char* argv[])
{
  int nbOfDest=2;	
  
  /* initialize random seed: */
  srand (time(NULL));

  //Set destination 
  class destination dest1(1,1,500.0,0), dest2(2,1,500.0,0), dest3(3,1,500.0,1); 

  // class destination dest1(1,1,500.0,1), dest2(1,1,500.0,1), dest3(1,1,500.0,1), dest4(2,1,500.0,1), dest5(2,1,500.0,1), dest6(1,1,500.0,1);
  destinations.push_back(dest1);
  destinations.push_back(dest2);
  destinations.push_back(dest3);
  // destinations.push_back(dest4);
  // destinations.push_back(dest5);
  // destinations.push_back(dest6);

   
  //class packet myPacket(1,2,3);
  //cout << myPacket.m_destination << endl;
  //buffer.push_back(myPacket);

  //The simulator
  simulateQueue(buffer, 1);   // 1 : fifoOFDMAOptimal

  return(0);
}
