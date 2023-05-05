#include "LoRaWan_APP.h"
#include "Arduino.h"



#define RF_FREQUENCY                                865000000 // Hz
#define TX_OUTPUT_POWER                             5        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

typedef enum
{
    LOWPOWER,
    STATE_RX,
    STATE_TX
}States_t;

int16_t txNumber;
States_t state;
bool sleepMode = false;
int16_t Rssi,rxSize;
int checksum;


void setup() {
    Serial.begin(115200);
    Mcu.begin();
    txNumber=0;
    Rssi=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    state=STATE_TX;
}



void loop()
{
  switch(state)
  {
    case STATE_TX:
      delay(5000);
      txNumber++;
      //Modified reference code 
      // Create the packet with the hello message and the packet number
      sprintf(txpacket,"hello-%d",txNumber);

      // Calculate the checksum for this package
      checksum = calculateCheckSum(txpacket, strlen(txpacket));

      // Append the checksum at the end of this package
      sprintf(txpacket,"hello-%d.%d",txNumber, checksum);
      Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));
      //Modified reference code 
      // Send the packet
      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); // Calls on TransmitDone
      state=LOWPOWER;
      break;
    case STATE_RX:
      Serial.println("into RX mode");
      Radio.Rx( 0 );
      state=LOWPOWER;
      break;
    case LOWPOWER:
      Radio.IrqProcess( );
      break;
    default:
      break;
  }
}

// OnTransmitDone
void OnTxDone( void )
{
  Serial.print("TX done......");
  state=STATE_RX;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.print("TX Timeout......");
    state=STATE_TX;
}

// OnReceiveDone
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Rssi=rssi;
    rxSize=size;

    // Copy the received packet into rxpacket
    memcpy(rxpacket, payload, size );

    
    rxpacket[size]='\0';
// Kacey & Jonas(extract & compare)
    char delim = '.';
    int delimIndex = findIndexOf(rxpacket, size, delim);

    // The original packet without the checksum
    char rxpacketcopy[BUFFER_SIZE];

    // The checksum portion
    char checksum_slice[size - delimIndex];

    // Copy the rxpacket until the deliminator
    // Example: hello-200.10 -> hello-200 (rxpacketcopy)
    strncpy(rxpacketcopy, rxpacket, delimIndex);

    // Copy the checksum slice from the rxpacket
    // Example: hello-200.10 -> 10
    strcpy(checksum_slice, &rxpacket[delimIndex + 1]);

    // Add the null terminator 
    rxpacketcopy[delimIndex] = '\0';

    // Calculate the checksum of the rxpacketcopy pkacket
    checksum = calculateCheckSum(rxpacketcopy, strlen(rxpacketcopy));

    Radio.Sleep( );

    Serial.printf("\r\nReceived packet \"%s\" with Rssi %d, length %d",rxpacket,Rssi,rxSize);
    Serial.printf("\r\n\Calculated checksum: %d\nPackage checksum: %s\r\n", checksum, checksum_slice);

    // Check if the calculated checksum matches the one in the package
    if (atoi(checksum_slice) == checksum) {
        Serial.println("Checksums are equal!");
    } else {
        Serial.println("Checksums are not equal!");
    }

    state=STATE_TX;
}
//Kacey & Jonas 

// Jonas
// Returns the index of the deliminator given the package
int findIndexOf(char* packet, uint16_t size, char delim) {
  int index = -1;
  for (uint16_t i = 0; i < size; i++) {
    if (packet[i] == delim) {
      index = i;
    }
  }
  return index;
}
//Jonas
// Kacey
// Calculates the checksum by xoring each character bit
int16_t calculateCheckSum(char* packet, uint16_t size) {
  int16_t xo = 0;
  for (uint16_t i = 0; i < size; i++) {
    xo = xo ^ (int) packet[i];
  }
  return xo;
}
//Kacey
