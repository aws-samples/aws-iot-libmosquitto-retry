#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <time.h>
#include <unistd.h>

#include "backoff_algorithm.h"

// The maximum number of retries
#define RETRY_MAX_ATTEMPTS            ( 999U )

// The maximum back-off delay (in milliseconds) for between retries
#define RETRY_MAX_BACKOFF_DELAY_MS    ( 3000U )

// The base back-off delay (in milliseconds) for retry configuration in the example
#define RETRY_BACKOFF_BASE_MS         ( 1000U )

static int run = -1;
int retry_count = 0;

typedef struct {
	bool pending_unsuback;
    bool connected;
} mqtt_unsub_user_data_t;

void send_unsubscribe(struct mosquitto *mosq, void * obj) {
    printf("send_unsubscribe\r\n");

    int mid = 0xFFFFF;
    mosquitto_unsubscribe(mosq, &mid, "unsubscribe/test");
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
	printf("on_connect\r\n");

    if(rc){
		exit(1);
	}else{
        mqtt_unsub_user_data_t* mqtt_unsub_user_data = (mqtt_unsub_user_data_t*) obj;
        mqtt_unsub_user_data->connected = true;
	}

}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
    printf("on_disconnect\r\n");
	run = rc;
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid) {
    printf("on_unsubscribe\r\n");
    mqtt_unsub_user_data_t* mqtt_unsub_user_data = (mqtt_unsub_user_data_t*) obj;
    mqtt_unsub_user_data->pending_unsuback = false;

}

int main(int argc, char *argv[]) {

    BackoffAlgorithmStatus_t retryStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryParams;
    uint16_t nextRetryBackoff = 0;

    int32_t dnsStatus = -1;
    struct timespec tp;

    // Get current time to seed pseudo random number generator.
    ( void ) clock_gettime( CLOCK_REALTIME, &tp );

    // Seed pseudo random number generator with seconds.
    srand( tp.tv_sec );

    // Initialize reconnect attempts and interval.
    BackoffAlgorithm_InitializeParams( &retryParams,
                                RETRY_BACKOFF_BASE_MS,
                                RETRY_MAX_BACKOFF_DELAY_MS,
                                RETRY_MAX_ATTEMPTS );

    // Get current time to seed pseudo random number generator. */
    ( void ) clock_gettime( CLOCK_REALTIME, &tp );
    
    // Seed pseudo random number generator with seconds. */
    srand( tp.tv_sec );

    int rc;
	struct mosquitto *mosq;

	mosquitto_lib_init();

    mqtt_unsub_user_data_t mqtt_unsub_user_data = {};
    mqtt_unsub_user_data.pending_unsuback = true;
    mqtt_unsub_user_data.connected = false;

	mosq = mosquitto_new("unsubscribe-test", true, &mqtt_unsub_user_data);
	if(mosq == NULL){
		return 1;
	}

	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);

	rc = mosquitto_connect(mosq, "localhost", 1883, 60);

    int loop = mosquitto_loop_start(mosq);

    if(loop != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Unable to start loop: %i\n", loop);
        exit(1);
    }

    while(!mqtt_unsub_user_data.connected); // wait to get connected
    
    printf("connected!\r\n");

    do {

        if (mqtt_unsub_user_data.pending_unsuback) {

            /* Generate a random number and get back-off value (in milliseconds) for the next retry.
             * Note: It is recommended to use a random number generator that is seeded with
             * device-specific entropy source so that backoff calculation across devices is different
             * and possibility of network collision between devices attempting retries can be avoided.
             *
             * For the simplicity of this code example, the pseudo random number generator, rand()
             * function is used. */
            retryStatus = BackoffAlgorithm_GetNextBackoff( &retryParams, rand(), &nextRetryBackoff );

            /* Wait for the calculated backoff period before the next retry attempt of querying DNS.
             * As usleep() takes nanoseconds as the parameter, we multiply the backoff period by 1000. */
            ( void ) usleep( nextRetryBackoff * 1000U );

            send_unsubscribe(mosq, &mqtt_unsub_user_data);
            
        }

        printf("nextRetryBackoff                        :  %d\r\n", nextRetryBackoff);
        printf("mqtt_unsub_user_data.pending_unsuback   :  %i\r\n", mqtt_unsub_user_data.pending_unsuback);
        printf("BackoffAlgorithmRetriesExhausted        :  %i\r\n", BackoffAlgorithmRetriesExhausted);
        printf("retryStatus                             :  %i\r\n", retryStatus);

    } while( 
        ( mqtt_unsub_user_data.pending_unsuback ) && 
        ( retryStatus != BackoffAlgorithmRetriesExhausted ) 
    );


    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
	return run;
}