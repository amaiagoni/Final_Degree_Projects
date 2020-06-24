/* GENERAL FUNCTIONALITY CONFIGURATIONS -----------------------------------------------------------*/

#define VEHICLE_ID 													"B1234"						/**< Vehicle identification code, consisting of a letter defining the vehicle type (B - bicycle, P - public bus) and a four digit numeric code */
#define SECS_BETWEEN_WIFI_DUMP 							5								/**< Seconds between data acquisition stops for dumping data to the database through WiFi connection. It will determine the number of RMS values to store */
#define NUMBER_OF_SECONDS_WITH_SAME_GPS 		500								/**< Number of RMS groups that will have the same GPS positioning */
#define FRAMES_PER_DATA_UNIT								16								/**< Number of data frames that will constitute a unit of noise acquisition */

/* MICROPHONE AND DATA ACQUISITION ----------------------------------------------------------------*/

#define FRAME_LENGTH 												3000							/**< Number of values that will constitute a frame - For avoiding data loss, it is recommended to be half the size of the defined buffer length */
#define BUFFER_LENGTH 											6000							/**< Size of the buffer used by the DMA for data acquisition - For avoiding data loss, it is recommended to be double the size of the defined frame length */

/* WIFI MODULE ------------------------------------------------------------------------------------*/

#define WIFI_SSID														"BELFAST"					/**< WiFi Connection for data dump - SSID */
#define WIFI_PASSWORD												"42314231"				/**< WiFi Connection for data dump - Password */
#define SERVICE_PROTOCOL										"TCP"							/**< Connection to service running database - Protocol. For working with InfluxDB it must be TCP */
#define SERVICE_IP													"192.168.1.108"		/**< Connection to service running database - IPV4 of the device where the service is running */
#define SERVICE_PORT												"8086"						/**< Connection to service running database - Port number of the service in the device where it is running */
#define DATABASE_NAME												"Zaratamap"				/**< Name of the database the data dump is going to made into */
#define RETRY_NUMBER 												3									/**< Number of retries for receiving an OK response from an AT command sent to the ESP8266 */	
#define MAX_SECS_BETWEEN_STATUS 						10								/**< Maximum seconds to wait between two AT commands sent to the ESP8266 */
#define SECS_FOR_NO_REPLY_MESSAGE 					5									/**< Maximum seconds to wait between no-reply messages sent to the ESP8266 - Recommended to be smaller than MAX_SECS_BETWEEN_STATUS */
#define SECS_BETWEEN_DATA_MESSAGES 					5									/**< Maximum seconds to wait between the send of data messages to the database that don't return an OK answer - Recommended to be smaller than MAX_SECS_BETWEEN_STATUS */

/* GPS MODULE -------------------------------------------------------------------------------------*/

#define MAX_SECS_BETWEEN_STATUS_GPS 				20								/**< Maximum seconds to wait for an OK reply from an AT command sent to the GPS Module */
#define RETRY_NUMBER_GPS 										0									/**< Number of retries for receiving an OK response from an AT command sent to the GPS Module */	

