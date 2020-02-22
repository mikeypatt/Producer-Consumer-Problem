/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/
int TIME_LIMIT = 20 ;

#include "helper.h"

struct Job {
    
    int job_id ;
    int job_duration ;
};

struct Thread{
    
    int id ;
    int* sem_id ;
    Job* job_queue ;
    int number_of_jobs ;
    int size_of_queue ;
    int* counter ;
    
    Thread(int p_id,int* sem_id,Job* job_queue,int j,int q,int*(c)): id(p_id), 
    sem_id(sem_id),job_queue(job_queue),number_of_jobs(j), size_of_queue(q), counter(c) {}
    
};


void *producer (void *id);
void *consumer (void *id);


int main (int argc, char **argv)
{
  if(argc < 5)
  {
      cerr << "usage: main <size_of_queue>" ;
      cerr<< " <number_of_jobs> <number_of_producers> <number_of_consumers>" << endl ;
      return 1 ;
  }
  
  
  string str_cmd = "ipcrm -S" + to_string(SEM_KEY) ;
  char cmd[1000] ;
  strcpy(cmd ,str_cmd.c_str());
  system(cmd) ;
  
  //Initialising the parameters
  int size_of_queue =  check_arg(argv[1]) ;
  int number_of_jobs =  check_arg(argv[2]) ;
  int number_of_producers =  check_arg(argv[3]) ;
  int number_of_consumers =  check_arg(argv[4]) ;
  
  //create an array of jobs for the queue 
  Job job_queue[size_of_queue] ;

  //Intialising the semaphores
  int number_of_sem = 3 , mutex = 1 ,space = size_of_queue, item = 0 ;
  key_t set_key = SEM_KEY ;
  int sem_id = sem_create(set_key,number_of_sem) ;
  if(sem_id == -1)
  {
      cerr << "Could not create the semaphore set" << endl ;
      return 1 ;
  }
  if(sem_init(sem_id, 0,mutex)!= 0 )//Producer & Consumer mutex semaphore
  {
      cerr << "Could not initialise semaphore" << endl ;
      return 1 ;
  }
  if(sem_init(sem_id,1,space)!= 0)  //check buffer is not full
  {
      cerr << "Could not initialise semaphore" << endl ;
      return 1 ;
  }
  if(sem_init(sem_id,2,item)!= 0) //check buffer is not empty 
  {
      cerr << "Could not initialise semaphore" << endl ;
      return 1 ;
  }
  
  
  int producer_queue_counter = 0 ;
  int consumer_queue_counter = 0 ;
  
  //Creating the Producer Threads
  Thread* producer_array[number_of_producers] ;
  pthread_t producerid[number_of_producers] ;
  int producer_id ;
  for(int i = 0 ; i < number_of_producers ; i++)
  {
      producer_id = i+1 ;
      producer_array[i] = new Thread(producer_id,&sem_id,job_queue,number_of_jobs,size_of_queue,&producer_queue_counter);
      pthread_create (&producerid[i], NULL,producer,(void*) producer_array[i]);
  }
  
  //Creating the Consumer Threads
  Thread* consumer_array[number_of_consumers] ;
  pthread_t consumerid[number_of_consumers] ;
  int consumer_id ;
  for(int i = 0 ; i < number_of_consumers ; i++)
  {
      consumer_id = i+1 ;
      consumer_array[i] = new Thread(consumer_id,&sem_id,job_queue,number_of_jobs,size_of_queue,&consumer_queue_counter); 
      pthread_create (&consumerid[i], NULL,consumer,(void*) consumer_array[i]);
  }

  //pthread_t producerid;
  //int parameter = 5;

  //pthread_create (&producerid, NULL, producer, (void *) &parameter);
  for(int i = 0 ; i < number_of_producers ; i++)
  {
      pthread_join (producerid[i], NULL) ;
  }
  for(int i = 0 ; i < number_of_consumers ; i++)
  {
      pthread_join (consumerid[i], NULL) ;
  }
  
  
  for(int i = 0 ; i < number_of_consumers ; i++)
  {
      delete consumer_array[i] ;
  }
  for(int i = 0 ; i < number_of_producers ; i++)
  {
      delete producer_array[i] ;
  }
  

  return 0;
}

void *producer (void *producer_parameter) 
{
  
  //Intialise the structure passed in to thread function 
  Thread* thread_struct = (Thread *) producer_parameter ;
  
  //Check space now and timeout if no space after 20 seconds  
  if(sem_timedwait(*(thread_struct->sem_id),1,TIME_LIMIT) == -1)
  {
      cerr << "Producer("<<thread_struct->id <<"): Cannot create job as there is no space" ;
      cerr << " and the thread timed out after 20s" << endl  ;
      pthread_exit(0);
  }
    /*
    clock_t startTime = clock();
    doSomeOperation();
    clock_t endTime = clock();
    clock_t clockTicksTaken = endTime - startTime;
    double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
  */
  //create the job 
  for(int i = 0 ; i <= thread_struct->number_of_jobs ; i++)
  {
      //get mutual exclusion
      sem_wait(*(thread_struct->sem_id),0) ;
    
      int job_id = *(thread_struct->counter) + 1 ;
      int job_duration = random_number(1,10) ;
      int current_index = *(thread_struct->counter) ;
      thread_struct->job_queue[current_index].job_id = job_id ;
      thread_struct->job_queue[current_index].job_duration = job_duration ;
      cerr << "Producer("<<thread_struct->id <<"): Job ID: " << job_id <<" Job Duration: " << job_duration << endl ; 
      *(thread_struct->counter) =  *(thread_struct->counter) + 1  % thread_struct->size_of_queue ;  //add 1 to the global reference count 
      
      //give up mutual exclusion
      sem_signal(*(thread_struct->sem_id),0) ;
      //up item 
      sem_signal(*(thread_struct->sem_id),2) ;
      
      if(i == thread_struct->number_of_jobs)
      {
          cerr << "Producer("<<thread_struct->id <<"): No jobs left to generate" << endl ;
          pthread_exit(0);
      }
      
      sleep(random_number(1,5)) ;
  }
  
  pthread_exit(0);
  
}

void *consumer (void *consumer_parameter) 
{
    //Intialise the structure passed in to thread function 
    Thread* con_thread_struct = (Thread *) consumer_parameter ;
    
    while(true)
    {
        //Check space now and timeout if no space after 20 seconds  
        if(sem_timedwait(*(con_thread_struct->sem_id),2,TIME_LIMIT) == -1)
        {
            cerr << "Consumer("<<con_thread_struct->id <<"): No Jobs Left" << endl ;
            pthread_exit(0);
        }
        
        //get mutual exclusion
        sem_wait(*(con_thread_struct->sem_id),0) ;
        //get the position
        int current_index = *(con_thread_struct->counter) ;
        
        //fetch item at front of consumer queue
        int consume_job_id = con_thread_struct->job_queue[current_index].job_id ;
        int consume_job_duration = con_thread_struct->job_queue[current_index].job_duration ;
        //shift all around to get the end of queue
        cerr << "Consumer("<<con_thread_struct->id <<"): Job ID: " << consume_job_id <<" executing sleep duration: " << consume_job_duration<< endl ; 
        //give up mutual exclusion
        sem_signal(*(con_thread_struct->sem_id),0) ;
        //up item 
        sem_signal(*(con_thread_struct->sem_id),1) ;
        //up mutex
        // up space
        sleep(consume_job_duration) ;
        cerr << "Consumer("<<con_thread_struct->id <<"): Job ID: " << consume_job_id <<" is completed" << endl ; 
    }
    
    pthread_exit(0);

}
