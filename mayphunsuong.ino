#define BLYNK_TEMPLATE_ID "TMPL6hxy5H-jj"
#define BLYNK_TEMPLATE_NAME "haq"
#define BLYNK_AUTH_TOKEN "2fYYz9ECNTT5eUXboKxN8zXQ-r-EtI36"

#include <DHT.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <EEPROM.h>

#define DHTPIN 32  
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27,16,2);
const char* ssid = "Luan";
const char* pass = "12345678";
unsigned long last_time;
unsigned long last1;
float muc_nuoc;
unsigned long lastDebounceTime = 0;
float nhietdo=0.0;
float doam=0.0;
float filterednhietdo = 0.0;
float filtereddoam = 0.0;
float ml;
#define trig 5
#define echo 18
#define MODE 26
#define EN_PIN 15
#define MODE_PIN 13
#define BUTTON_PIN 14
#define LED_PIN 27
#define RELAY1_PIN 4   
#define RELAY2_PIN 23
float ratio[] = {
    3.82, 4.15, 4.49, 4.86, 5.26, 5.26,
    5.69, 6.16, 6.65, 7.18, 7.75, 8.36,
    9.01, 9.71, 10.46, 10.46, 11.26, 12.12,
    13.03, 14.01, 15.06, 16.17, 17.37, 18.64,
    20.00, 20.00, 21.45, 22.99, 24.64, 26.40,
    28.27, 30.27, 32.41, 34.68, 37.10, 37.10,
    39.68, 42.44, 45.37
};
TaskHandle_t myTaskHandle = NULL;
TimerHandle_t myTimer; // Handle của timer
volatile bool on_off = false;
volatile bool manual_auto = false;
volatile bool en = false;
SemaphoreHandle_t sema_auto;
SemaphoreHandle_t sema_manual;
SemaphoreHandle_t control;
bool menu = 0;
// Hệ số alpha cho bộ lọc IIR thông thấp
float alpha = 0.1;
unsigned long tmenu=0;
unsigned char m1=0;
char HR= 45;
unsigned char k2=0;
unsigned char m2=0;
unsigned char m3=0;
unsigned char m4=0;
unsigned char m5=0;

void TimerCallback(TimerHandle_t xTimer) {
  if((manual_auto)&&( digitalRead(LED_PIN))){
     Serial.println("callback tắt máy");
     on_off=0;
     digitalWrite(LED_PIN,on_off);
     digitalWrite(RELAY1_PIN,on_off);
     digitalWrite(RELAY2_PIN,on_off);
     xSemaphoreGive(control);
  }
}
void IRAM_ATTR button_on_off() {// ISR xử lý nút nhấn
     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
     if (millis() - lastDebounceTime > 70) {
     xSemaphoreGiveFromISR(control, &xHigherPriorityTaskWoken); // Cấp semaphore từ ngắt
    on_off = !on_off; // Đảo trạng thái
    //if (xHigherPriorityTaskWoken == pdTRUE) { 
      lastDebounceTime = millis(); // Cập nhật thời gian ngắt cuối
     
 
    if(menu){
      if(m1&&!m2&&!m3&&!m4&&!m5){
        m1++;if(m1==3)m1=1;
      }else if(m2&&!m3&&!m4&&!m5&&(m1==1)){
        m2++;if(m2==5)m2=1;
      }else if(m3&&!m2&&!m4&&!m5&&(m1==2)){
        m3++;if(m3==5)m3=1;
      }     
    }

       }
     
     last_time=millis();
             portYIELD_FROM_ISR();  // Đảm bảo task có thể chạy ngay lập tức nếu cần
    }
 void IRAM_ATTR mode_manual_auto() {
     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
     if ( millis() - lastDebounceTime > 70) {
    xSemaphoreGiveFromISR(control, &xHigherPriorityTaskWoken); // Cấp semaphore từ ngắt
    manual_auto = !manual_auto; // Đảo trạng thái
       if(menu){
        if(m1==0)m1=1;
        else if(m1==1){
             if(m2==0)m2=1;
            else if(m2==1)HR++;if(HR>=101)HR=0;
            else if(m2==3)k2=1;
            else if(m2==4)m2=0;
        }else if(m1==2){
            if(m3==0)m3=1;
           else if(m3==4)m3=0;
        }      
     }    
        lastDebounceTime = millis(); 
    }
    last_time=millis();
    portYIELD_FROM_ISR();
}
 void IRAM_ATTR en_auto() {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
     if (millis() - lastDebounceTime > 100) {
      xSemaphoreGiveFromISR(control, &xHigherPriorityTaskWoken); // Cấp semaphore từ ngắt
    lastDebounceTime = millis(); // Cập nhật thời gian ngắt cuối
    en = !en; // Đảo trạng thái
    //if (xHigherPriorityTaskWoken == pdTRUE) {
    }
    last_time=millis()-60001;
            portYIELD_FROM_ISR();  // Đảm bảo task có thể chạy ngay lập tức nếu cần
}

BLYNK_WRITE(V0) { // Nhận tín hiệu từ virtual pin V1
    bool blynkState = param.asInt(); // Lấy trạng thái từ Blynk
    // Cấp semaphore để thay đổi trạng thái máy
    //if (xSemaphoreTake(xStateSemaphore, portMAX_DELAY)) {
        on_off = blynkState;  // Cập nhật trạng thái máy
        xSemaphoreGive(control);  // Trả lại semaphore
        Serial.println(blynkState ? "on_off: ON" : "on_off:  OFF");
        vTaskDelay(1);
    //}
}

BLYNK_WRITE(V3) { // Nhận tín hiệu từ virtual pin V1
    bool blynkState = param.asInt(); // Lấy trạng thái từ Blynk
    // Cấp semaphore để thay đổi trạng thái máy
    //if (xSemaphoreTake(xStateSemaphore, portMAX_DELAY)) {
        manual_auto = blynkState;  // Cập nhật trạng thái máy
        xSemaphoreGive(control);  // Trả lại semaphore
        Serial.println(blynkState ? "mode:  manual" : "mode:  auto");
    //}
    vTaskDelay(1);
}

// Task xử lý trạng thái máy
void task_manual(void* arg){
    while (1) {
        if (xSemaphoreTake(sema_manual, portMAX_DELAY)){
                digitalWrite(LED_PIN, on_off);
                     digitalWrite(RELAY1_PIN,on_off);
                     digitalWrite(RELAY2_PIN,on_off); 
                     Blynk.virtualWrite(V3, manual_auto);
                     Blynk.virtualWrite(V0, on_off); 
                Serial.println(on_off ? "manual ON" : "manual OFF");
                
        }
    }
}

void task_menu(void* arg){
    while (1) {
        tmenu=millis();
      while((digitalRead(MODE_PIN)==0)&&(digitalRead(BUTTON_PIN)==0)&&menu==0){
        Serial.println("Task is Suspend");
        digitalWrite(LED_PIN, 0);
             digitalWrite(RELAY1_PIN,0);
                digitalWrite(RELAY2_PIN,0);
         Serial.println("ktra menu");
          vTaskDelay(1000);
        if((millis()-tmenu >1000)&&menu==0){
          vTaskSuspend(myTaskHandle);
          vTaskDelay(1);
           Serial.println("bat menu");
            menu=1;
          while(1){
             Serial.println("menu");
             Serial.println(menu);
              vTaskDelay(1000);
             if((digitalRead(MODE_PIN)==0)&&(digitalRead(BUTTON_PIN)==0)&&menu==1){
               vTaskDelay(10);
              if(millis()-tmenu >1000){
                 xSemaphoreGive(control);
                 vTaskResume(myTaskHandle);
                 Serial.println("Task is resumed"); 
                break;
              }else{ tmenu=millis();}
             }
             
         }
        }
      }
    
      menu=0;tmenu=0;m1=0;k2=0;m2=0;m3=0;m4=0;m5=0;
     vTaskDelay(100);
  }
   
}
   
void task_control(void* arg){
    while (1) {
      if (xSemaphoreTake(control, portMAX_DELAY) == pdTRUE) {      
        if (!manual_auto){
          digitalWrite(MODE,manual_auto);
                Serial.println(on_off);
            Serial.println("GIVE sema_manual");
            xSemaphoreGive(sema_manual);  
        }else if ((manual_auto)&&(digitalRead(15))){
          Blynk.virtualWrite(V3, manual_auto);
        Blynk.virtualWrite(V0, on_off);
          digitalWrite(MODE,manual_auto);
          xSemaphoreGive(sema_auto); 
          Serial.println("GIVE sema_auto");
        }else if((manual_auto)&&(!digitalRead(15))){
        on_off=0;
        Blynk.virtualWrite(V3, manual_auto);
        Blynk.virtualWrite(V0, on_off);
        digitalWrite(LED_PIN, on_off);
             digitalWrite(RELAY1_PIN,on_off);
            digitalWrite(RELAY2_PIN,on_off);
        digitalWrite(MODE,manual_auto);
        } 
    }
vTaskDelay(10);


  }
}

void task_auto(void* arg) {
  HR =EEPROM.readChar(1);
  last_time = millis();
    while (1) {
        if (xSemaphoreTake(sema_auto, portMAX_DELAY)) {
          if(millis()- (last_time + 60000) > 0){
                if(filtereddoam <= HR){
                  Serial.println(en);
                Serial.println(on_off);
                unsigned int t =((float)HR-filtereddoam)*(1.0/100)*(ratio[(int)nhietdo])*(245.0/2)*(1.0/2)*(1.0/300)*(60*60*1000);
                Serial.printf("thoi gian phun: ");  Serial.print(t); Serial.println("ms"); 
                on_off=digitalRead(15);
                digitalWrite(LED_PIN, on_off); 
                 digitalWrite(RELAY1_PIN,on_off);
                 digitalWrite(RELAY2_PIN,on_off);
                Blynk.virtualWrite(V0, on_off);
                Serial.println(en);
                Serial.println(on_off);
                Serial.println(on_off ? "auto ON" : "auto OFF");
                last_time = t + millis();  
                if (myTimer != NULL){
                   xTimerDelete(myTimer, 0);  // Xóa timer cũ
                  }
                myTimer = xTimerCreate("MyTimer", pdMS_TO_TICKS(t), pdFALSE, (void*)0, TimerCallback);
                    if (myTimer == NULL) {
                     Serial.println("Không thể tạo timer.");
                     return;
                  }
                    if (xTimerStart(myTimer, 0) != pdPASS) {
                    Serial.println("Không thể bắt đầu timer.");
                    return;
                  }
            }   
        }
      }  
    }
}
  
void blynk_Task(void* arg) {
  last1=millis();
    while (1) {
      if (!Blynk.connected()) {
         Serial.println("Blynk disconnected. Reconnecting...");
         Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
        }
       Blynk.run();
       if (millis() - last1 >= 3000){
       Blynk.virtualWrite(V2, filterednhietdo);
       Blynk.virtualWrite(V1, filtereddoam);
       Blynk.virtualWrite(V4, ml);
       last1 = millis();
  }
       vTaskDelay(10);
    }
}

void LCD_Task(void* arg) {
       lcd.setCursor(0,0);
       lcd.printf("xin chao!toi la:"); 
       lcd.setCursor(0,1);
       lcd.printf("<may phun suong>"); 
       lcd.display();
       vTaskDelay(4000);
  unsigned int tt; 
    while (1){
   if(menu){
    Serial.println("**********");
     Serial.println(m1);
     Serial.println(m2);
     Serial.println(m3);
     Serial.println(m4);
     Serial.println(m5);
     Serial.println("___________");
    switch(m1){
     case 0:
      //lcd.clear();
      lcd.setCursor(0,0);
      lcd.printf("  N1 & N2 EXIT  "); 
       lcd.setCursor(0,1);
       lcd.printf("N1:Menu/N2:XUONG"); 
       lcd.display(); 
       break;
    case 1:
         switch(m2){
         case 0:
          lcd.setCursor(0,0);
          lcd.printf(">DO AM MONG MUON"); 
          lcd.setCursor(0,1);
          lcd.printf("THONG SO        "); 
          lcd.display(); 
         break; 
          case 1:
          tt=millis();
          while(digitalRead(MODE_PIN)==0){
            if(millis()-tt>50){
            HR++;
            if(HR>=101)HR=0;
             lcd.setCursor(0,0);
          lcd.printf(">SET H = %d%%     ",HR); 
          lcd.setCursor(0,1);
          lcd.printf("Khuyen dung 45%%"); 
          lcd.display();
          tt=millis();
            }
            vTaskDelay(50);
          }
          lcd.setCursor(0,0);
          lcd.printf(">SET H = %d%%     ",HR); 
          lcd.setCursor(0,1);
          lcd.printf("Khuyen dung 45%%"); 
          lcd.display(); 
         break;
         case 2
          lcd.setCursor(0,0);
          lcd.printf(">Khuyen dung 45%"); 
          lcd.setCursor(0,1);
          lcd.printf("SAVE<--         "); 
          lcd.display(); 
         break;
         case 3:
            if(!k2){
          lcd.setCursor(0,0);
          lcd.printf(">SAVE<--        "); 
          lcd.setCursor(0,1);
          lcd.printf("BACK-->         "); 
          lcd.display(); 
          tt=millis();
          }else if(k2){
            if(millis()-tt>1500){k2=0;}
            lcd.setCursor(0,0);
            lcd.printf("     DA LUU     "); 
            lcd.setCursor(0,1);
            lcd.printf("H = %d%%       ",HR); 
            lcd.display();
            EEPROM.writeChar(1,HR);
            EEPROM.commit();
              }
              break;
          case 4:
          lcd.setCursor(0,0);
          lcd.printf("SAVE<--         "); 
          lcd.setCursor(0,1);
          lcd.printf(">BACK-->        "); 
          lcd.display(); 
         break;  
            }
          break;
       case 2:
         switch(m3){
         case 0:
          lcd.setCursor(0,0);
          lcd.printf("DO AM MONG MUON "); 
          lcd.setCursor(0,1);
          lcd.printf(">THONG SO       "); 
          lcd.display(); 
         break;
          case 1:
          lcd.setCursor(0,0);
          lcd.printf("Dung tich: 1 lit"); 
          lcd.setCursor(0,1);
          lcd.printf("Cong suat: 5,3W "); 
          lcd.display(); 
         break;
         case 2:
          lcd.setCursor(0,0);
          lcd.printf("  Toc do phun   "); 
          lcd.setCursor(0,1);
          lcd.printf(" toi da 300ml/h "); 
          lcd.display(); 
         break;
         case 3:
          lcd.setCursor(0,0);
          lcd.printf("Dien tich sudung"); 
          lcd.setCursor(0,1);
          lcd.printf("    10 - 25m2   "); 
          lcd.display(); 
         break;
         case 4:
          lcd.setCursor(0,0);
          lcd.printf(">BACK-->        "); 
          lcd.setCursor(0,1);
          lcd.printf("----------------"); 
          lcd.display(); 
         break;
         }
        break; 
    }
   }else{
      if(ml>=90){
       lcd.setCursor(0,0);
        lcd.printf("T:%.1foC/H:%.1f%%",filterednhietdo,filtereddoam);
       lcd.setCursor(0,1);
       lcd.printf("Con %.2f ml    ",ml);
       lcd.display();
       vTaskDelay(2000);   
        
     }else{
      //lcd.clear();
      lcd.setCursor(0,0);
      lcd.printf("     VUI LONG   "); 
      lcd.setCursor(0,1);
      lcd.printf("   THEM NUOC!!  "); 
      lcd.display(); 
  }
}
       vTaskDelay(100);
  }
}

void read_sensor(void* arg) {
  bool ab=0;
  float thoi_gian;
    while (1) {
      nhietdo = dht.readTemperature();
      doam = dht.readHumidity();
      
      if (isnan(filterednhietdo) || isnan(filtereddoam)) {
      filterednhietdo = dht.readTemperature();
      filtereddoam = dht.readHumidity();
    }
       //Áp dụng bộ lọc thông thấp IIR cho dữ liệu đọc
      filterednhietdo = alpha*nhietdo + (1.0-alpha)*filterednhietdo;
      filtereddoam = alpha*doam+(1.0-alpha)*filtereddoam;
      digitalWrite(trig, 0);
      delayMicroseconds(2);
      digitalWrite(trig, 1);
      delayMicroseconds(10);
      digitalWrite(trig, 0);
       thoi_gian = pulseIn(echo, HIGH);
      muc_nuoc= (thoi_gian/2/29.412);
      ml=(6.0-muc_nuoc)*18.0*10.0;
      vTaskDelay(1);
      if ((ml < 90 )&&(ab==0)){
        ab=1;
        Serial.println("Task is Suspend");
        digitalWrite(LED_PIN, 0);
             digitalWrite(RELAY1_PIN,0);
                digitalWrite(RELAY2_PIN,0);
        vTaskSuspend(myTaskHandle);
  }else if ((ml >=90 ) &&(ab==1)){
     ab=0;
    xSemaphoreGive(control);
    vTaskResume(myTaskHandle);
    Serial.println("Task is resumed"); 
  }
  vTaskDelay(1000);
 }

}

void setup(){ 
    Serial.begin(9600);
    pinMode(EN_PIN, INPUT_PULLUP);
    pinMode(MODE, OUTPUT);
    pinMode(MODE_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
      pinMode(trig,OUTPUT);
      pinMode(echo,INPUT);
      lcd.init();
    lcd.backlight();
    dht.begin();
    //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    EEPROM.begin(8); // ESP32 cần khai báo dung lượng EEPROM
  filterednhietdo = dht.readTemperature();
  filtereddoam = dht.readHumidity();
    sema_auto = xSemaphoreCreateBinary();
    sema_manual= xSemaphoreCreateBinary();
    control = xSemaphoreCreateBinary();
    //on_off= xSemaphoreCreateBinary();
    attachInterrupt(BUTTON_PIN, button_on_off, FALLING); // Gắn ngắt ngoài cho nút nhấn
    attachInterrupt(MODE_PIN, mode_manual_auto, FALLING); // Gắn ngắt ngoài cho nút nhấn
    attachInterrupt(EN_PIN, en_auto, CHANGE);
    HR= EEPROM.readChar(1);
    xTaskCreate(task_control,"Task_control",4098,NULL,4, &myTaskHandle );
    xTaskCreate(task_manual,"Task_on_offf",4098,NULL,3,NULL);
    xTaskCreate(task_auto,"task_auto",4098,NULL,3,NULL);
    xTaskCreate(read_sensor,"Task_sensor",2048,NULL,2,NULL);
    xTaskCreate(LCD_Task,"Task_LCD",4096,NULL,2,NULL); 
    xTaskCreate(task_menu,"Task_menu",4096,NULL,4,NULL); 
    xTaskCreate(blynk_Task,"Task_blynk",4096,NULL,3,NULL); 
    Serial.println("Setup complete.");
}
void loop() {}
