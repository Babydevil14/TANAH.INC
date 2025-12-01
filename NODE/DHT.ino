/*
 PROYEK: TANAH.inc
 ROLE: SENDER NODE - V2 (STABIL)
 Interval: 10 Detik (Agar tidak spamming Central Node)
*/


#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <DHT.h>


// --- KONFIGURASI ---
#define ID_NODE 1       // <--- GANTI JADI 2 UNTUK ALAT KEDUA
#define WIFI_CHANNEL 6  // SESUAIKAN DENGAN CENTRAL NODE (Lamo = 6 biasanya)


uint8_t targetMAC[] = {0x88, 0x57, 0x21, 0xB6, 0x45, 0xC4};


#define DHTPIN 32     
#define DHTTYPE DHT22 
const int soilPin = 35;


// Kalibrasi
const int DRY_VAL = 4095;
const int WET_VAL = 1500;


DHT dht(DHTPIN, DHTTYPE);


typedef struct struct_message {
 int id;
 float temp;
 float hum;
 int soil;
} struct_message;


struct_message myData;
esp_now_peer_info_t peerInfo;


// Fungsi Baca
void bacaDHT() {
 myData.temp = dht.readTemperature();
 myData.hum = dht.readHumidity();
 if (isnan(myData.temp)) { myData.temp = 0; myData.hum = 0; }
  Serial.print("   > [UDARA] Suhu: "); Serial.print(myData.temp);
 Serial.print(" C | Hum: "); Serial.println(myData.hum);
}


void bacaTanah() {
 int raw = analogRead(soilPin);
 myData.soil = map(raw, DRY_VAL, WET_VAL, 0, 100);
 if (myData.soil < 0) myData.soil = 0;
 if (myData.soil > 100) myData.soil = 100;
  Serial.print("   > [TANAH] Soil: "); Serial.print(myData.soil); Serial.println("%");
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
 Serial.print("Status: ");
 Serial.println(status == ESP_NOW_SEND_SUCCESS ? "TERKIRIM" : "GAGAL");
}


void setup() {
 Serial.begin(115200);
 dht.begin();
  WiFi.mode(WIFI_STA);
 esp_wifi_set_promiscuous(true);
 esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
 esp_wifi_set_promiscuous(false);


 if (esp_now_init() != ESP_OK) return;
 esp_now_register_send_cb(OnDataSent);


 memcpy(peerInfo.peer_addr, targetMAC, 6);
 peerInfo.channel = WIFI_CHANNEL; 
 peerInfo.encrypt = false;
 esp_now_add_peer(&peerInfo);
  Serial.print("NODE "); Serial.print(ID_NODE); Serial.println(" SIAP.");
}


void loop() {
 myData.id = ID_NODE;
 Serial.println("\n--- Membaca Sensor ---");


 if (ID_NODE == 1) {
   bacaDHT();
   myData.soil = 0; // Node 1 tidak kirim tanah
 } else {
   bacaTanah();
   myData.temp = 0; // Node 2 tidak kirim suhu
   myData.hum = 0;
 }


 esp_err_t result = esp_now_send(targetMAC, (uint8_t *) &myData, sizeof(myData));
  // DELAY DIPERPANJANG JADI 10 DETIK
 // Agar Central Node punya waktu bernafas untuk upload data Node 1
 // sebelum data Node 2 datang.
 delay(10000);
}


