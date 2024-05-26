#pragma once

#ifndef _WIN32

#include <pthread.h>

class C_pthread_mutex_t_lock
{
    private:
        pthread_mutex_t & _mtx;
    public:
        C_pthread_mutex_t_lock( pthread_mutex_t & m ) : _mtx( m ) { pthread_mutex_lock( & _mtx ); }
        ~C_pthread_mutex_t_lock() { pthread_mutex_unlock( & _mtx ); }
};

#endif

class CSimpleThread
{
#ifdef _WIN32    
    private:
        HANDLE heventStop;
        HANDLE hthread;
#else
    public: 
        pthread_t the_thread;
        pthread_cond_t end_condition;
        pthread_mutex_t the_mutex;
        bool stop_running;

#endif

    public:
#ifdef _WIN32
        CSimpleThread( LPTHREAD_START_ROUTINE func ) : heventStop( INVALID_HANDLE_VALUE ), hthread( INVALID_HANDLE_VALUE )
        {
            heventStop = CreateEvent( 0, FALSE, FALSE, 0 );
            hthread = CreateThread( 0, 0, func, heventStop, 0, 0 );
        }

        void EndThread()
        {
            if ( INVALID_HANDLE_VALUE != hthread )
            {
                SetEvent( heventStop );
                WaitForSingleObject( hthread, INFINITE );
                CloseHandle( hthread );
                hthread = INVALID_HANDLE_VALUE;
                CloseHandle( heventStop );
                heventStop = INVALID_HANDLE_VALUE;
            }
        }
#else        
        CSimpleThread( void * ( * start_routine )( void * ) ) : the_thread( 0 )
        {
            stop_running = false;
            end_condition = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
            the_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
            pthread_cond_init( & end_condition, 0 );   
            int ret = pthread_create( & the_thread, 0, start_routine, (void *) this );  
            tracer.Trace( "simplethread: return value from pthread_create: %d\n", ret );
        }

        void EndThread()
        {
            if ( 0 != the_thread )
            {
                stop_running = true;

                {
                    // This signal is used to wake the thread from sleeping so it can check if it's time to stop.

                    tracer.Trace( "simplethread: signaling a thread to check if it's time to complete\n" );
                    C_pthread_mutex_t_lock mtx_lock( the_mutex );
                    pthread_cond_signal( & end_condition );
                }

                tracer.Trace( "simplethread: joining the thread\n" );
                pthread_join( the_thread, 0 );
                the_thread = 0;
                tracer.Trace( "simplethread: destroying the thread resources\n" );
                pthread_cond_destroy( & end_condition );
                pthread_mutex_destroy( & the_mutex );
            }
        }
#endif

        ~CSimpleThread() { EndThread(); }
}; //CSimpleThread
       
