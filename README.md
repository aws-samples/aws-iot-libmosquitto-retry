# aws-iot-libmosquitto-retry

This is an example of an MQTT client that demonstrates how to perform an unsubscribe operation with retry and exponential backoff mechanisms. The example uses the Mosquitto library and FreeRTOS backoffalgorithm.

## Key Components and Functionality

### Libraries and constants

The example includes standard libraries (stdbool, stdio, stdlib, time and unistd) and specific libraries like libmosqutto for MQTT functionality and FreeRTOS’ backoffAlgorithm for handling retry logic with backoff.

There are several constants related to the retry mechanism:
-	`RETRY_MAX_ATTEMPTS:` Maximum number of retry attempts.
-	`RETRY_MAX_BACKOFF_DELAY_MS:` Maximum back-off delay in milliseconds.
-	`RETRY_BACKOFF_BASE_MS:` Base back-off delay in milliseconds.
-	`MAX_RETRIES` and `TIMEOUT` are additional constants used in the retry logic.

### Data Structures

`mqtt_unsub_user_data_t:` A structure used to track the MQTT client's state, including whether it is connected (connected) and if there is a pending UNSUBACK (unsubscribe acknowledgment) (pending_unsuback).

### Callback Functions:

- `on_connect:` Called when the client successfully connects to the MQTT broker. It sets the connected flag to true.

- `on_disconnect:` Called when the client disconnects from the MQTT broker. It updates the run variable with the return code.

- `on_unsubscribe:` Called when the client successfully unsubscribes from a topic. It sets pending_unsuback to false.

### Main MQTT Logic:

- Initialization:

    - Initializes the backoff algorithm parameters.
    - Seeds the pseudo-random number generator for use in the backoff calculation.
    - Initializes the Mosquitto library and creates a new Mosquitto client instance (`mosquitto_new`).
    - Sets up the callback functions for connection, disconnection, and unsubscription events.

- Connection to Broker:

    - Connects to the MQTT broker on the specified URI and port using `mosquitto_connect`.
    - Starts the Mosquitto network loop with `mosquitto_loop_start`, which handles network communication in a separate thread.

- Unsubscribe Logic with Retry:

    - Enters a loop where it attempts to unsubscribe from the topic "unsubscribe/test".
    - If an UNSUBACK is not received (pending_unsuback remains true), the code calculates the next retry delay using the backoff algorithm and waits for the specified backoff period before retrying.
    - This loop continues until either an `UNSUBACK` is received or the maximum number of retries is exhausted (`BackoffAlgorithmRetriesExhausted`).

- Cleanup and Exit:

    - Once the unsubscribe process is complete or the retries are exhausted, the client disconnects from the MQTT broker, destroys the Mosquitto client instance, cleans up the Mosquitto library, and exits with the run status.

## How the Code Works

The code is designed to handle unreliable network conditions or throtling scenarios by implementing a robust retry mechanism with exponential backoff.

When the client connects to the broker, it immediately tries to unsubscribe from a predefined topic.

If the UNSUBACK is not received, the code retries the unsubscribe operation after waiting for a backoff period that grows exponentially with each attempt.

The use of exponential backoff helps avoid flooding the network with retry attempts, especially when multiple devices are trying to perform the same action at the same time.

The program exits once the unsubscribe process is confirmed or the retry limit is reached.

## Usage

You will need to create an artificial delay to the MQTT server to simulate a timeout or throtling scenario. Use one terminal to run the server and another terminal to run the client.

```c
// https://github.com/eclipse/mosquitto/blob/v2.0.18/src/handle_unsubscribe.c#L162
int handle__unsubscribe(struct mosquitto *context) {
    
    /*
    .
    .
    .
    */

    printf("Waiting.....5s here\r\n");
    sleep(5);

	mosquitto__free(reason_codes);
	return rc;
}
```
```bash
make WITH_TLS=no WITH_CJSON=no WITH_DOCS=no && ./src/mosquitto -v 
# 1725014427: New client connected from 127.0.0.1:50946 as unsubscribe-test (p2, c1, k60).
# 1725014427: No will message specified.
# 1725014427: Sending CONNACK to unsubscribe-test (0, 0)
# 1725014427: Received UNSUBSCRIBE from unsubscribe-test
# 1725014427:     unsubscribe/test
# 1725014427: unsubscribe-test unsubscribe/test
# 1725014427: Sending UNSUBACK to unsubscribe-test
# Waiting.....5s here
# 1725014432: Received UNSUBSCRIBE from unsubscribe-test
# 1725014432:     unsubscribe/test
# 1725014432: unsubscribe-test unsubscribe/test
# 1725014432: Sending UNSUBACK to unsubscribe-test
# Waiting.....5s here
```
To run the client:

1. Download the code and install dependencies

```bash
sudo apt-get install libmosquitto-dev 
git clone https://github.com/aws-samples/aws-iot-libmosquitto-retry
cd aws-iot-libmosquitto-retry
git clone -b v1.4.1 https://github.com/FreeRTOS/backoffAlgorithm
```
2. Compile and run the client

```bash
./make 
# gcc -I backoffAlgorithm/source/include main.c backoffAlgorithm/source/backoff_algorithm.c -o main -lmosquitto

ldd main
#         linux-vdso.so.1 (0x00007ffc7cb3b000)
#         libmosquitto.so.1 => /usr/local/lib/libmosquitto.so.1 (0x000076073c290000) <--- make sure libmosquit-dev is isntalled
#         libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x000076073c000000)
#         /lib64/ld-linux-x86-64.so.2 (0x000076073c2b5000)

./main 
# on_connect
# connected!
# send_unsubscribe
# nextRetryBackoff                        :  491
# mqtt_unsub_user_data.pending_unsuback   :  1
# BackoffAlgorithmRetriesExhausted        :  1
# retryStatus                             :  0
# send_unsubscribe
# nextRetryBackoff                        :  1400
# mqtt_unsub_user_data.pending_unsuback   :  1
# BackoffAlgorithmRetriesExhausted        :  1
# retryStatus                             :  0
# send_unsubscribe
# nextRetryBackoff                        :  1665
# mqtt_unsub_user_data.pending_unsuback   :  1
# BackoffAlgorithmRetriesExhausted        :  1
# retryStatus                             :  0
# on_unsubscribe
# send_unsubscribe
# nextRetryBackoff                        :  2115
# mqtt_unsub_user_data.pending_unsuback   :  0
# BackoffAlgorithmRetriesExhausted        :  1
# retryStatus                             :  0

```
## Summary

This code is an MQTT client example that demonstrates the use of the Mosquitto library for MQTT communication and implements a retry mechanism with exponential backoff to handle unreliable network or throtling scenarios when unsubscribing from a topic. 

The code is robust against connection failures and ensures that the unsubscribe operation is retried until success or the maximum retry limit is reached.