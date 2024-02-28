#include "config.h"
#include "post_functions.hh"
#include <iostream>
#include <string>
#include "math.h"


const int BUFFER_SIZE =  32768;
const int NUM_RETRIES = 5;
const int RETRY_DELAY = 200;
bool turn = true; 
String url = ""; /// URL da implantação do AppScript
size_t fileSize;
wav_header_t header; 


/// mountSDCARD: monta o SD card no ESP32. -> Após isso, está pronto para uso. 
bool mountSDCARD(){

  if(!SD.begin()){
    Serial.println("Card Mount Failed, restarting ESP.");
    return false;
  }
  
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");

  if(cardType == CARD_MMC){
        Serial.println("MMC");
  } else if(cardType == CARD_SD){
        Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
  } else {
        Serial.println("UNKNOWN");
  }

  return true;
}

void unmountSDCARD(){
  SD.end();
  Serial.println("Unmounted the SD CARD. ");
}


/// Função que retira o primeiro caractere de uma String (necessária para tratar o filename ao enviá-lo ao servidor)
void getSubstringFromSecond(String str, char* result) {
  int len = 0;
  while (str[len] != '\0') {
      len++;
  }

  if (len <= 1) {
    // Handle the case when the string is too short or empty
    result[0] = '\0'; // Set the result string as empty
    return;
  }

  int startIndex = 1; // Start from the second character
  int resultIndex = 0;

  while (str[startIndex] != '\0') {
    result[resultIndex] = str[startIndex];
    startIndex++;
    resultIndex++;
  }

  result[resultIndex] = '\0'; // Null-terminate the result string
}

// sendBinaryDataToAppScript: função que envia o áudio adquirido em buffers de tamanho BUFFER_SIZE para o servidor. 
// Inicialmente, é feito um GET para enviar o nome do arquivo. Após isso, são feitos vários POSTs até que se envie o arquivo por completo. 
// A cada tentativa de POST, avalia-se o código retornado para garantir que os dados foram recebidos -> caso haja algum erro, são feitas até 5 tentativas. 

void sendBinaryDataToAppScript(const char * fname) {

  File file = SD.open(fname, "rb"); 
  int n_try=5; int i = 0;
  while (!file) {
    Serial.println("File open failed. Retrying... ");
    File file = SD.open(fname, "rb");
    i++;
    if(i>=n_try){return;}
  }


  fileSize = file.size();


  char result[100]; // You can adjust the size as per your requirement
  getSubstringFromSecond(fname, result);

  HTTPClient http;
  http.begin(url + "?filename=" + result + "&fileSize=" + String(fileSize) + "&check_filesize=false"  + "&delete_file=false");
  int GETCODE = http.GET();  i = 0;
  Serial.println("GET result code: " + String(GETCODE));
  while((GETCODE != 302)) {
    GETCODE = http.GET(); i++;
    Serial.println("GET result code: " + String(GETCODE));
    if (i>= n_try) {Serial.println("Failed to send filename!"); return; }
  }
  http.end();
    
  uint8_t * postbuffer = (uint8_t *)malloc(sizeof(uint8_t) * BUFFER_SIZE);
  int bytesRead; int totalSent = 0;

  http.setTimeout(20000); // Timeout elevado pois os buffers possuem 32kB. 
  http.begin(url);
  http.addHeader("Content-Type", "application/octet-stream"); // Headerless binary files

  // Leitura do arquivo: 
  while (file.available()) {
      
    bytesRead = file.read(postbuffer, BUFFER_SIZE); 
    Serial.println(bytesRead);

    if (bytesRead > 0) {
      Serial.printf("Sending chunk %d - %d bytes...\n", totalSent, totalSent + bytesRead);
      bool success;
      for (int retry = 0; retry < NUM_RETRIES; retry++) {

        int httpResponseCode = http.POST((postbuffer), bytesRead);

        if ((httpResponseCode > 0)){
          Serial.printf("HTTP Response code: %d\n", httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
          //Serial.println("Sent chunk successfully."); 
          success = true; break; 
        } else {
          Serial.printf("Error sending data (Retry %d): %s\n", retry + 1, http.errorToString(httpResponseCode).c_str());
          delay(RETRY_DELAY);
        }

      }   

      if (!success) {
        Serial.println("Failed to send buffer. Aborting transmission.");
        break;
      }

      totalSent += bytesRead;

    } else {

      Serial.println("Error reading file");
      break;
    }
  }

  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  Serial.println(free_size);
  free(postbuffer);
  file.close();
  http.end();
  
}

// Function to convert an int or short to a byte array
uint8_t* int_to_byte_array(void* data, size_t size) {
    uint8_t* byte_array = (uint8_t*)malloc(size);

    if (byte_array == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return NULL;
    }

    for (size_t i = 0; i < size; i++) {
        byte_array[i] = ((uint8_t*)data)[i];
    }

    return byte_array;
}


void writeWavHeader(File fp, int fsize){
  
  fp.write(header.riff_header,4);
  int fsize2= fsize+36;
  uint8_t* fsize_array = int_to_byte_array(&fsize2, sizeof(fsize));
  fp.write(fsize_array ,4);
  fp.write(header.wave_header,4);
  fp.write(header.fmt_header,4);
  uint8_t* chunksize_array = int_to_byte_array(&header.fmt_chunk_size, sizeof(int));
  fp.write(chunksize_array,4);
  uint8_t* audioformat_array = int_to_byte_array(&header.audio_format, sizeof(short));
  fp.write(audioformat_array ,2);
  uint8_t* nchannels_array = int_to_byte_array(&header.num_channels, sizeof(short));
  fp.write(nchannels_array ,2);
  uint8_t* samplerate_array = int_to_byte_array(&header.sample_rate, sizeof(int));
  fp.write(samplerate_array,4);
  uint8_t* byterate_array = int_to_byte_array(&header.byte_rate, sizeof(int));
  fp.write(byterate_array,4);
  uint8_t* alignment_array = int_to_byte_array(&header.sample_alignment, sizeof(short));
  fp.write(alignment_array,2);
  uint8_t* bitdepth_array = int_to_byte_array(&header.bit_depth, sizeof(short));
  fp.write(bitdepth_array,2); 
  fp.write(header.data_header,4);
  uint8_t* fsize2_array = int_to_byte_array(&fsize, sizeof(fsize));
  fp.write(fsize2_array ,4);

}

//// record: 
// Função que realiza a gravação do áudio com comunicação I2S. 
// Utiliza-se um buffer de tamanho 1024 * 32 bits; 
// O microfone escreve palavras de 16 bits, e realiza-se uma amplificação digital desse sinal, multiplicando-o por 4; 

void record(I2SSampler *input, const char *fname)//, int64_t *samples)
{

  int buffer_size =  1024;
  int64_t* samples = (int64_t *)malloc(sizeof(int64_t) * buffer_size); // buffer utilizado para gravação  
  input->start(); // Inicia o I2S
  
  File fp = SD.open(fname, "wb");
  int count_r = 0;  int mult = ceil(TEMPO*SAMPLE_RATE/buffer_size); // tempo de gravação

  int fsize = mult * 2 * buffer_size * 2;
  writeWavHeader(fp, fsize);

  Serial.println("Starting to record in");
  Serial.println("3..."); delay(1000); Serial.println("2..."); delay(1000); Serial.println("1..."); delay(1000);
  Serial.println("Speak!");

  gpio_set_level(GPIO_NUM_12,1); 

  int64_t c1,c2;
  c1 = millis();

  while (turn == 1)
  {
    int samples_read = input->read(samples, buffer_size); 
    uint8_t acquired_data[4];

    for(int i=0; i<samples_read;i++){
      acquired_data[0] = samples[i]>>48;
      acquired_data[1] = samples[i]>>56;
      acquired_data[2] = samples[i]>>16; 
      acquired_data[3] = samples[i]>>24; 
      fp.write(acquired_data,4);
    }

    count_r += samples_read;
    if (count_r >= mult*buffer_size){turn = 0; count_r = 0;}

  }
    
  gpio_set_level(GPIO_NUM_12,0); 

  c2 = millis();
  Serial.println(c2-c1);

  input->stop();
  fp.close();
  free(samples);
  Serial.print("Wrote: "); Serial.print(mult*buffer_size); Serial.print(" words."); Serial.println();
  turn=1;

  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  Serial.println(free_size);

}

String checkFileSizeWithAppsScript(const String& fname) {

  char result[100]; 
  getSubstringFromSecond(fname, result);
  String urlGET = url + "?filename=" + result + "&fileSize=" + String(fileSize) + "&check_filesize=true"  + "&delete_file=false"; // Include the parameters in the URL
  Serial.println(urlGET);
  HTTPClient http;
  http.begin(urlGET);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.println(httpCode);
  if (httpCode == 200 ) {
    String response = http.getString();
    http.end(); // Release resources and close the HTTP connection
    return response;
  } else {
    Serial.println("Error in HTTP GET request to Google Apps Script");
    http.end(); // Release resources and close the HTTP connection
    return ""; // Return an empty string to indicate an error
  }
}

String deleteOldFile(const String& fname)
{
  char result[100];
  getSubstringFromSecond(fname, result);
  String urlGET = url + "?filename=" + result + "&fileSize=" + String(fileSize) + "&check_filesize=false" + "&delete_file=true"; // Include the parameters in the URL
  
  HTTPClient http;
  http.begin(urlGET);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.println(httpCode);
  if (httpCode == 200 ) {
    String response = http.getString();
    http.end(); // Release resources and close the HTTP connection
    return response;
  } else {
    Serial.println("Error in HTTP GET request to Google Apps Script");
    http.end(); // Release resources and close the HTTP connection
    return ""; // Return an empty string to indicate an error
  }

}