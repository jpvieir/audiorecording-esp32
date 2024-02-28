///// LINSE - UFSC 
//// Algoritmo que realiza a aquisição de um sinal de áudio a cada intervalo definido de tempo. O áudio é adquirido via um Microfone I2S (INMP400) e armazenado em um SD CARD.
//// Após cada gravação, o sinal é enviado para o Google Drive com HTTP Posts. 
//// Toda gravação contém um nome que inclui data e horário. 
//// Especificações do áudio: FS=16kHz; 16 bit big-endian PCM.
//// Desenvolvido por: João Paulo Vieira

#include "wificonfig.h" // credenciais de rede + função que realiza conexão ao wifi
#include "post_functions.hh" // funções que enviam o arquivo ao servidor e que gravam o áudio
#include "config.h" // configuraçoes da interface I2S

//// Definição de variáveis:  
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
I2SSampler *input;

const int tempo_ms = 300000; int t1,t2 =0; const int tempograv = TEMPO*1000; int twifi1,twifi2=0;
bool connected = false; bool set = 0;

// Main Loop: 
// Se conecta ao WiFi para obter a data -> criar um filename para a gravação;
// Desconecta do WiFi para realizar a gravação (necessário por limitações do ESP32);
// Monta o SD Card e grava o áudio;
// Reconecta à internet e envia o áudio através da função sendBinaryDataToAppScript;
// Desmonta o SD Card e fica em Idle (Light Sleep) até a próxima gravação.

void main_loop(I2SSampler *input){

  t1=millis();  twifi1 = t1; 

  if (!set){
    connected = configurewifi();
    timeClient.begin();
    timeClient.setTimeOffset(-10800); // define a Timezone BR
    esp_wifi_set_ps(WIFI_PS_NONE);
    set = true; 
  }  
  else {
    esp_wifi_start();
    connected = configurewifi();
  }
      
  int N_RETRY = 50; int i = 0;

  while(!timeClient.update()){
    timeClient.forceUpdate();
    i++;
    if(i>=N_RETRY){i=0; Serial.println("Couldn't retrive date, restarting ESP. "); ESP.restart();}
     
  }
      
  twifi2=millis();

  char * filename; //char * filenamewav; filenamewav = (char *)malloc(sizeof(char)*24);
  if (connected && ((tempo_ms-(twifi2-twifi1))>tempograv)){ 
      
      String date = timeClient.getFormattedDate();
      
      char t[date.length()+1]; strcpy(t,date.c_str()); 
      char slash[30] = "/";
      char * f = strcat(slash,t); filename = strcat(f,".wav");

      uint8_t len = strlen(filename);
      for(int i =0; i < len; i++){
        if (*filename == (char)58){*filename = (char)45;} filename++;
      }
      filename = filename - len; Serial.print("Filename: "); Serial.println(filename);

      // Desconectando o ESP da internet para gravar:   
      WiFi.disconnect(true);
      esp_wifi_stop(); 

      bool sd_connected = mountSDCARD();

      if (sd_connected) { 

        record(input, filename); 
        vTaskDelay(pdMS_TO_TICKS(5000)); 

        esp_wifi_start();
        connected = configurewifi();
        Serial.print("Sending file: ");
        Serial.println(filename);

        if(connected){
          sendBinaryDataToAppScript(filename);
        }

        String response = checkFileSizeWithAppsScript(filename); String file_deleted; 
        Serial.println("File Size Comparison Result: " + response);

        int send_retries = 3; int n_retries = 0; 
        while(response != "true") {

          file_deleted = deleteOldFile(filename); Serial.println("File delete result: " + file_deleted); 
          if (file_deleted != "true") { Serial.println("Google failed to delete file. "); break; }

          sendBinaryDataToAppScript(filename);
          response = checkFileSizeWithAppsScript(filename);
          n_retries++;
          if (n_retries >= send_retries) { Serial.println("Failed to send file. Restarting ESP32. "); ESP.restart(); }
        }
      }
  } 

  unmountSDCARD();

  t2=millis();
  t2 = tempo_ms - (t2-t1); 

  Serial.print("Light Sleep mode for: ");
  Serial.print(t2/1000);
  Serial.println(" seconds.");

  if(t2>0){
  //  esp_sleep_enable_timer_wakeup(t2*1000- 1000*1000);
  //  esp_light_sleep_start();
      delay(t2);
  }

}

void main_task(void *param)
{
  I2SSampler *input = new I2SMEMSSampler(I2S_NUM_0, i2s_mic_pins, i2s_mic_Config); // configura a interface I2S
  while(true){
    main_loop(input);
  }  
}

/// Setup: configura um LED (ativo durante a gravação) e cria a Main Task;
void setup(){
  Serial.begin(921600); 
  pinMode(22,OUTPUT);
  xTaskCreate(main_task, "Main", 80000, NULL, 0, NULL); 
}

void loop(){}