//valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./bin/main

#include "pool.h"

void * thread_accept(void * pool){
  char *hello = "Hello from server";
  pool_thread *p=(pool_thread *)pool;
  int connval;

  while(1){
    connval = socket_accept(p->sockval);
    if(connval < 0) pthread_exit(NULL);
    send(connval , hello , strlen(hello) , 0);
    procesarPeticiones(connval);
    sleep(1);
    close(connval);
    //wait_finished_services();
  }
  pthread_exit(NULL);
}

pool_thread * pool_create(int sockval){
  pool_thread *pool;
  int i;

  pool = (pool_thread *) malloc (sizeof(pool_thread));
  if(pool == NULL){
    syslog(LOG_ERR, "Error creating pool");
    exit(EXIT_FAILURE);
  }

  pool->num_threads = NUM_THREADS;
  pool->stop = 0;
  pool->sockval = sockval;
  if(pthread_mutex_init(&pool->shared_mutex, NULL)!=0){
    free(pool);
    syslog(LOG_ERR, "Error creating mutex");
    exit(EXIT_FAILURE);
  }

  pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * pool->num_threads);
  if(pool->threads == NULL) {
    free(pool);
    syslog(LOG_ERR, "Error creating thread");
    exit(EXIT_FAILURE);
  }
  pool->work_function = thread_accept;

  //TODO hay que hacer algo con los hilos creados si hay error
  for(i=0; i<pool->num_threads; i++){
    if(pthread_create(&(pool->threads[i]),NULL, pool->work_function,(void *) pool)!=0){
      free(pool->threads);
      free(pool);
      syslog(LOG_ERR, "Error creating thread");
      exit(EXIT_FAILURE);
    }
  }
  return pool;
}

void pool_free(pool_thread * pool) {
  int i;

  syslog (LOG_INFO, "Cleaning");
  //TODO pensar
  pool->stop = 1;

  for(i=0; i<pool->num_threads; i++){
    pthread_kill(pool->threads[i], SIGINT);
  }

  for(i=0; i<pool->num_threads; i++){
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool);
}
